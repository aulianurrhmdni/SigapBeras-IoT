// ============================================================
// UTS IoT (LO2514020) - Kelompok 6 - Pilihan 1 (Modul 1 + Modul 2)
// SIGAP Beras: Pemantau Kelembapan & Titik Panas Gudang Beras
// ------------------------------------------------------------
// DHT22 → GPIO 27 | DS18B20 → GPIO 25 (pull-up 10kΩ ke 3V3)
// LCD I2C → SDA(21)/SCL(22), VCC ke VIN (5V)
// LED Hijau → GPIO 18 | LED Merah → GPIO 19 (masing-masing 220Ω)
// Buzzer Aktif → GPIO 23 | Push Button → GPIO 13 (aktif LOW)
// ------------------------------------------------------------
// [LOGIKA BARU]  : titik panas tumpukan, durasi paparan,
//                  aturan tombol konfirmasi petugas
// [NILAI TAMBAH] : kirim data ke cloud ThingSpeak (grafik otomatis)
// ============================================================
#include <WiFi.h>              // Library WiFi bawaan ESP32 (Modul 1)
#include <HTTPClient.h>        // Kirim data lewat HTTP ke ThingSpeak
#include <DHT.h>               // Sensor DHT22 (Modul 2)
#include <OneWire.h>           // Protokol 1-Wire untuk DS18B20
#include <DallasTemperature.h> // Sensor DS18B20 (Modul 2)
#include <LiquidCrystal_I2C.h> // LCD 16x2 I2C (Modul 2)

// === Pengaturan WiFi dan ThingSpeak kelompok ===
const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";
const char* thingspeakKey = "ISI_WRITE_API_KEY";
                                     // Write API Key dari ThingSpeak

// === Definisi Pin (sesuai Tabel Alokasi Pin) ===
#define PIN_DHT22     27
#define PIN_DS18B20   25
#define PIN_LED_HIJAU 18
#define PIN_LED_MERAH 19
#define PIN_BUZZER    23
#define PIN_TOMBOL    13

// === Batas Alarm ===
#define RH_WASPADA      70.0   // RH di atas 70%  = WASPADA (zona aman beras <=70%)
#define RH_KRITIS       80.0   // RH mulai 80%    = KRITIS  (melewati batas SNI 6128:2020)
#define SELISIH_WASPADA 3.0    // tumpukan lebih panas >=3°C dari udara = WASPADA
#define SELISIH_KRITIS  5.0    // tumpukan lebih panas >=5°C dari udara = KRITIS

// === Pengaturan Waktu (milidetik) ===
#define INTERVAL_BACA     2000   // baca sensor tiap 2 detik
#define BATAS_WASPADA     30000  // WASPADA lebih dari 30 detik naik jadi KRITIS
#define DURASI_SENYAP     60000  // buzzer senyap 60 detik setelah tombol ditekan
#define TEKAN_LAMA        1000   // tombol ditahan >=1 detik = ganti tampilan LCD
#define INTERVAL_THINGSPEAK 20000 // kirim ke ThingSpeak tiap 20 detik (akun gratis min. 15 detik)

// === Kode Status ===
#define AMAN    0
#define WASPADA 1
#define KRITIS  2
#define ERROR   3
const char* NAMA_STATUS[] = {"AMAN", "WASPADA", "KRITIS", "ERROR"};

// === Inisialisasi Objek Sensor ===
DHT dht(PIN_DHT22, DHT22);
OneWire oneWire(PIN_DS18B20);
DallasTemperature ds18b20(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);   // jika LCD kosong, ganti 0x27 menjadi 0x3F

// === Variabel Data Sensor ===
float suhuUdara = NAN;   // suhu udara gudang (DHT22)
float kelembapan = NAN;  // kelembapan rata-rata 3 data terakhir (DHT22)
float suhuBeras = NAN;   // suhu di dalam tumpukan beras (DS18B20)
float selisih = NAN;     // suhuBeras - suhuUdara
float dataRH[3];         // array moving average kelembapan
int indeksRH = 0, jumlahRH = 0;
int gagalDHT = 0, gagalDS = 0;   // hitungan gagal baca berturut-turut

// === Variabel Status ===
int status = AMAN, statusLama = AMAN;
const char* alasan = "NORMAL";
bool sedangWaspada = false;
unsigned long mulaiWaspada = 0;
bool senyap = false;
unsigned long mulaiSenyap = 0;

