# SIGAP BERAS – Sistem Pemantauan Gudang Beras Berbasis IoT

## 1. Deskripsi Proyek

SIGAP BERAS merupakan prototipe sistem pemantauan suhu dan kelembapan gudang beras berbasis Internet of Things (IoT) yang dikembangkan oleh Kelompok 6 GUNUNG KAWI, Kelas B, Program Studi Teknik Logistik, Institut Teknologi Kalimantan (ITK).

Sistem menggunakan mikrokontroler ESP32, sensor DHT22 untuk mengukur suhu dan kelembapan udara, serta sensor DS18B20 untuk mengukur suhu pada media beras. Hasil pembacaan ditampilkan melalui LCD 16×2 I2C dan Serial Monitor Arduino IDE.

Sistem dilengkapi indikator LED, buzzer, dan tombol konfirmasi petugas. Data pemantauan juga dikirimkan melalui jaringan Wi-Fi ke platform ThingSpeak sehingga dapat diamati dalam bentuk grafik secara daring.

## 2. Tujuan Pengembangan

1. Memantau suhu dan kelembapan udara pada lingkungan penyimpanan beras.
2. Mengukur suhu media beras menggunakan sensor DS18B20.
3. Menghitung selisih suhu media beras terhadap suhu udara.
4. Menentukan kondisi AMAN, WASPADA, KRITIS, atau ERROR.
5. Memberikan peringatan melalui LED dan buzzer.
6. Menampilkan hasil pemantauan melalui LCD dan Serial Monitor.
7. Mengirimkan data sensor ke ThingSpeak untuk pemantauan jarak jauh.

## 3. Komponen yang Digunakan

| No. | Komponen | Fungsi |
| --- | --- | --- |
| 1 | ESP32 DevKit | Pengendali utama dan koneksi Wi-Fi |
| 2 | DHT22 | Mengukur suhu dan kelembapan udara |
| 3 | DS18B20 | Mengukur suhu media beras |
| 4 | LCD 16×2 I2C | Menampilkan data sensor dan informasi sistem |
| 5 | LED hijau | Indikator kondisi AMAN |
| 6 | LED merah | Indikator kondisi WASPADA dan KRITIS |
| 7 | Buzzer aktif | Memberikan peringatan suara |
| 8 | Push button | Konfirmasi alarm dan pergantian tampilan LCD |
| 9 | Resistor 220 Ω | Membatasi arus LED |
| 10 | Resistor 10 kΩ | Pull-up jalur DATA DS18B20 |
| 11 | Breadboard dan kabel jumper | Menghubungkan komponen |
| 12 | Kabel USB | Menghubungkan ESP32 ke komputer |

## 4. Tabel Alokasi Pin ESP32

| Komponen | Pin ESP32 | Keterangan |
| --- | --- | --- |
| DHT22 DATA | GPIO 27 | Pembacaan suhu dan kelembapan |
| DS18B20 DATA | GPIO 25 | Menggunakan resistor pull-up 10 kΩ ke 3,3 V |
| LED hijau | GPIO 18 | Menggunakan resistor seri 220 Ω |
| LED merah | GPIO 19 | Menggunakan resistor seri 220 Ω |
| Buzzer | GPIO 23 | Kendali alarm suara |
| Push button | GPIO 13 | Input dengan INPUT_PULLUP |
| LCD SDA | GPIO 21 | Jalur data I2C |
| LCD SCL | GPIO 22 | Jalur clock I2C |

Konfigurasi pin tersebut digunakan sebagai acuan perakitan rangkaian SIGAP BERAS.

## 5. Panduan Perakitan Rangkaian Fisik

### 5.1 Persiapan ESP32

1. Siapkan ESP32, breadboard, sensor, dan komponen pendukung.
2. Pastikan kabel USB belum terhubung saat melakukan pemasangan.
3. Tentukan jalur 3,3 V dan GND pada breadboard.
4. Hubungkan jalur GND setiap komponen ke GND ESP32.

### 5.2 Pemasangan Sensor DHT22

1. Hubungkan VCC sensor DHT22 ke 3,3 V ESP32.
2. Hubungkan GND sensor ke GND ESP32.
3. Hubungkan pin DATA sensor ke GPIO 27.
4. Pastikan susunan pin sesuai jenis sensor atau modul DHT22 yang digunakan.

### 5.3 Pemasangan Sensor DS18B20

