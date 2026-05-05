# 🚗 RC Car ESP32

Mobil RC berbasis ESP32 dengan kontrol via MQTT. Mendukung 4 mode kontrol:
- **Web Browser** (`web_kontrolv2.html`)
- **Suara** (`kontrol-copy.py`)
- **Suara + AI Gemini** (`kontrol-ai-copy.py`)
- **Gestur Jari MediaPipe** (`kontrol-jari.py`)

## Hardware
- ESP32
- Motor Driver ZK-5AD
- 2x Motor DC

## Wiring
| ESP32 Pin | ZK-5AD |
|-----------|--------|
| 25 | D0 (kiri mundur) |
| 26 | D1 (kiri maju) |
| 27 | D2 (kanan mundur) |
| 32 | D3 (kanan maju) |

## MQTT
- **Broker:** `broker.hivemq.com`
- **Topic:** `muhayara/rc/whatever-you-want`
- **Perintah:** `F` maju · `B` mundur · `L` kiri · `R` kanan · `S` stop

## Setup
1. Install [PlatformIO](https://platformio.org/)
2. Clone repo ini
3. Buka di VS Code + PlatformIO
4. Upload ke ESP32
5. Hubungkan ke WiFi lewat hotspot `Muhayara-RCCar` (password: `rccar4321`)
6. Buka `http://192.168.4.1` di browser
