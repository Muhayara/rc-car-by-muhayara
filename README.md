# 🚗 RC Car ESP32 by Muhayara

Mobil RC berbasis ESP32 dengan kontrol via MQTT. Mendukung 4 mode kontrol:
- 🌐 **Web Browser** (`web_kontrolv2.html`)
- 🎤 **Suara** (`kontrol-copy.py`)
- 🤖 **Suara + AI Gemini** (`kontrol-ai-copy.py`)
- ✋ **Gestur Jari MediaPipe** (`kontrol-jari.py`)

---

## 📦 Hardware
- ESP32
- Motor Driver ZK-5AD
- 4x Motor DC

## 🔌 Wiring
| ESP32 Pin | ZK-5AD | Fungsi |
|-----------|--------|--------|
| 25 | D0 | Kiri mundur |
| 26 | D1 | Kiri maju |
| 27 | D2 | Kanan mundur |
| 32 | D3 | Kanan maju |

## 📡 MQTT
- **Broker:** `broker.hivemq.com`
- **Topic:** `muhayara/rc/whatever-you-want`
- **Perintah:** `F` maju · `B` mundur · `L` kiri · `R` kanan · `S` stop

---

## 🛠️ Setup ESP32 (Firmware)

### Cara 1 — PlatformIO (Rekomendasi)
1. Install [VS Code](https://code.visualstudio.com/)
2. Install extension **PlatformIO IDE** di VS Code
3. Clone repo ini:
   ```bash
   git clone https://github.com/Muhayara/rc-car-by-muhayara.git
   cd rc-car-by-muhayara
   ```
4. Buka folder di VS Code
5. PlatformIO otomatis install dependencies
6. Klik tombol **Upload** (→) di toolbar bawah VS Code

### Cara 2 — Arduino IDE
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Tambah board ESP32 — buka **File → Preferences**, tambahkan URL ini di *Additional Board Manager URLs*:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Buka **Tools → Board → Board Manager**, cari `esp32`, install
4. Install library yang dibutuhkan via **Tools → Manage Libraries**:
   - `MQTT` by Joel Gaehwiler
5. Buka file `src/main.cpp`, copy semua isinya ke sketch baru di Arduino IDE
6. Pilih board **ESP32 Dev Module** dan port yang sesuai
7. Klik **Upload**

---

## 🌐 Setup Web Controller (`web_kontrolv2.html`)
Tidak perlu install apapun, langsung buka file `web_kontrolv2.html` di browser — Chrome/Firefox/Safari semua bisa.

Pastikan device kamu terhubung ke internet agar bisa konek ke broker MQTT.

---

## 🎤 Setup Kontrol Suara & AI (`kontrol-copy.py` / `kontrol-ai-copy.py`)

### Requirements
- Python 3.8+

### Install dependencies
```bash
pip install paho-mqtt SpeechRecognition pyaudio google-generativeai
```

> **Catatan untuk Linux:** Kalau `pyaudio` gagal install, jalankan dulu:
> ```bash
> sudo apt install portaudio19-dev  # Ubuntu/Debian
> sudo pacman -S portaudio          # Arch Linux
> ```

### Jalankan
```bash
python kontrol-copy.py
# atau untuk versi AI
python kontrol-ai-copy.py
```

---

## ✋ Setup Kontrol Gestur Jari (`kontrol-jari.py`)

### Requirements
- Python 3.8+
- Webcam

### Install dependencies
```bash
pip install paho-mqtt mediapipe opencv-python
```

### Jalankan
```bash
python kontrol-jari.py
```

---

## 🚀 First Time Setup Mobil RC
1. Upload firmware ke ESP32
2. ESP32 akan membuat hotspot WiFi: **`Muhayara-RCCar`** (password: `rccar4321`)
3. Hubungkan HP/laptop ke hotspot tersebut
4. Buka browser, akses **`http://192.168.4.1`**
5. Pilih WiFi rumah kamu dan masukkan password
6. ESP32 akan konek ke internet dan siap dikendalikan via MQTT