1. Hubungkan kabel VCC sensor ke 3,3 V ESP32.
2. Hubungkan kabel GND sensor ke GND ESP32.
3. Hubungkan kabel DATA sensor ke GPIO 25.
4. Pasang resistor pull-up 10 kΩ antara DATA dan 3,3 V.
5. Tempatkan bagian probe pada media beras untuk pengujian suhu.

Warna kabel sensor harus diperiksa berdasarkan datasheet perangkat yang digunakan.

### 5.4 Pemasangan LCD I2C

1. Hubungkan pin SDA LCD ke GPIO 21.
2. Hubungkan pin SCL LCD ke GPIO 22.
3. Hubungkan GND LCD ke GND ESP32.
4. Hubungkan VCC LCD sesuai spesifikasi modul.
5. Pastikan level tegangan jalur I2C kompatibel dengan ESP32.

Alamat LCD yang digunakan dalam program adalah 0x27.

### 5.5 Pemasangan LED dan Buzzer

1. Hubungkan LED hijau ke GPIO 18 melalui resistor 220 Ω.
2. Hubungkan LED merah ke GPIO 19 melalui resistor 220 Ω.
3. Hubungkan kaki negatif masing-masing LED ke GND.
4. Hubungkan kendali buzzer ke GPIO 23.
5. Gunakan rangkaian penggerak buzzer apabila kebutuhan arusnya melebihi kemampuan GPIO ESP32.

### 5.6 Pemasangan Push Button

1. Hubungkan salah satu terminal tombol ke GPIO 13.
2. Hubungkan terminal lainnya ke GND.
3. Program menggunakan INPUT_PULLUP sehingga tombol terbaca LOW ketika ditekan.

## 6. Persiapan Arduino IDE

### 6.1 Instalasi Perangkat Lunak

1. Instal Arduino IDE pada komputer.
2. Tambahkan dukungan board ESP32 melalui Boards Manager.
3. Pastikan driver USB perangkat sudah tersedia.
4. Pilih board ESP32 yang sesuai dengan perangkat.

### 6.2 Library yang Digunakan

Program menggunakan library berikut:

- WiFi.h
- HTTPClient.h
- DHT.h
- OneWire.h
- DallasTemperature.h
- LiquidCrystal_I2C.h

Pastikan library yang diperlukan telah terpasang dan kompatibel dengan ESP32.

## 7. Cara Mengunggah Program ke ESP32

1. Unduh file `UTS_IoT_Klp06_P1_Code.ino` dari repository GitHub.
2. Buka file menggunakan Arduino IDE.
3. Periksa konfigurasi pin dan library yang digunakan.
4. Isi nama Wi-Fi, password Wi-Fi, dan Write API Key ThingSpeak pada salinan kode lokal.
5. Hubungkan ESP32 ke komputer menggunakan kabel USB.
6. Pilih board ESP32 yang sesuai melalui menu Tools.
7. Pilih port perangkat yang terdeteksi.
8. Tekan Verify untuk memeriksa program.
9. Tekan Upload untuk mengunggah program.
10. Setelah proses berhasil, buka Serial Monitor dengan baud rate 9600.
11. Periksa pembacaan sensor, status sistem, dan koneksi ThingSpeak.

**Catatan keamanan:** Jangan mengunggah password Wi-Fi atau Write API Key asli ke repository publik.

## 8. Mekanisme Kerja SIGAP BERAS

1. ESP32 melakukan inisialisasi sensor, LCD, LED, buzzer, tombol, dan koneksi Wi-Fi.
2. Sensor DHT22 membaca suhu dan kelembapan udara.
3. Sensor DS18B20 membaca suhu pada media beras.
4. Program menghitung selisih suhu media beras terhadap suhu udara.
5. Nilai kelembapan diolah menggunakan rata-rata bergerak maksimal tiga pembacaan terakhir.
6. Program menentukan status berdasarkan kelembapan, selisih suhu, dan durasi kondisi WASPADA.
7. LED dan buzzer memberikan respons sesuai status.
8. LCD dan Serial Monitor menampilkan hasil pemantauan.
9. Data dikirimkan ke ThingSpeak apabila koneksi internet tersedia.

Pembacaan sensor dijadwalkan setiap 2 detik, sedangkan pengiriman data ke ThingSpeak dijadwalkan setiap 20 detik.

## 9. Kriteria Status Sistem

