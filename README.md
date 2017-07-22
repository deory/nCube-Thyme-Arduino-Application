# nCube-Thyme-Arduino-Application

## Introduction
nCube:Thyme for Arduino is ADN(Application Dedicated Node) of oneM2M global IoT standard.
If you want to know about oneM2M, please reference [this(oneM2M)](http://onem2m.org/technical/published-documents).
This nCube:Thyme for Arduino Application is written in C language supporting Arduino sketchs.
Also this application using nCube:Thyme for Arduino source code provided by KETI(Korea Electronics Technology Institute).

## Connectivity structure
nCube:Thyme for Arduino Application is just ADN-AE. So, if you want to use this application's full function, first setup [Mobius](https://github.com/IoTKETI/Mobius) IoT Platform.
This application connect to Mobius IoT Platform using [MCA reference point](http://onem2m.org/images/files/deliverables/Release2/TS-0001-%20Functional_Architecture-V2_10_0.pdf) and Connection protocol is MQTT.
<div align="center">
<img src="https://user-images.githubusercontent.com/23092171/28490822-a66e5344-6f1e-11e7-8934-e1e35104bd1e.png" width="600"/>
</div>

## Software structure
nCube:Thyme for Arduino Application use WiFi for using MQTT connection providing publish and subscribe functions. Co2 sensor connected using digital Pin on Adafruit Feather M0 measure and upload Co2 concentration using MQTT. Also RGB-LED and Power Relay connected with Adafruit Feather M0 is controlled depending on MQTT packets.
<div align="center">
<img src="https://user-images.githubusercontent.com/23092171/28490823-a9df6a22-6f1e-11e7-9e46-43f09f67de7b.png" width="600"/>
</div>

## Hardware Composition
- Adafuit Feather M0
- CM1106 Co2 Sensor
- RGB-LED
- Adafruit Power Relay FeatherWing
<div align="center">
<img src="https://user-images.githubusercontent.com/23092171/28490927-d2e5e458-6f20-11e7-948b-e3889089ec97.png" width="600"/>
</div>
Connect theese things like figure above.

## Pre-requirement
- Install [Arduino IDE](https://www.arduino.cc)
- In the Arduino IDE, add Board manager URL provided by Adafruit.
- Install Board managers. One is Arduino SAMD board manager, the other is Adafruit SAMD board manager.
- Download [nCube:Thyme for Arduino](https://github.com/IoTKETI/nCube-Thyme-Arduino), and move libraries to the Arduino IDE's libraries directory.

## Running nCube:Thyme for Arduino Application
1. Open nCube-Mint.ino file with Arduino IDE.
2. Upload the sketch to Adafruit Feather M0.
3. nCube:Thyme for Arduino Application will enter WiFi provisioning mode.
4. Connect to the nCube:Thyme for Arduino Application's WiFi provisioning page using smart phone or laptop.
5. Input WiFi AP SSID and Password in the provisioning page.
6. Then, nCube:Thyme for Arduino Application will create ae, cnt, sub on Mobius according to configurarion in the nCube-Mint sketch.

