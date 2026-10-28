#include <WiFi.h>
#include <FirebaseESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= KREDENSIAL WIFI & FIREBASE =================
const char* ssid = "DEKSTOP 7";           
const char* password = "kamarakmal2";     

// PENTING: Untuk ESP32, FIREBASE_HOST jangan pakai "https://" dan jangan ada "/" di akhir
#define FIREBASE_HOST "monitoring-suhu-real-tim-90141-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyAsbqDriEzTmFRsvxOlZMreOJZdchnOGdw"

// ================= DEFINISI PIN HARDWARE =================
#define SENSOR_PIN 4
#define LED_HIJAU 16
#define LED_MERAH 17
#define BUZZER 18

OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
FirebaseData fbdo;
FirebaseData fbdoControl;
FirebaseAuth auth;
FirebaseConfig config;

// ================= VARIABEL SISTEM =================
float suhuRuangan = 0.0;
bool isMuted = false;
bool sensorTerdeteksi = true;

unsigned long waktuSebelumnya = 0;
const long intervalKedip = 500;
bool statusFlipFlop = LOW;

unsigned long waktuSebelumnyaCepat = 0;
const long intervalKedipCepat = 150;
bool statusFlipFlopCepat = LOW;

unsigned long waktuSebelumnyaSuperCepat = 0;
const long intervalKedipSuperCepat = 60; // 60ms untuk efek strobo sirine kilat
bool statusFlipFlopSuperCepat = LOW;

unsigned long waktuBacaSuhu = 0;
unsigned long waktuCekKontrol = 0;
unsigned long waktuSpamSerial = 0;
const long intervalSpam = 120; // Interval spam Serial Monitor saat suhu >= 60 C (120ms)

bool stateHijau = LOW;
bool stateMerah = LOW;
bool stateBuzzer = LOW;
int statusBahaya = 0; 

// ================= SETUP UTAMA =================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==================================================");
  Serial.println("  SISTEM MONITORING SUHU & ALARM KEBAKARAN ESP32  ");
  Serial.println("==================================================");
  
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  
  // Tes Indikator saat Booting
  digitalWrite(LED_HIJAU, HIGH);
  digitalWrite(LED_MERAH, HIGH);
  delay(300);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(LED_MERAH, LOW);

  sensors.begin();
  sensors.setResolution(10); // 10-bit: konversi sangat cepat (187ms) dan stabil
  sensors.setWaitForConversion(true); 
  sensors.requestTemperatures(); 
  
  // Hubungkan ke WiFi
  Serial.print("Menghubungkan ke WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int retryWiFi = 0;
  while (WiFi.status() != WL_CONNECTED && retryWiFi < 20) {
    delay(500);
    Serial.print(".");
    retryWiFi++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi Terhubung! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[PERINGATAN] Gagal terhubung ke WiFi, periksa SSID & Password!");
  }

  // ================= KONFIGURASI FIREBASE =================
  config.database_url = FIREBASE_HOST;
  config.api_key = FIREBASE_AUTH;

  // Gunakan Mode Test (Bypass otentikasi login anonim yang menyebabkan 'Bad request')
  config.signer.test_mode = true;

  // Alokasi buffer SSL BearSSL agar transmisi data stabil
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdoControl.setBSSLBufferSize(2048, 512);

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);

  Serial.println("[OK] Firebase Initialized (Mode Test). Siap streaming data!");
}