// === Variabel Timer, Tombol, dan Tampilan ===
unsigned long waktuBaca = 0;
int hitungData = 0;                 // nomor data di Serial Monitor
int halamanLCD = 0;                 // 0 = data sensor, 1 = info WiFi
bool tombolStabil = HIGH, bacaanTerakhir = HIGH, tekanLamaSelesai = false;
unsigned long waktuBerubah = 0, waktuTekan = 0;

// === Variabel ThingSpeak ===
// Pengiriman ke ThingSpeak berjalan di core 0 (ESP32 dual core), terpisah dari
// loop() di core 1, supaya alarm dan tombol tidak tertunda saat menunggu internet.
struct DataKirim { float u, h, b, d; int st; const char* al; bool senyap; bool valid; };
DataKirim dataKirim = {NAN, NAN, NAN, NAN, AMAN, "NORMAL", false, false};
portMUX_TYPE kunciData = portMUX_INITIALIZER_UNLOCKED;   // pengaman data antar-core
char kejadianTerakhir[64] = "";   // kejadian terbaru, dikirim sebagai "status" ThingSpeak
bool adaKejadianBaru = false;

// ============================================================
// FUNGSI BANTU
// ============================================================

// Catat kejadian ke Serial Monitor dan simpan untuk dikirim ke ThingSpeak
void catatKejadian(String pesan) {
  Serial.println(">> KEJADIAN: " + pesan);
  portENTER_CRITICAL(&kunciData);
  strncpy(kejadianTerakhir, pesan.c_str(), sizeof(kejadianTerakhir) - 1);
  kejadianTerakhir[sizeof(kejadianTerakhir) - 1] = '\0';
  adaKejadianBaru = true;
  portEXIT_CRITICAL(&kunciData);
}

// ============================================================
// 1. KONEKSI WiFi (mengikuti Modul 1 - Program 2)
// ============================================================
void sambungWiFi() {
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Sambung WiFi...");
  WiFi.begin(ssid, password);

  // LED merah berkedip selama proses koneksi (maksimal 20 detik)
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < 20000) {
    digitalWrite(PIN_LED_MERAH, !digitalRead(PIN_LED_MERAH));
    delay(500);
    Serial.print(".");
  }
  digitalWrite(PIN_LED_MERAH, LOW);
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("============================");
    Serial.println("WiFi Terhubung!");
    Serial.print("Alamat IP   : "); Serial.println(WiFi.localIP());
    Serial.print("Sinyal RSSI : "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
    Serial.println("============================");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
  } else {
    // Alat tetap memantau walau WiFi gagal (ThingSpeak saja yang tidak jalan)
    Serial.println("[ERROR] WiFi gagal. Alat tetap berjalan tanpa ThingSpeak.");
    lcd.setCursor(0, 1); lcd.print("WiFi gagal");
  }
  delay(2000);
}

// ============================================================
// 2. PEMBACAAN SENSOR (mengikuti Modul 2 - Fase 3)
// ============================================================
void bacaSensor() {
  // === Baca DHT22 ===
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    gagalDHT++;   // gagal sesekali itu wajar: nilai terakhir tetap dipakai
    Serial.println("ERROR: Gagal membaca DHT22! Periksa kabel DATA.");
  } else {
    gagalDHT = 0;
    suhuUdara = t;
    // Moving average 3 data terakhir agar lonjakan sesaat tidak memicu alarm
    dataRH[indeksRH] = h;
    indeksRH = (indeksRH + 1) % 3;
    if (jumlahRH < 3) jumlahRH++;
    float total = 0;
    for (int i = 0; i < jumlahRH; i++) total += dataRH[i];
    kelembapan = total / jumlahRH;
  }

  // === Baca DS18B20 ===
  // Ambil hasil pengukuran sebelumnya, lalu minta pengukuran baru tanpa
  // menunggu 750 ms, sehingga tombol tetap responsif.
  float b = ds18b20.getTempCByIndex(0);
  ds18b20.requestTemperatures();
  if (b == -127.0 || b == 85.0) {   // -127 = tidak terdeteksi, 85 = gagal ukur
    gagalDS++;
    Serial.println("ERROR: DS18B20 tidak terbaca! Periksa kabel dan resistor 10k.");
  } else {
    gagalDS = 0;
    suhuBeras = b;
  }

  // [LOGIKA BARU] Selisih suhu tumpukan beras terhadap udara gudang.
  // Aktivitas hama/jamur menghasilkan panas, sehingga tumpukan lebih panas.
  selisih = suhuBeras - suhuUdara;
}

