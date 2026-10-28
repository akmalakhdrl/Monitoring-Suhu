# 🌡️ IoT Temperature & Fire Alarm Monitoring System (ESP32 + Firebase)

Sistem pemantauan suhu dan deteksi dini kebakaran berbasis **Internet of Things (IoT)** menggunakan mikrokontroler **ESP32**, sensor suhu **DS18B20**, **Firebase Realtime Database**, dan **Web Dashboard Interaktif**.

![Dashboard Preview](https://img.shields.io/badge/Status-Active-brightgreen)
![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Firebase](https://img.shields.io/badge/Database-Firebase_RTDB-orange)
![License](https://img.shields.io/badge/License-MIT-green)

---

## 📌 Fitur Utama

- 📊 **Real-time Monitoring**: Pembaruan data suhu setiap 1 detik secara live via Firebase Realtime Database.
- 🚨 **Sistem Peringatan Bertingkat**:
  - **Normal (< 35 °C)**: Status AMAN, LED Hijau menyala.
  - **Waspada (35 °C - 49.9 °C)**: Status SIAGA, LED Merah berkedip lambat (500 ms).
  - **Bahaya Api (50 °C - 59.9 °C)**: Status BAHAYA, LED Merah menyala, Buzzer berbunyi (500 ms), Banner Darurat aktif.
  - **Bahaya Kritis (≥ 60 °C)**: Status BAHAYA KRITIS, LED Merah menyala, Buzzer berbunyi cepat (150 ms), Efek peringatan api.
- 🔕 **Kontrol Mute Buzzer 2-Arah**: Tombol mute/unmute dari Web Dashboard yang langsung tersinkronisasi ke hardware ESP32. Jika suhu kembali turun di bawah 50 °C, buzzer otomatis disiagakan kembali.
- 📈 **Grafik Tren Suhu (Chart.js)**: Memantau riwayat fluktuasi suhu dalam bentuk kurva realtime dengan indikator warna dinamis.
- 📝 **Log Aktivitas**: Pencatatan riwayat kejadian beserta timestamp secara otomatis.
- 🧪 **Fitur Simulasi Data**: Pengujian tampilan web tanpa perlu menyalakan hardware ESP32.

---

## 🔌 Skema Rangkaian Hardware (Pinout ESP32)

| Komponen | Pin ESP32 | Keterangan |
| :--- | :--- | :--- |
| **Sensor DS18B20 (Data)** | **GPIO 4** | Membutuhkan resistor pull-up 4.7kΩ ke 3.3V |
| **LED Hijau (Indikator Aman)** | **GPIO 16** | Hubungkan ke resistor 220Ω |
| **LED Merah (Indikator Bahaya)** | **GPIO 17** | Hubungkan ke resistor 220Ω |
| **Buzzer Aktif 5V** | **GPIO 18** | Kutub positif ke GPIO 18, negatif ke GND |
| **Power Sensor & Buzzer** | **3.3V / 5V & GND** | Hubungkan ke VCC & GND ESP32 |

---

## 📂 Struktur File Repository

```text
├── index.html                  # Web Dashboard utama (Monitoring & Kontrol)
├── monitoring_suhu_esp32.ino   # Program Arduino IDE untuk ESP32
├── README.md                   # Dokumentasi lengkap proyek
└── .gitignore                  # File pengabaian git
```

---

## 🚀 Panduan Instalasi & Penggunaan

### 1. Persiapan Arduino IDE (ESP32)
1. Pasang board ESP32 di Arduino IDE melalui *Boards Manager* (`esp32 by Espressif`).
2. Pasang library berikut melalui *Library Manager*:
   - **Firebase ESP32 Client** (oleh Mobizt)
   - **DallasTemperature** (oleh Miles Burton)
   - **OneWire** (oleh Paul Stoffregen)
3. Buka file `monitoring_suhu_esp32.ino`.
4. Sesuaikan kredensial WiFi Anda:
   ```cpp
   const char* ssid = "NAMA_WIFI_ANDA";
   const char* password = "PASSWORD_WIFI";
   ```
5. Pilih board **ESP32 Dev Module** dan Port COM yang sesuai, lalu klik **Upload**.

### 2. Membuka Web Dashboard
1. Cukup klik ganda file **`index.html`** untuk membukanya di browser (Google Chrome, Edge, Safari, dll).
2. Anda juga dapat mengaktifkan **GitHub Pages** di repository ini pada menu *Settings > Pages* agar dashboard dapat diakses secara online dari mana saja.

---

## 📄 Lisensi
Proyek ini dibuat untuk keperluan monitoring suhu dan peringatan kebakaran IoT. Bebas dikembangkan dan dimodifikasi untuk tujuan edukasi dan komersial.