| Status | Kondisi | Respons Sistem |
| --- | --- | --- |
| AMAN | RH ≤ 70% dan selisih suhu < 3°C | LED hijau menyala dan buzzer mati |
| WASPADA | RH > 70% hingga < 80%, atau selisih suhu ≥ 3°C hingga < 5°C | LED merah berkedip dan buzzer berbunyi pendek |
| KRITIS | RH ≥ 80%, selisih suhu ≥ 5°C, atau WASPADA berlangsung ≥ 30 detik | LED merah menyala terus dan buzzer berbunyi terus |
| ERROR | Sensor gagal dibaca sesuai logika program | LED berkedip bergantian dan buzzer mati |

Status sistem ditentukan berdasarkan tingkat kondisi paling serius yang terdeteksi.

Ambang tersebut merupakan parameter pengujian kelompok, bukan standar resmi penyimpanan beras.

## 10. Fungsi Push Button

### 10.1 Penekanan Singkat

Pada kondisi WASPADA atau KRITIS, tombol digunakan untuk mengonfirmasi alarm. Buzzer dapat disenyapkan selama 60 detik, sedangkan pemantauan sensor tetap berlangsung.

Pada kondisi AMAN, penekanan singkat dicatat sebagai kegiatan pemeriksaan rutin petugas.

### 10.2 Penekanan Lama

Penekanan tombol minimal 1 detik digunakan untuk mengganti halaman LCD antara tampilan data sensor dan informasi koneksi Wi-Fi.

## 11. Integrasi ThingSpeak

SIGAP BERAS menggunakan ThingSpeak sebagai platform pemantauan data berbasis cloud.

Data dikirimkan melalui jaringan Wi-Fi menggunakan permintaan HTTP menuju layanan ThingSpeak.

### 11.1 Susunan Field ThingSpeak

| Field | Data |
| --- | --- |
| Field 1 | Suhu udara (°C) |
| Field 2 | Kelembapan udara (%) |
| Field 3 | Suhu beras (°C) |
| Field 4 | Selisih suhu (°C) |
| Field 5 | Status sistem (0 = AMAN, 1 = WASPADA, 2 = KRITIS, 3 = ERROR) |
| Field 6 | Status buzzer senyap (0/1) |
| Field 7 | Kekuatan sinyal Wi-Fi (RSSI, dBm) |

Program juga dapat mengirimkan informasi kejadian atau alasan status melalui parameter status ThingSpeak.

### 11.2 Dashboard ThingSpeak

Dashboard pemantauan SIGAP BERAS dapat diakses melalui:

https://thingspeak.mathworks.com/channels/3561284

## 12. Pengujian Sistem

Pengujian SIGAP BERAS dilakukan untuk mengamati respons sistem terhadap kondisi AMAN, WASPADA, dan KRITIS.

Pengujian mencakup pembacaan sensor DHT22 dan DS18B20, perubahan status sistem, respons LED, buzzer, LCD, Serial Monitor, serta pengiriman data ke ThingSpeak.

Pengujian tombol dilakukan untuk mengetahui fungsi konfirmasi petugas dan pergantian halaman LCD.

Hasil pengujian ditunjukkan melalui dokumentasi Video Demo kelompok.

## 13. Dokumentasi Video Demo

Video Demo memperlihatkan prototipe SIGAP BERAS serta proses pengujian sistem pemantauan gudang beras.

**Video Demo YouTube:**

https://youtu.be/k6jcQdwA2E8

## 14. Source Code

Program utama SIGAP BERAS tersedia pada file:

`UTS_IoT_Klp06_P1_Code.ino`

Program dikembangkan menggunakan Arduino IDE untuk ESP32 dan mencakup pembacaan sensor, pengendalian indikator, pengelolaan alarm, tampilan LCD, serta pengiriman data ke ThingSpeak.

## 15. Identitas Kelompok

**Kelompok 6 – GUNUNG KAWI**

Mata Kuliah: Internet of Things (IoT)

Kelas: B

Program Studi: Teknik Logistik

Institusi: Institut Teknologi Kalimantan (ITK)

### Anggota Kelompok

1. Aulia Nur Rahmadani – 21241012
2. Puput Pebiyana – 21241045
3. Alif Dzaky Fauzan – 21241009
4. Heni Nabila Zahra – 21241026

---

Dokumentasi ini disusun sebagai bagian dari proyek Ujian Tengah Semester mata kuliah Internet of Things (IoT), Semester Gasal 2026/2027, Institut Teknologi Kalimantan.
