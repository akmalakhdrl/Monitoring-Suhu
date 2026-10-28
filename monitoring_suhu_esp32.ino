#include <WiFi.h>
#include <FirebaseESP32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ================= KREDENSIAL WIFI & FIREBASE =================
const char* ssid = "DEKSTOP 7";           
const char* password = "kamarakmal2";     

#define FIREBASE_HOST "https://monitoring-suhu-real-tim-90141-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "AIzaSyAsbqDriEzTmFRsvxOlZMreOJZdchnOGdw"

// ================= DEFINISI PIN HARDWARE =================
#define SENSOR_PIN 4
#define LED_HIJAU 16
#define LED_MERAH 17
#define BUZZER 18

OneWire oneWire(SENSOR_PIN);
DallasTemperature sensors(&oneWire);
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ================= VARIABEL SISTEM =================
float suhuRuangan = 0.0;
bool isMuted = false;

unsigned long waktuSebelumnya = 0;
const long intervalKedip = 500;
bool statusFlipFlop = LOW;

unsigned long waktuSebelumnyaCepat = 0;
const long intervalKedipCepat = 150;
bool statusFlipFlopCepat = LOW;

unsigned long waktuBacaSuhu = 0;

bool stateHijau = LOW;
bool stateMerah = LOW;
bool stateBuzzer = LOW;
int statusBahaya = 0; 

// ================= SETUP UTAMA =================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Sistem Monitoring Suhu & Kebakaran ESP32 Dimulai ---");
  
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
  sensors.setWaitForConversion(false); 
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

  // Konfigurasi Firebase
  config.api_key = FIREBASE_AUTH;
  config.database_url = FIREBASE_HOST;

  // Mode Test / Anonymous Sign-in
  config.signer.test_mode = true;
  Firebase.signUp(&config, &auth, "", "");

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);

  // Inisialisasi awal nilai kontrol jika belum ada
  if (!Firebase.getBool(fbdo, "/kontrol/is_muted")) {
    Firebase.setBool(fbdo, "/kontrol/is_muted", false);
  }
  
  Serial.println("[OK] Firebase Initialized. Siap mengirim data ke Web Dashboard!");
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

  // 2. Pembacaan Suhu & Kirim ke Firebase (Setiap 1 Detik)
  if (waktuSekarang - waktuBacaSuhu >= 1000) {
    waktuBacaSuhu = waktuSekarang;
    
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C && tempC > -55.0 && tempC < 125.0) {
      suhuRuangan = tempC;
    } else {
      Serial.println("[PERINGATAN] Sensor DS18B20 tidak terbaca / terlepas!");
    }
    sensors.requestTemperatures(); 
    
    // Otomatis reset status mute jika suhu sudah kembali normal di bawah 50°C
    if (suhuRuangan < 50.0 && isMuted == true) {
      isMuted = false;
      Firebase.setBool(fbdo, "/kontrol/is_muted", false);
      Serial.println("Suhu sudah aman (< 50 C). Alarm buzzer disiagakan kembali.");
    }

    // Kirim Data Lengkap ke Firebase Realtime Database
    FirebaseJson json;
    json.set("suhu", suhuRuangan);
    json.set("bahaya", statusBahaya);
    json.set("hijau", stateHijau);
    json.set("merah", stateMerah);
    
    if (Firebase.setJSON(fbdo, "/sensor", json)) {
      Serial.printf("[TERKIRIM KE WEB] Suhu: %.1f C | Bahaya: %d | LED Hijau: %d | LED Merah: %d\n", 
                    suhuRuangan, statusBahaya, stateHijau, stateMerah);
    } else {
      Serial.print("[ERROR FIREBASE] ");
      Serial.println(fbdo.errorReason());
    }
    
    // Baca Status Mute dari Web
    if (Firebase.getBool(fbdo, "/kontrol/is_muted")) {
      isMuted = fbdo.boolData();
    }
  }

  // 3. Logika Kondisi Hardware Berdasarkan Suhu
  stateHijau = LOW;
  stateMerah = LOW;
  bool targetBuzzer = LOW;
  statusBahaya = 0; 

  if (suhuRuangan >= 60.0) {
    // Bahaya Kebakaran Kritis
    stateMerah = HIGH;
    targetBuzzer = statusFlipFlopCepat;
    statusBahaya = 2; 
  }
  else if (suhuRuangan >= 50.0) {
    // Waspada Bahaya
    stateMerah = HIGH;
    targetBuzzer = statusFlipFlop;
    statusBahaya = 1; 
  }
  else if (suhuRuangan >= 35.0) {
    // Hangat / Peringatan
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

