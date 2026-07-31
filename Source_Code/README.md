
# Source Code

This folder contains the source code for the **Motion-Based Smart LED Lighting System** built using the **ESP32** and the **Arduino framework**.

The system uses a **PIR sensor** to detect motion, an **LDR** to measure ambient light, and **three potentiometers** to adjust timing and light threshold values. It supports **AUTO**, **MANUAL ON**, and **MANUAL OFF** modes.

Users can control the system through **Bluetooth Classic** by changing modes, setting timer values (T1, T2, T3), and checking the current status. All settings are saved in the ESP32's **Non-Volatile Storage (NVS)**, so they remain available after a restart.

To save power, the ESP32 automatically enters **deep sleep** when inactive and wakes up when motion is detected.
