/*
 * ================================================================
 *  RC Car ESP32 — WiFi Manager + MQTT
 *
 *  Sinkron dengan 4 controller:
 *    [1] web_kontrolv2.html   → Tombol browser
 *    [2] kontrol-copy.py      → Suara (speech recognition)
 *    [3] kontrol-ai-copy.py   → Suara + AI Gemini
 *    [4] kontrol-jari.py      → Gestur jari (MediaPipe)
 *
 *  Broker  : broker.hivemq.com
 *  Topic   : muhayara/rc/sh52s7a
 *  Perintah: F=maju | B=mundur | L=kiri | R=kanan | S=stop
 * ================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <MQTT.h>

// ================================================================
// KONFIGURASI
// ================================================================
const char* AP_SSID     = "Muhayara-RCCar";
const char* AP_PASSWORD = "rccar4321";
const char* MQTT_SERVER = "broker.hivemq.com";
const char* MQTT_TOPIC  = "muhayara/rc/sh52s7a";
// ================================================================


// ── Pin Motor ZK-5AD ─────────────────────────
// Kiri  maju  = D1(pin26) HIGH + D0(pin25) LOW
// Kiri  mundur= D0(pin25) HIGH + D1(pin26) LOW
// Kanan maju  = D3(pin32) HIGH + D2(pin27) LOW
// Kanan mundur= D2(pin27) HIGH + D3(pin32) LOW
const int D0 = 25;
const int D1 = 26;
const int D2 = 27;
const int D3 = 32;

// ── Objek ─────────────────────────────────────
WebServer   server(80);
Preferences prefs;
WiFiClient  espClient;
MQTTClient  mqttClient;

// ── State WiFi ────────────────────────────────
String savedSSID      = "";
String savedPass      = "";
bool   wifiConnected  = false;
bool   pendingConnect = false;
String pendingSSID    = "";
String pendingPass    = "";

// ── State scan async ──────────────────────────
// Scan tidak blocking — dimulai di handleScan()
// hasilnya dibaca di loop() lalu flag scanReady = true
bool scanReady    = false;  // hasil scan sudah siap dibaca
bool scanRunning  = false;  // scan sedang berjalan

// ================================================================
// MOTOR
// ================================================================
// KIRI (pin25/D0): HIGH=mundur, LOW=maju
// KANAN(pin32/D3): HIGH=mundur, LOW=maju
// ── Pin Motor ZK-5AD ─────────────────────────
// Kiri  maju  = D1(pin26) HIGH + D0(pin25) LOW
// Kiri  mundur= D0(pin25) HIGH + D1(pin26) LOW
// Kanan maju  = D3(pin32) HIGH + D2(pin27) LOW
// Kanan mundur= D2(pin27) HIGH + D3(pin32) LOW
void stopCar()  { digitalWrite(D0,LOW);  digitalWrite(D1,LOW);  digitalWrite(D2,LOW);  digitalWrite(D3,LOW);  }
void forward()  { digitalWrite(D0,LOW);  digitalWrite(D1,HIGH); digitalWrite(D2,HIGH);  digitalWrite(D3,LOW); }
void backward() { digitalWrite(D0,HIGH); digitalWrite(D1,LOW);  digitalWrite(D2,LOW); digitalWrite(D3,HIGH);  }
void left()     { digitalWrite(D0,LOW);  digitalWrite(D1,LOW); digitalWrite(D2,HIGH);  digitalWrite(D3,LOW);  }
void right()    { digitalWrite(D0,LOW);  digitalWrite(D1,HIGH);  digitalWrite(D2,LOW);  digitalWrite(D3,LOW); }

// ================================================================
// MQTT
// ================================================================
void onMessage(String &topic, String &payload) {
  Serial.println("[MQTT] Perintah: " + payload);
  if      (payload == "F") forward();
  else if (payload == "B") backward();
  else if (payload == "L") left();
  else if (payload == "R") right();
  else if (payload == "S") stopCar();
}

void reconnectMQTT() {
  if (!mqttClient.connected()) {
    String id = "RCCar-" + String(random(0xffff), HEX);
    if (mqttClient.connect(id.c_str())) {
      mqttClient.subscribe(MQTT_TOPIC);
      Serial.println("[MQTT] Terhubung!");
    }
  }
}

// ================================================================
// CSS
// ================================================================
const char CSS[] PROGMEM = R"CSS(
*{margin:0;padding:0;box-sizing:border-box}
body{background:#0a0a0a;color:#eee;font-family:'Segoe UI',sans-serif;padding:16px;min-height:100vh}
h1{text-align:center;color:#00e5ff;font-size:22px;letter-spacing:3px;padding:16px 0 4px}
.sub{text-align:center;color:#444;font-size:12px;margin-bottom:16px}
.card{background:#161616;border:1px solid #222;border-radius:14px;padding:14px 16px;margin-bottom:12px}
.card-label{font-size:10px;color:#444;letter-spacing:2px;text-transform:uppercase;margin-bottom:10px}
.status-row{display:flex;justify-content:space-between;align-items:center;font-size:13px}
.btn-forget{background:transparent;color:#ff4444;border:1px solid #ff4444;border-radius:8px;padding:5px 12px;font-size:11px;cursor:pointer}
.section-label{font-size:10px;color:#444;letter-spacing:2px;text-transform:uppercase;margin:16px 0 8px}
.wifi-item{display:flex;align-items:center;justify-content:space-between;padding:13px 14px;border-radius:12px;margin-bottom:6px;cursor:pointer;border:1px solid #1e1e1e;background:#111;transition:background 0.15s}
.wifi-item:hover{background:#1a1a1a;border-color:#333}
.wifi-item.saved{border-color:#00e5ff30;background:#00e5ff06}
.wifi-left{display:flex;align-items:center;gap:12px}
.bars{display:flex;align-items:flex-end;gap:2px;height:16px}
.bar{width:4px;border-radius:1px}
.wifi-info{display:flex;flex-direction:column;gap:3px}
.wifi-name{font-size:14px;color:#ddd}
.badge{font-size:10px;color:#00e5ff;background:#00e5ff18;border-radius:4px;padding:1px 6px;margin-left:6px}
.wifi-meta{font-size:11px;color:#444}
.arrow{color:#333;font-size:22px;line-height:1}
.modal-bg{display:none;position:fixed;inset:0;background:rgba(0,0,0,0.85);z-index:99;align-items:flex-end;justify-content:center}
.modal-bg.show{display:flex}
.modal{background:#181818;border:1px solid #2a2a2a;border-radius:20px 20px 0 0;padding:24px 20px 36px;width:100%;max-width:440px}
.modal h3{font-size:16px;margin-bottom:4px}
.modal-sub{font-size:12px;color:#555;margin-bottom:18px}
label{display:block;font-size:12px;color:#555;margin-bottom:6px;margin-top:14px}
input[type=password]{width:100%;padding:12px 14px;background:#111;border:1px solid #2a2a2a;border-radius:10px;color:white;font-size:15px;outline:none}
input:focus{border-color:#00e5ff}
.btn-ok{width:100%;padding:13px;margin-top:18px;background:#00e5ff;color:#000;font-size:15px;font-weight:bold;border:none;border-radius:12px;cursor:pointer}
.btn-cancel{width:100%;padding:11px;margin-top:8px;background:transparent;color:#555;font-size:13px;border:1px solid #222;border-radius:12px;cursor:pointer}
.alert-ok{padding:12px 16px;border-radius:10px;font-size:13px;margin-bottom:12px;background:rgba(0,200,100,0.08);border:1px solid #00c864;color:#00c864}
.alert-err{padding:12px 16px;border-radius:10px;font-size:13px;margin-bottom:12px;background:rgba(255,68,68,0.08);border:1px solid #ff4444;color:#ff4444}
.btn-scan{width:100%;padding:10px;background:transparent;color:#00e5ff;border:1px solid #00e5ff22;border-radius:10px;font-size:13px;cursor:pointer;margin-bottom:10px}
.btn-scan:disabled{color:#444;border-color:#222;cursor:not-allowed}
.empty{color:#444;text-align:center;font-size:13px;padding:24px}
.scanning{color:#00e5ff88;text-align:center;font-size:13px;padding:20px}
.chip{display:inline-block;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:bold}
.chip-on{background:#00e5ff18;color:#00e5ff;border:1px solid #00e5ff33}
.chip-off{background:#ff444418;color:#ff6666;border:1px solid #ff444433}
)CSS";

// ================================================================
// BUILD HTML LIST DARI HASIL SCAN
// ================================================================
String buildWifiList() {
  if (scanRunning) {
    return "<p class=\"scanning\">&#128268; Scanning... mohon tunggu</p>";
  }
  if (!scanReady) {
    return "<p class=\"empty\">Tekan <b>Scan Ulang</b> untuk mencari jaringan WiFi.</p>";
  }

  int n = WiFi.scanComplete();
  if (n <= 0) {
    return "<p class=\"empty\">Tidak ada jaringan ditemukan.</p>";
  }

  String html = "";
  for (int i = 0; i < n; i++) {
    String ssid   = WiFi.SSID(i);
    int    rssi   = WiFi.RSSI(i);
    bool   locked = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    bool   saved  = (ssid == savedSSID);

    if (ssid.length() == 0) continue;

    int bars; String bc;
    if      (rssi > -55) { bars = 4; bc = "#00e5ff"; }
    else if (rssi > -67) { bars = 3; bc = "#00e5ff"; }
    else if (rssi > -78) { bars = 2; bc = "#ffaa00"; }
    else                 { bars = 1; bc = "#ff4444"; }

    html += "<div class=\"wifi-item" + String(saved?" saved":"") + "\" onclick=\"selectWifi('" + ssid + "')\">";
    html += "<div class=\"wifi-left\"><div class=\"bars\">";
    for (int b = 1; b <= 4; b++)
      html += "<div class=\"bar\" style=\"height:" + String(b*4) + "px;background:" + bc + ";opacity:" + (b<=bars?"1":"0.2") + "\"></div>";
    html += "</div><div class=\"wifi-info\"><span class=\"wifi-name\">" + ssid;
    if (saved) html += " <span class=\"badge\">Aktif</span>";
    html += "</span><span class=\"wifi-meta\">" + String(rssi) + " dBm &nbsp;" + (locked?"&#128274;":"&#128275;") + "</span>";
    html += "</div></div><span class=\"arrow\">&#8250;</span></div>";
  }
  return html;
}

// ================================================================
// BUILD HALAMAN PORTAL
// ================================================================
String buildPage(String alertClass = "", String alertMsg = "") {
  String chip = wifiConnected
    ? "<span class=\"chip chip-on\">&#9679;Online</span>"
    : "<span class=\"chip chip-off\">&#9679;Offline</span>";
  String wifiStatus = (savedSSID != "")
    ? "Tersimpan: <b style=\"color:#00e5ff\">" + savedSSID + "</b>"
    : "<span style=\"color:#555\">Belum ada WiFi tersimpan</span>";

  String h = "<!DOCTYPE html><html lang=\"id\"><head>";
  h += "<meta charset=\"UTF-8\">";
  h += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  h += "<title>RC Car WiFi</title><style>"; h += CSS; h += "</style></head><body>";
  h += "<h1>RC CAR</h1><p class=\"sub\">WiFi Manager &mdash;192.168.4.1</p>";

  if (alertClass != "") h += "<div class=\"" + alertClass + "\">" + alertMsg + "</div>";

  h += "<div class=\"card\"><div class=\"card-label\">Status</div>";
  h += "<div class=\"status-row\" style=\"margin-bottom:10px\">";
  h += "<span style=\"font-size:12px;color:#555\">Internet</span>" + chip;
  h += "</div><div class=\"status-row\"><span>" + wifiStatus + "</span>";
  if (savedSSID != "") {
    h += "<form action=\"/reset\" method=\"POST\" style=\"display:inline\">";
    h += "<button type=\"submit\" class=\"btn-forget\">Lupakan</button></form>";
  }
  h += "</div></div>";
  h += "<div class=\"section-label\">Jaringan WiFi Tersedia</div>";

  // Tombol scan — klik → POST /scan → tampil halaman scanning → auto reload
  String scanLabel = scanRunning ? "Scanning..." : "&#8635; Scan Ulang";
  h += "<form action=\"/scan\" method=\"POST\" style=\"margin-bottom:10px\">";
  h += "<button type=\"submit\" class=\"btn-scan\"" + String(scanRunning?" disabled":"") + ">" + scanLabel + "</button>";
  h += "</form>";

  h += buildWifiList();

  // Modal password
  h += "<div class=\"modal-bg\" id=\"modalBg\"><div class=\"modal\">";
  h += "<h3 id=\"modalTitle\"></h3>";
  h += "<p class=\"modal-sub\">Masukkan password untuk terhubung</p>";
  h += "<form action=\"/simpan\" method=\"POST\">";
  h += "<input type=\"hidden\" name=\"ssid\" id=\"hiddenSSID\">";
  h += "<label>Password WiFi</label>";
  h += "<input type=\"password\" name=\"password\" placeholder=\"Kosongkan jika open network\" autocomplete=\"off\">";
  h += "<button type=\"submit\" class=\"btn-ok\">Hubungkan</button>";
  h += "</form>";
  h += "<button class=\"btn-cancel\" onclick=\"closeModal()\">Batal</button>";
  h += "</div></div>";

  h += "<script>";
  h += "function selectWifi(s){document.getElementById('modalTitle').textContent=s;document.getElementById('hiddenSSID').value=s;document.getElementById('modalBg').classList.add('show');}";
  h += "function closeModal(){document.getElementById('modalBg').classList.remove('show');}";
  h += "document.getElementById('modalBg').addEventListener('click',function(e){if(e.target===this)closeModal();});";
  h += "</script></body></html>";
  return h;
}

// ── Halaman "sedang scanning" — auto reload ───
String buildScanningPage() {
  String h = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  h += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  // Auto reload ke / setelah 4 detik (cukup waktu scan selesai)
  h += "<meta http-equiv=\"refresh\" content=\"4;url=/\">";
  h += "<title>Scanning...</title>";
  h += "<style>*{margin:0;padding:0;box-sizing:border-box}body{background:#0a0a0a;color:#eee;font-family:'Segoe UI',sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;flex-direction:column;gap:20px;padding:20px;text-align:center}h1{color:#00e5ff;font-size:20px;letter-spacing:3px}.sp{width:48px;height:48px;border:4px solid #222;border-top:4px solid #00e5ff;border-radius:50%;animation:spin .8s linear infinite}@keyframes spin{to{transform:rotate(360deg)}}p{color:#555;font-size:13px}</style>";
  h += "</head><body><h1>RC CAR</h1><div class=\"sp\"></div>";
  h += "<p>Scanning jaringan WiFi...<br>Halaman otomatis diperbarui</p>";
  h += "</body></html>";
  return h;
}

// ── Halaman loading konek ─────────────────────
String buildLoading(String ssid) {
  String h = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
  h += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  h += "<meta http-equiv=\"refresh\" content=\"4;url=/status\">";
  h += "<title>Menghubungkan...</title>";
  h += "<style>*{margin:0;padding:0;box-sizing:border-box}body{background:#0a0a0a;color:#eee;font-family:'Segoe UI',sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;flex-direction:column;gap:20px;padding:20px;text-align:center}h1{color:#00e5ff;font-size:20px;letter-spacing:3px}.sp{width:48px;height:48px;border:4px solid #222;border-top:4px solid #00e5ff;border-radius:50%;animation:spin .8s linear infinite}@keyframes spin{to{transform:rotate(360deg)}}.nm{color:#fff;font-size:16px;font-weight:bold}p{color:#555;font-size:13px}</style>";
  h += "</head><body><h1>RC CAR</h1><div class=\"sp\"></div>";
  h += "<div class=\"nm\">" + ssid + "</div>";
  h += "<p>Sedang menghubungkan...<br>Halaman otomatis diperbarui</p>";
  h += "</body></html>";
  return h;
}

// ================================================================
// HANDLER
// ================================================================
void handleRoot() {
  server.send(200, "text/html", buildPage());
}

void handleScan() {
  // Langsung mulai scan async — TIDAK blocking, server tetap jalan
  WiFi.scanDelete();
  WiFi.scanNetworks(true); // true = async
  scanRunning = true;
  scanReady   = false;
  Serial.println("[Scan] Scan dimulai (async)...");

  // Tampilkan halaman "scanning..." dengan auto-reload 4 detik
  server.send(200, "text/html", buildScanningPage());
}

void handleStatus() {
  if (wifiConnected) {
    String h = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
    h += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    h += "<title>Terhubung!</title>";
    h += "<style>*{margin:0;padding:0;box-sizing:border-box}body{background:#0a0a0a;color:#eee;font-family:'Segoe UI',sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;flex-direction:column;gap:16px;padding:20px;text-align:center}h1{color:#00e5ff;font-size:20px;letter-spacing:3px}.ic{font-size:64px}.mg{color:#00c864;font-size:18px;font-weight:bold}p{color:#555;font-size:13px;line-height:1.7}.btn{display:inline-block;margin-top:8px;padding:12px 28px;background:#00e5ff;color:#000;font-size:14px;font-weight:bold;border-radius:12px;text-decoration:none;border:none;cursor:pointer}</style>";
    h += "</head><body><h1>RC CAR</h1><div class=\"ic\">&#9989;</div>";
    h += "<div class=\"mg\">WiFi Terhubung!</div>";
    h += "<p>RC Car online &amp; siap dikendalikan.<br>Hotspot <b>Muhayara-RCCar</b> tetap aktif.</p>";
    h += "<a class=\"btn\" href=\"/\">Kembali ke Portal</a>";
    h += "</body></html>";
    server.send(200, "text/html", h);
  } else if (pendingConnect) {
    server.sendHeader("Refresh", "3;url=/status");
    server.send(200, "text/html", buildLoading(pendingSSID));
  } else {
    server.send(200, "text/html", buildPage("alert-err",
      "Gagal terhubung ke <b>" + pendingSSID + "</b>. Cek password atau sinyal."));
  }
}

void handleSimpan() {
  String ssid = server.arg("ssid");
  String pass = server.arg("password");
  if (ssid.isEmpty()) {
    server.send(200, "text/html", buildPage("alert-err", "SSID tidak boleh kosong!"));
    return;
  }
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  savedSSID = ssid; savedPass = pass;
  pendingSSID = ssid; pendingPass = pass;
  pendingConnect = true; wifiConnected = false;
  Serial.println("[WiFi] Antri konek ke: " + ssid);
  server.send(200, "text/html", buildLoading(ssid));
}

void handleReset() {
  prefs.begin("wifi", false); prefs.clear(); prefs.end();
  savedSSID = ""; savedPass = "";
  pendingSSID = ""; pendingPass = "";
  wifiConnected = false;
  WiFi.disconnect();
  scanReady   = false;
  scanRunning = false;
  WiFi.scanDelete();
  server.send(200, "text/html", buildPage("alert-ok", "WiFi dilupakan! Pilih jaringan baru."));
  Serial.println("[WiFi] Dilupakan.");
}

// ================================================================
// LOAD WIFI TERSIMPAN
// ================================================================
bool loadSavedWifi() {
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  prefs.end();
  if (savedSSID == "") return false;

  Serial.println("[WiFi] Mencoba tersimpan: " + savedSSID);
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500); Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Terhubung! IP: " + WiFi.localIP().toString());
    return true;
  }
  Serial.println("\n[WiFi] Gagal.");
  return false;
}

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n============================");
  Serial.println(" RC Car ESP32 Boot");
  Serial.println("============================");

  pinMode(D0,OUTPUT); pinMode(D1,OUTPUT); pinMode(D2,OUTPUT); pinMode(D3,OUTPUT);
  stopCar();


  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500);
  Serial.println("[AP] Hotspot : " + String(AP_SSID));
  Serial.println("[AP] IP      : " + WiFi.softAPIP().toString());

  server.on("/",       HTTP_GET,  handleRoot);
  server.on("/scan",   HTTP_POST, handleScan);   // POST agar tidak kena GET reload
  server.on("/simpan", HTTP_POST, handleSimpan);
  server.on("/reset",  HTTP_POST, handleReset);
  server.on("/status", HTTP_GET,  handleStatus);
  server.begin();
  Serial.println("[Web] Portal  : http://192.168.4.1");

  if (loadSavedWifi()) {
    wifiConnected = true;
    mqttClient.begin(MQTT_SERVER, espClient);
    mqttClient.onMessage(onMessage);
    Serial.println("[RC]  Siap dikendalikan!");
    Serial.println("[MQTT] Topic : " + String(MQTT_TOPIC));
  } else {
    Serial.println("[RC]  Belum ada WiFi. Buka http://192.168.4.1");
  }
}

// ================================================================
// LOOP — ringan, cek hasil scan async di sini
// ================================================================
void loop() {
  server.handleClient();

  // ── Cek hasil scan async ──────────────────
  if (scanRunning) {
    int result = WiFi.scanComplete();
    if (result >= 0) {
      // Scan selesai
      scanRunning = false;
      scanReady   = true;
      Serial.println("[Scan] Selesai: " + String(result) + " jaringan ditemukan");
    }
    // result == WIFI_SCAN_RUNNING (-1) → masih jalan, tunggu loop berikutnya
  }

  // ── Proses koneksi WiFi baru ──────────────
  if (pendingConnect) {
    pendingConnect = false;
    Serial.println("[WiFi] Konek ke: " + pendingSSID);
    WiFi.begin(pendingSSID.c_str(), pendingPass.c_str());
    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
      delay(500); Serial.print(".");
      server.handleClient();
    }
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("\n[WiFi] Terhubung: " + WiFi.localIP().toString());
      mqttClient.begin(MQTT_SERVER, espClient);
      mqttClient.onMessage(onMessage);
      Serial.println("[RC]  Siap dikendalikan!");
    } else {
      Serial.println("\n[WiFi] Gagal konek!");
      prefs.begin("wifi", false); prefs.clear(); prefs.end();
      savedSSID = ""; savedPass = ""; pendingSSID = "";
    }
  }

  // ── MQTT ──────────────────────────────────
  if (wifiConnected) {
    if (!mqttClient.connected()) reconnectMQTT();
    mqttClient.loop();
  }
}