// ================= LOOP UTAMA =================
void loop() {
  unsigned long waktuSekarang = millis();
  
  // 1. Logika Kedip Flip-Flop
  if (waktuSekarang - waktuSebelumnya >= intervalKedip) {
    waktuSebelumnya = waktuSekarang;
    statusFlipFlop = !statusFlipFlop; 
  }
  
  if (waktuSekarang - waktuSebelumnyaCepat >= intervalKedipCepat) {
    waktuSebelumnyaCepat = waktuSekarang;
    statusFlipFlopCepat = !statusFlipFlopCepat; 
  }

  if (waktuSekarang - waktuSebelumnyaSuperCepat >= intervalKedipSuperCepat) {
    waktuSebelumnyaSuperCepat = waktuSekarang;
    statusFlipFlopSuperCepat = !statusFlipFlopSuperCepat; 
  }

  // 2. Pembacaan Suhu & Kirim ke Firebase (Setiap 1.2 Detik)
  if (waktuSekarang - waktuBacaSuhu >= 1200) {
    waktuBacaSuhu = waktuSekarang;
    
    // Minta konversi suhu DS18B20 (resolusi 10-bit selesai dalam 187ms)
    sensors.requestTemperatures(); 
    float tempC = sensors.getTempCByIndex(0);
    
    static int sensorGagalHitung = 0;
    bool readingValid = (tempC != DEVICE_DISCONNECTED_C && tempC > -50.0 && tempC < 125.0 && tempC != 85.0);

    if (readingValid) {
      suhuRuangan = tempC;
      sensorGagalHitung = 0;
      sensorTerdeteksi = true;
    } else {
      sensorGagalHitung++;
      // Toleransi 3x gagal membaca sebelum dinyatakan putus (mencegah false alarm akibat jitter 1-Wire)
      if (sensorGagalHitung >= 3) {
        sensorTerdeteksi = false;
        Serial.println(F("[SENSOR] DS18B20 tidak terdeteksi! Periksa kabel & resistor pull-up 4.7k."));
      }
    }
    
    // Otomatis reset status mute jika suhu sudah kembali normal di bawah 50°C
    if (suhuRuangan < 50.0 && isMuted == true) {
      isMuted = false;
      Firebase.setBool(fbdoControl, "/kontrol/is_muted", false);
      Serial.println("[INFO] Suhu aman (< 50 C). Alarm buzzer disiagakan kembali.");
    }

    // Hitung status kondisi bahaya sebelum dikirim ke Firebase
    if (suhuRuangan >= 60.0) {
      statusBahaya = 2;
    } else if (suhuRuangan >= 50.0) {
      statusBahaya = 1;
    } else {
      statusBahaya = 0;
    }

    // Kirim Data ke Firebase
    if (Firebase.ready()) {
      FirebaseJson json;
      json.set("suhu", suhuRuangan);
      json.set("bahaya", statusBahaya);
      json.set("hijau", suhuRuangan < 35.0);
      json.set("merah", suhuRuangan >= 35.0);
      json.set("terdeteksi", sensorTerdeteksi);
      json.set("uptime", (int)(millis() / 1000));
      
      if (Firebase.setJSON(fbdo, "/sensor", json)) {
        if (suhuRuangan < 60.0) {
          Serial.printf("[SUKSES] Suhu: %.1f C | Bahaya: %d | Terdeteksi: %s\n", 
                        suhuRuangan, statusBahaya, sensorTerdeteksi ? "YA" : "TIDAK");
        }
      } else {
        Serial.print("[ERROR FIREBASE] ");
        Serial.println(fbdo.errorReason());
      }
    } else {
      Serial.println("[FIREBASE] Menghubungkan sesi...");
    }
  }

  // 3. Cek Status Kontrol Mute dari Web secara Berkala (Setiap 2.5 Detik via fbdoControl)
  if (waktuSekarang - waktuCekKontrol >= 2500) {
    waktuCekKontrol = waktuSekarang;
    if (Firebase.ready()) {
      if (Firebase.getBool(fbdoControl, "/kontrol/is_muted")) {
        isMuted = fbdoControl.boolData();
      }
    }
  }

  // 3. Logika Kondisi Hardware Berdasarkan Suhu
  stateHijau = LOW;
  stateMerah = LOW;
  bool targetBuzzer = LOW;
  statusBahaya = 0; 

  if (suhuRuangan >= 60.0) {
    // ================= BAHAYA KRITIS (>= 60°C) - MODE SPAM PENUH =================
    stateMerah = HIGH;                      // Lampu Merah menyala terang penuh
    targetBuzzer = statusFlipFlopSuperCepat; // Buzzer menjerit strobo super cepat (60ms)
    statusBahaya = 2; 

    // Spam Peringatan Bahaya Penuh di Serial Monitor terus menerus setiap 120ms
    if (waktuSekarang - waktuSpamSerial >= intervalSpam) {
      waktuSpamSerial = waktuSekarang;
      Serial.println(F("🚨🚨🚨 [PERINGATAN BAHAYA: SUHU > 60°C TERDETEKSI!] 🚨🚨🚨"));
      Serial.print(F("🔥 [KRITIS] SUHU SAAT INI: "));
      Serial.print(suhuRuangan, 1);
      Serial.println(F(" °C - KEBAKARAN KRITIS SEDANG BERLANGSUNG!"));
      Serial.println(F("⚡ [AKSI] BUZZER MENJERIT PENUH & LED MERAH MAKSIMAL!"));
      Serial.println(F("=========================================================="));
    }
  }
  else if (suhuRuangan >= 50.0) {
    // Waspada Bahaya (50 - 59 C)
    stateMerah = HIGH;
    targetBuzzer = statusFlipFlop;
    statusBahaya = 1; 
  }
  else if (suhuRuangan >= 35.0) {
    // Hangat / Peringatan (35 - 49 C)
    stateMerah = statusFlipFlop;
    statusBahaya = 0; 
  } 
  else {
    // Suhu Normal / Aman (< 35 C)
    stateHijau = HIGH;
    statusBahaya = 0;
  }

  // Eksekusi Mute Buzzer
  if (isMuted) {
    stateBuzzer = LOW; 
  } else {
    stateBuzzer = targetBuzzer; 
  }

  // Tulis Status ke Pin Fisik ESP32
  digitalWrite(LED_HIJAU, stateHijau);
  digitalWrite(LED_MERAH, stateMerah);
  digitalWrite(BUZZER, stateBuzzer);
}