// ============================================================
// 3. LOGIKA STATUS 3 TINGKAT
// ============================================================
void tentukanStatus() {
  if (gagalDHT >= 3 || gagalDS >= 3 || isnan(suhuUdara) || isnan(suhuBeras)) {
    status = ERROR;                  // gagal 3x berturut-turut (±6 detik)
    alasan = "SENSOR ERROR";
    sedangWaspada = false;
  } else {
    // Status kelembapan (RH)
    int stRH = AMAN;
    if (kelembapan <= RH_WASPADA) {                                   // RH <= 70
      stRH = AMAN;
    } else if (kelembapan > RH_WASPADA && kelembapan < RH_KRITIS) {   // 70 < RH < 80
      stRH = WASPADA;
    } else if (kelembapan >= RH_KRITIS) {                             // RH >= 80
      stRH = KRITIS;
    }

    // Status titik panas (selisih suhu beras - udara)
    int stPanas = AMAN;
    if (selisih < SELISIH_WASPADA) {                                         // selisih < 3
      stPanas = AMAN;
    } else if (selisih >= SELISIH_WASPADA && selisih < SELISIH_KRITIS) {     // 3 <= selisih < 5
      stPanas = WASPADA;
    } else if (selisih >= SELISIH_KRITIS) {                                  // selisih >= 5
      stPanas = KRITIS;
    }

    // Status akhir = yang paling parah
    status = max(stRH, stPanas);
    if (stRH > AMAN && stPanas > AMAN) alasan = "RH + TITIK PANAS";
    else if (stRH > AMAN)              alasan = "RH TINGGI";
    else if (stPanas > AMAN)           alasan = "TITIK PANAS";
    else                               alasan = "NORMAL";

    // [LOGIKA BARU] Durasi paparan: WASPADA yang bertahan lebih dari
    // 30 detik dinaikkan menjadi KRITIS, karena kerusakan beras
    // bergantung pada lamanya terpapar kondisi buruk.
    if (status == WASPADA) {
      if (!sedangWaspada) { sedangWaspada = true; mulaiWaspada = millis(); }
      if (millis() - mulaiWaspada >= BATAS_WASPADA) {
        status = KRITIS;
        alasan = "WASPADA >30 dtk";
      }
    } else {
      sedangWaspada = false;
    }
  }

  // Catat setiap perubahan status
  if (status != statusLama) {
    // [LOGIKA BARU] Bila kondisi memburuk atau kembali aman, buzzer
    // diaktifkan lagi walaupun sebelumnya sudah dikonfirmasi petugas.
    if ((status > statusLama && status != ERROR) || status == AMAN) senyap = false;
    catatKejadian(String(NAMA_STATUS[statusLama]) + " -> " + NAMA_STATUS[status] + " (" + alasan + ")");
    statusLama = status;
  }
}

// ============================================================
// 4. OUTPUT LED DAN BUZZER
// ============================================================
// AMAN    : LED hijau menyala, buzzer mati
// WASPADA : LED merah berkedip, buzzer bip pendek tiap 2 detik
// KRITIS  : LED merah menyala terus, buzzer berbunyi terus
// ERROR   : LED hijau dan merah berkedip bergantian, buzzer mati
void aturOutput() {
  bool kedip = (millis() / 500) % 2;
  bool hijau = (status == AMAN) || (status == ERROR && kedip);
  bool merah = (status == KRITIS) || (status == WASPADA && kedip) || (status == ERROR && !kedip);
  bool bunyi = (status == KRITIS) || (status == WASPADA && millis() % 2000 < 200);
  if (senyap) bunyi = false;

  digitalWrite(PIN_LED_HIJAU, hijau ? HIGH : LOW);
  digitalWrite(PIN_LED_MERAH, merah ? HIGH : LOW);
  digitalWrite(PIN_BUZZER, bunyi ? HIGH : LOW);
}

