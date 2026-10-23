# Installation Guide for Arduino DHT22 Temperature & Humidity Sensor Project

This guide will help you set up and run the temperature and humidity monitoring system using an Arduino Uno and DHT22 sensor.

## Prerequisites

1. Hardware Requirements:
   - Arduino Uno board
   - DHT22 temperature and humidity sensor
   - Connecting wires
   - USB cable for Arduino

2. Software Requirements:
   - Visual Studio Code
   - PlatformIO IDE extension
   - Arduino IDE (optional)

## Installation Steps

### 1. Hardware Setup

Connect the DHT22 sensor to the Arduino Uno as follows:
- Connect DHT22 VCC pin to Arduino 5V
- Connect DHT22 DATA pin to Arduino Digital Pin 2
- Connect DHT22 GND pin to Arduino GND

### 2. Software Setup

1. Install Visual Studio Code
   - Download from: https://code.visualstudio.com/
   - Install following your operating system's procedure

2. Install PlatformIO IDE Extension
   - Open VS Code
   - Go to Extensions (or press Ctrl+Shift+X)
   - Search for "PlatformIO IDE"
   - Click Install

### 3. Project Setup

1. Clone or download this project to your local machine

2. Open the project in VS Code:
   - File -> Open Folder
   - Navigate to the project directory
   - Select the folder containing `platformio.ini`

3. The project will automatically download required dependencies:
   - Adafruit Unified Sensor library
   - DHT sensor library

### 4. Building and Uploading

1. Connect your Arduino Uno to your computer via USB

2. Build the project:
   - Click the PlatformIO icon in the sidebar
   - Click "Build" or use the command palette (Ctrl+Shift+P) and type "PlatformIO: Build"

3. Upload to Arduino:
   - Click "Upload" in PlatformIO
   - Wait for the upload to complete

### 5. Monitoring

To monitor the sensor readings:
1. Open Serial Monitor:
   - Click on "Serial Monitor" in PlatformIO
   - Or use the command palette and type "PlatformIO: Serial Monitor"
2. You should see temperature and humidity readings every 2 seconds

## Troubleshooting

1. If upload fails:
   - Check if Arduino is properly connected
   - Verify the correct COM port is selected
   - Make sure no other program is using the serial port

2. If sensor readings show "Failed to read from DHT sensor":
   - Check the wiring connections
   - Verify the sensor is properly powered
   - Try disconnecting and reconnecting the Arduino

## Project Structure

- `src/main.cpp`: Main program file
- `platformio.ini`: Project configuration and dependencies
- `diagram.json`: Wiring diagram for the project

## Additional Notes

- The sensor readings update every 2 seconds
- Temperature is displayed in both Celsius and Fahrenheit
- Serial monitor runs at 9600 baud rate
