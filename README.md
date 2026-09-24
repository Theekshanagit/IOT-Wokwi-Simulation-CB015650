# 🌿 Automated Commercial Micro-Climate Nursery Controller

> **Module:** COMP50069 — Hardware, Microcontrollers and Sensors  
> **Institution:** APIIT Sri Lanka | University of Staffordshire  
> **Target Domain:** Agri-Tech & Industrial High-Value Micro-Climate Greenhouses  
> **Simulation Link:** [Simulate on Wokwi](https://wokwi.com/projects/475439433764366337)

---

## 📌 Overview

The **Automated Commercial Micro-Climate Nursery Controller** is an ESP32-based embedded system designed to protect sensitive indoor crops from midday heat spikes and stagnant air conditions. Operating as a closed-loop Finite State Machine (FSM), the controller monitors environmental parameters in real-time, regulates micro-climates using passive vent actuation and supplemental lighting, and provides hardware interrupt-driven manual overrides and diagnostic safety postures.

---

## ✨ Key Features

* **🤖 Autonomous Mode:** Real-time closed-loop control of ambient temperature (`DHT22`) and solar light levels (`LDR`).
  * **Temperature Regulation:** Opens the vent servo window ($90^\circ$) when ambient temperature exceeds **30.0°C**.
  * **Photoperiod Supplementation:** Illuminates a 3-LED grow light array when natural solar irradiance falls below threshold (**<1500 ADC counts**).
* **🛠️ Manual Override Mode:** Hardware interrupt-driven manual control via a push-button on `GPIO26`. Completely bypasses automated routines and locks the vent fully open for routine maintenance, harvesting, or sanitation cycles.
* **🛡️ Failsafe Safety Posture:** Automatic hardware failure detection (e.g., disconnected signal lines, floating `NaN` values, or out-of-bound ADC readings). Immediately locks vents open to prevent thermal crop damage and flashes diagnostic warnings on the OLED display.
* **📺 Real-Time Display:** 0.96" SSD1306 OLED display driving live updates of temperature, humidity, light levels, active system mode, and diagnostic alerts.

---

## 🛠️ Hardware Pinout Matrix

| Component | Pin | ESP32 GPIO Pin | Description / Wiring Notes |
| :--- | :--- | :--- | :--- |
| **DHT22** | DATA | `GPIO 4` | Digital temp/humidity sensor (Requires 10kΩ pull-up) |
| **LDR Module** | AO | `GPIO 34` | Analog light sensor (ADC1 channel) |
| **Servo Motor**| Signal | `GPIO 13` | PWM pin controlling vent window position |
| **Push Button** | Pin 1 | `GPIO 26` | Hardware interrupt (Active LOW, internal pull-up) |
| **Grow LED 1** | Anode | `GPIO 27` | Supplemental PAR LED (via 220Ω resistor) |
| **Grow LED 2** | Anode | `GPIO 2` | Supplemental PAR LED (via 220Ω resistor) |
| **Grow LED 3** | Anode | `GPIO 5` | Supplemental PAR LED (via 220Ω resistor) |
| **OLED (I2C)** | SDA / SCL | `GPIO 21` / `GPIO 22` | 128x64 SSD1306 Display (I2C Address `0x3C`) |

---

## 📂 Repository Structure

```text
.
├── src/
│   └── main.ino           # Complete C++ ESP32 firmware source code
├── diagram.json           # Wokwi circuit diagram layout & wire connections
├── wokwi-project.txt      # Wokwi project identifier
├── docs/
│   └── Report.pdf         # 3,000-word Technical Report (PDF format)
└── README.md              # Project overview and documentation