// ============================================================
// 5. PUSH BUTTON (aktif LOW, pull-up internal seperti Modul 1)
// ============================================================
// Tekan singkat (<1 detik)  : konfirmasi petugas
// Tekan lama (>=1 detik)    : ganti tampilan LCD (data sensor / info WiFi)
void konfirmasiPetugas() {
  // [LOGIKA BARU] Saat WASPADA/KRITIS: buzzer senyap 60 detik.
  // Saat AMAN: dicatat sebagai pengecekan rutin petugas.
  if (status == WASPADA || status == KRITIS) {
    senyap = true;
    mulaiSenyap = millis();
    catatKejadian("Petugas konfirmasi, buzzer senyap 60 dtk");
  } else {
    catatKejadian(String("Cek rutin petugas, status ") + NAMA_STATUS[status]);
  }
}

void tampilLCD();   // deklarasi awal (fungsinya ada di bawah)

void cekTombol() {
  bool baca = digitalRead(PIN_TOMBOL);
  if (baca != bacaanTerakhir) {         // debounce 50 ms
    waktuBerubah = millis();
    bacaanTerakhir = baca;
  }
  if (millis() - waktuBerubah > 50 && baca != tombolStabil) {
    tombolStabil = baca;
    if (tombolStabil == LOW) {          // tombol mulai ditekan
      waktuTekan = millis();
      tekanLamaSelesai = false;
    } else if (!tekanLamaSelesai) {     // dilepas sebelum 1 detik
      konfirmasiPetugas();
    }
  }
  if (tombolStabil == LOW && !tekanLamaSelesai && millis() - waktuTekan >= TEKAN_LAMA) {
    tekanLamaSelesai = true;            // ditahan >= 1 detik
    halamanLCD = 1 - halamanLCD;
    tampilLCD();
  }
}

// ============================================================
// 6. TAMPILAN LCD DAN SERIAL MONITOR (format tabel Modul 2)
// ============================================================
void tampilLCD() {
  lcd.clear();
  if (halamanLCD == 0) {
    // Baris 1: suhu udara & kelembapan | Baris 2: suhu beras & status
    lcd.setCursor(0, 0);
    lcd.print("U:"); lcd.print(suhuUdara, 1);
    lcd.print("C H:"); lcd.print(kelembapan, 1); lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("B:"); lcd.print(suhuBeras, 1); lcd.print("C ");
    lcd.print(NAMA_STATUS[status]);
    if (senyap) lcd.print("*");         // tanda * = buzzer sedang disenyapkan
  } else {
    // Info koneksi WiFi: alamat IP dan kekuatan sinyal (RSSI)
    lcd.setCursor(0, 0);
    if (WiFi.status() == WL_CONNECTED) lcd.print(WiFi.localIP());
    else lcd.print("WiFi terputus");
    lcd.setCursor(0, 1);
    lcd.print("RSSI:"); lcd.print(WiFi.RSSI()); lcd.print(" dBm");
  }
}

