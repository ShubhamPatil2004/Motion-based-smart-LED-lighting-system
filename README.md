# Motion-Based Smart LED Lighting System

An **ESP32-based smart lighting system** that automatically controls LED brightness using **PIR motion detection** and **LDR-based ambient light sensing**. The system features **three-stage PWM dimming**, adjustable timing and light thresholds using potentiometers, and a compact custom PCB design for energy-efficient lighting automation.

## Features

* Human motion detection using a PIR sensor
* Ambient light sensing using an LDR
* Automatic energy-efficient lighting control
* Three-stage PWM brightness control:

  * 100% brightness
  * 50% brightness
  * 20% brightness
  * Automatic OFF
* Adjustable lighting duration using potentiometers
* Adjustable ambient light threshold
* Compact PCB implementation

## Hardware Used

* ESP32 DevKit V1
* HC-SR501 PIR Motion Sensor
* LDR (Light Dependent Resistor)
* 3 × 10 kΩ Potentiometers
* LED
* Resistors
* General Purpose PCB
* USB Power Supply

## Working Principle

The ESP32 continuously monitors the **PIR sensor** and **LDR**.

* If **motion is detected** and the **ambient light is below the set threshold**, the LED turns **ON**.
* The LED brightness then gradually decreases:

  * **100% → 50% → 20% → OFF**
* If new motion is detected while the LED is dimming, the timer resets and the LED returns to **100% brightness**.

## Future Improvements

* IoT monitoring and remote control
* Mobile application
* MQTT integration
* Blynk integration
* Home Assistant support
* Machine learning-based occupancy prediction
