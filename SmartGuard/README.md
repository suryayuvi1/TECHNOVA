# SmartGuard — ESP32-C3 Safety Monitoring System

SmartGuard is an offline safety monitoring project for the ESP32-C3. It reads temperature and humidity from a DHT11, monitors a flame sensor, checks worker RFID cards, and provides local alerts through an RGB LED, buzzer, LCD, and a phone-friendly web dashboard.

## Repository contents

- `SmartGuard.ino` — Arduino sketch and embedded dashboard
- `README.md` — setup, configuration, and upload instructions
- `.gitignore` — common Arduino build and editor files to omit from Git

## Hardware

- PCB Cupid Glyph-C3 ESP32-C3 board
- DHT11 temperature/humidity sensor
- Flame sensor with digital output
- MFRC522 RFID reader
- 16×2 I2C LCD using a PCF8574 backpack (address `0x27`)
- RGB LED and buzzer
- Push button

### Pin assignments

| Component | ESP32-C3 pin |
| --- | ---: |
| DHT11 data | 0 |
| Buzzer | 1 |
| Flame sensor output | 2 |
| RGB LED red | 3 |
| LCD SDA | 4 |
| LCD SCL | 5 |
| RGB LED green | 6 |
| RGB LED blue | 7 |
| RFID SS/SDA | 8 |
| RFID SCK | 9 |
| Push button | 10 |
| RFID MOSI | 18 |
| RFID MISO | 19 |
| RFID RST | 20 |

Check the board pinout and module voltage requirements before wiring. The pin assignments above come from the sketch.

## Arduino setup

1. Install Arduino IDE and add ESP32 board support.
2. Install these libraries through **Library Manager**:
   - DHT sensor library (Adafruit)
   - Adafruit Unified Sensor (dependency for the DHT library)
   - MFRC522
   - LiquidCrystal_PCF8574
3. Open `SmartGuard.ino` in Arduino IDE and select the ESP32-C3 board and serial port.
4. Before uploading, configure your own access point password and authorized RFID card UID in the sketch. Do not publish real credentials or a UID that grants access. The included sample password and UID are placeholders/examples.
5. Upload the sketch and open Serial Monitor at **115200 baud**.

## Connect to the dashboard

After startup, connect a phone or computer to the Wi-Fi access point configured in the sketch. Open `http://192.168.4.1` in a browser. The dashboard is served directly by the ESP32-C3 and works on the local network without internet access.

## Behavior

Alerts are prioritized in this order: flame detected, emergency mode, high temperature, DHT sensor error, denied RFID, authenticated worker, then normal. The high-temperature threshold is configured as `TEMP_LIMIT` in the sketch. Emergency mode can be toggled with the physical button or dashboard controls.

## Notes

- The flame sensor is configured as active-low (`FLAME_DETECTED LOW`); change this if your module behaves differently.
- RFID authorization compares the scanned card UID with `authorizedUID` in the sketch. Replace it with your card's UID before relying on access control.
- This is a prototype project. Test sensors and alerts with your hardware before using it in any safety-critical setting.

## License

No license is specified. Add a license file if you want to state how others may use, modify, and redistribute this project.