void tampilSerial() {
  Serial.printf("%3d | %13.1f | %13.1f | %15.1f | %+10.1f | %s (%s)%s\n",
                hitungData, suhuUdara, kelembapan, suhuBeras, selisih,
                NAMA_STATUS[status], alasan, senyap ? " [SENYAP]" : "");
  // Status koneksi WiFi (IP & RSSI) dicetak tiap 10 data
  if (hitungData % 10 == 0) {
    Serial.printf("    WiFi: %s | IP: %s | RSSI: %d dBm\n",
                  WiFi.status() == WL_CONNECTED ? "Terhubung" : "Terputus",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  }
}

// ============================================================
// 7. [NILAI TAMBAH] KIRIM DATA KE CLOUD THINGSPEAK
// ============================================================
// Isi channel ThingSpeak:
//   field1 = suhu udara (C)      field2 = kelembapan (%)
//   field3 = suhu beras (C)      field4 = selisih suhu (C)
//   field5 = status (0 AMAN, 1 WASPADA, 2 KRITIS, 3 ERROR)
//   field6 = buzzer senyap (0/1) field7 = RSSI WiFi (dBm)
//   status = teks kejadian terbaru / alasan status

// Ubah teks agar aman dikirim lewat alamat URL (spasi menjadi %20, dst.)
String urlEncode(String teks) {
  String hasil = "";
  const char* hex = "0123456789ABCDEF";
  for (unsigned int i = 0; i < teks.length(); i++) {
    char c = teks[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.') {
      hasil += c;
    } else {
      hasil += '%';
      hasil += hex[(c >> 4) & 0x0F];
      hasil += hex[c & 0x0F];
    }
  }
  return hasil;
}

// Tambahkan satu field ke URL (dilewati bila nilainya NaN / sensor error)
void tambahField(String &url, int nomor, float nilai) {
  if (!isnan(nilai)) url += "&field" + String(nomor) + "=" + String(nilai, 1);
}

void tugasThingSpeak(void* parameter) { // berjalan terus di core 0
  unsigned long terakhirKirim = 0;
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(200));
    if (millis() - terakhirKirim < INTERVAL_THINGSPEAK || WiFi.status() != WL_CONNECTED) continue;

    // Salin data dari core 1 dengan aman
    DataKirim d;
    char kejadian[64];
    bool kejadianBaru;
    portENTER_CRITICAL(&kunciData);
    d = dataKirim;
    strcpy(kejadian, kejadianTerakhir);
    kejadianBaru = adaKejadianBaru;
    adaKejadianBaru = false;
    portEXIT_CRITICAL(&kunciData);
    if (!d.valid) continue;
    terakhirKirim = millis();

    // Susun alamat pengiriman data
    String url = "http://api.thingspeak.com/update?api_key=" + String(thingspeakKey);
    tambahField(url, 1, d.u);
    tambahField(url, 2, d.h);
    tambahField(url, 3, d.b);
    tambahField(url, 4, d.d);
    url += "&field5=" + String(d.st);
    url += "&field6=" + String(d.senyap ? 1 : 0);
    url += "&field7=" + String(WiFi.RSSI());
    String teksStatus = kejadianBaru ? String(kejadian)
                                     : String(NAMA_STATUS[d.st]) + " (" + d.al + ")";
    url += "&status=" + urlEncode(teksStatus);

    // Kirim ke ThingSpeak
    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    http.setTimeout(5000);
    int kode = http.GET();
    String balasan = (kode == 200) ? http.getString() : "";
    http.end();

    // ThingSpeak membalas nomor entri (>0) bila berhasil, "0" bila ditolak
    if (kode == 200 && balasan.toInt() > 0) {
      Serial.println("[ThingSpeak] Data terkirim, entri ke-" + balasan);
    } else if (kode == 200) {
      Serial.println("[ThingSpeak] Ditolak (cek Write API Key, atau kirim terlalu cepat)");
    } else {
      Serial.printf("[ThingSpeak] Gagal kirim, kode %d (cek internet hotspot)\n", kode);
    }
  }
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_HIJAU, OUTPUT);
  pinMode(PIN_LED_MERAH, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_TOMBOL, INPUT_PULLUP);    // nilai default HIGH, LOW saat ditekan

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("SIGAP Beras");
  lcd.setCursor(0, 1); lcd.print("Kelompok 6");
  delay(2000);

  dht.begin();
  ds18b20.begin();
  ds18b20.requestTemperatures();          // pengukuran pertama (menunggu)
  ds18b20.setWaitForConversion(false);    // berikutnya tidak menunggu

  sambungWiFi();

  // Jalankan pengiriman ThingSpeak di core 0
  xTaskCreatePinnedToCore(tugasThingSpeak, "ThingSpeak", 8192, NULL, 1, NULL, 0);
  catatKejadian("Alat dinyalakan");

  // Header tabel di Serial Monitor
  Serial.println("No  | Suhu_DHT22(C) | Humi_DHT22(%) | Suhu_DS18B20(C) | Selisih(C) | Status");
  Serial.println("----+---------------+---------------+-----------------+------------+-----------");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  cekTombol();     // dicek setiap saat agar tombol responsif
  aturOutput();    // LED & buzzer diperbarui setiap saat

  // Masa senyap buzzer selesai setelah 60 detik
  if (senyap && millis() - mulaiSenyap >= DURASI_SENYAP) {
    senyap = false;
    catatKejadian("Masa senyap selesai");
  }

  // Baca sensor setiap 2000 ms
  if (millis() - waktuBaca < INTERVAL_BACA) return;
  waktuBaca = millis();
  hitungData++;

  bacaSensor();
  tentukanStatus();
  tampilSerial();
  tampilLCD();

  // Simpan salinan data untuk dikirim ke ThingSpeak oleh core 0
  portENTER_CRITICAL(&kunciData);
  dataKirim = {suhuUdara, kelembapan, suhuBeras, selisih, status, alasan, senyap, true};
  portEXIT_CRITICAL(&kunciData);
}