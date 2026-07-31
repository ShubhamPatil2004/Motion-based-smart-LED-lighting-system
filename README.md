Motion-based-smart-LED-lighting-system
An ESP32-based smart lighting system that automatically controls LED brightness using PIR motion detection and LDR-based ambient light sensing. Features three-stage PWM dimming, adjustable timing and light thresholds via potentiometers, and a custom PCB design for energy-efficient, intelligent lighting automation.

Features
Human motion detection using PIR sensor

Ambient light sensing using LDR

Energy-efficient lighting control

Three-stage PWM dimming

100%
50%
20%
Adjustable timing using potentiometers

Adjustable light threshold

Compact PCB implementation

Hardware
ESP32 DevKit V1

PIR Sensor (HC-SR501)

LDR

3 × 10k Potentiometers

LED

Resistors

General Purpose PCB

USB Power Supply

Working Principle
The ESP32 continuously monitors:

PIR Sensor
LDR
If:

Motion = Detected
AND
Ambient Light = Dark
The LED turns ON.

Brightness decreases gradually:

100% → 50% → 20% → OFF

If new motion is detected during dimming, the timer resets and the LED returns to full brightness.

Future Improvements
IoT Monitoring
Mobile App
MQTT
Blynk
Home Assistant
Machine Learning based occupancy prediction
