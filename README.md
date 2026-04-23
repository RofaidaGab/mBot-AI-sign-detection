# mBot AI Sign Detection

An autonomous robot car that detects traffic signs in real time using a trained TensorFlow/Keras model and responds with physical driving actions. Built for the **Intro to Computer & Electrical Engineering** course (first term project) at Istanbul Medipol University.

---

## How It Works

The system is split across three components that communicate over Wi-Fi and UART:

```
[ESP32-CAM] ---(MJPEG stream)---> [Python on laptop]
                                       |
                              TFLite model inference
                                       |
                              detected sign command
                                       |
[ESP32-CAM] <---(Wi-Fi TCP)--- [Python on laptop]
     |
  (UART TX/RX)
     |
[mBot Arduino]  -->  motor actions
```

1. The **ESP32-CAM** (mounted on the mBot) connects to a Wi-Fi hotspot, streams live MJPEG video, and listens for text commands over TCP.
2. A **Python script** on the laptop fetches the stream, runs each frame through a TFLite sign detection model, and sends the detected sign name back to the ESP32-CAM over Wi-Fi.
3. The **ESP32-CAM** forwards the command to the **mBot's Arduino** via UART (TX/RX), which then executes the corresponding motor action.

---

## Signs Detected

| Sign | mBot Action |
|---|---|
| **Start** | Begin line following |
| **Stop** | Stop all motors |
| **C-Road** | Execute a turn based on detected circle diameter |
| **C-Road End** | Resume line following |

> **Note:** C-Road and C-Road End signs are detected by the model but were not used in the final demo. The car relies on fixed diameter thresholds (30 and 100 px) for turns and on the onboard line-following sensors. You can enable C-Road sign control by modifying the Python command logic.

---

## Repository Structure

| File | Description |
|---|---|
| `mbotComm.ino` | ESP32-CAM firmware — sets up Wi-Fi, camera stream, TCP server, and UART relay to Arduino |
| `testmBotCommunication.py` | Python script — fetches stream, runs TFLite inference, sends commands back to ESP32 |
| `traffic_sign_model_final.keras` | Trained Keras model (for retraining / evaluation) |
| `traffic_sign_model_final.tflite` | Quantised TFLite model used for real-time inference |
| `app_httpd.cpp` | ESP32 camera HTTP server (handles MJPEG streaming) |
| `camera_pins.h` | GPIO pin definitions for AI-Thinker ESP32-CAM board |
| `camera_index.h` | Embedded HTML for the camera stream web UI |
| `ci.json` | Arduino board/library configuration |
| `partitions.csv` | ESP32 flash partition table |
| `testarduinopart/` | Standalone Arduino test sketches used during development |

---

## Setup & Usage

### Requirements

- Makeblock mBot (with Arduino Uno)
- AI-Thinker ESP32-CAM module
- Python 3.x with `tensorflow`, `opencv-python`, `numpy`, `requests`
- Arduino IDE with **Memcore** and **ESP32** board support installed

### Steps

**1. Flash the ESP32-CAM**

Open `mbotComm.ino` in Arduino IDE. Update the Wi-Fi credentials:

```cpp
const char* ssid     = "YourHotspotName";
const char* password = "YourPassword";
```

Select board: `AI Thinker ESP32-CAM`. Upload using a USB-UART adapter (GPIO0 to GND during flash).

**2. Connect everything**

- Wire ESP32-CAM TX → mBot Arduino RX, and ESP32-CAM RX → mBot Arduino TX
- Power on the mBot and ESP32-CAM
- Connect your laptop to **the same hotspot** the ESP32 is configured to join

**3. Get the IP address**

Open Serial Monitor (115200 baud) after powering on. The ESP32 will print its assigned IP:

```
WiFi connected
IP Address: 192.168.x.x
```

**4. Run the Python script**

```bash
python testmBotCommunication.py
```

Update the IP address in the script to match what was printed above. The script will open the camera stream, run inference on each frame, and start sending commands automatically.

> ⚠️ The IP address changes depending on which hotspot you connect to. Always check Serial Monitor after connecting to a new network.

---

## Model

The sign classifier was trained using **TensorFlow/Keras** on a custom dataset of the four sign classes. The model was then converted to **TFLite** format for efficient inference on the laptop CPU during live streaming.

- Input: cropped/resized frame from ESP32-CAM MJPEG stream
- Output: class probabilities for `[start, stop, c-road, c-road-end]`
- Inference runs on the laptop, not on the microcontroller

---

## Known Limitations

- The IP address is dynamic — it must be updated manually in the Python script each session when switching hotspots
- C-Road turning currently uses fixed pixel-diameter thresholds rather than the sign detection output
- Stream latency depends on hotspot quality and distance

---

## Tech Stack

`Python` `TensorFlow / Keras` `TFLite` `OpenCV` `ESP32-CAM` `Arduino` `C++` `UART` `Wi-Fi TCP`
