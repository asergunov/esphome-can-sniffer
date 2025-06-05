# CAN Sniffer Device

Device to save logs to SDCard and host them over http.

## Components

- XIAO ESP32c6
- 2x TJA1042T/3 CAN Transceivers
- SPI-Compatible SDCard
- CAR USB Power supply

## Usage

Make `secrets.yaml` with your credentials:

```yaml
wifi_ssid: ""
wifi_password: ""

ap_password: ""

ota_password: ""
api_encryption_key: ""
```

- build and install firmware
- Connect transceivers to bus
- Install SDCard
- Power on device
- <http://can-logger.local> to see the device dashboard
- <http://can-logger.local/file> to see card content
- <http://can-logger.local/file/can.log> to see the log
- <http://can-logger.local/file/can.log?download=true> to download log

## Component consideration

### Controller

ESP32c6 is the only ESP32 SoC with two TWAI 2.0 controllers integrated for today. It doesn't have SDMMC controller so SDCard could be used only in SPI mode.

XIAO board has external antenna connector, 3.3v LDO.

### CAN Transceivers

To don't add level shifters we need 3.3v compatible can transceivers. There was two options considered:

- SN65HVD230
- TJA1042T/3

SN65HVD230 is 3.3v powered so it can't drive CANH to standard 3.5V. TJA1042T/3 has dual power supply so it could have 3.3v for logic and 5v for driving the bus.

## Schematics

- [Pin selecting considerations](../../esp32c6.md#pin-considerations)

### Transceivers

- CAN1_RX -> GPIO4
- CAN1_TX -> GPIO16
- CAN1_SLP -> GPIO9/BOOT
- CAN1_GND -> GND
- CAN1_VIO -> 3v3
- CAN1_VCC -> 5v

- CAN2_RX -> GPIO5
- CAN2_TX -> GPIO17
- CAN2_SLP -> GPIO9/BOOT
- CAN2_GND -> GND
- CAN2_VIO -> 3v3
- CAN2_VCC -> 5v

### SDCard

All the SD card pins except GND should have ~10K pull up resistors to 3.3V. Nice to have decoupling capacitor near the card.

- DAT3 -> GPIO21, 10k pull up
- CLK -> GPIO6, 10k pull up
- CMD -> GPIO2, 10k pull up
- DAT0 -> GPIO7, 10k pull up
- VSS -> GND
- VCC -> 3v3
- DAT1 -> 10k pull up
- DAT2 -> 10k pull up
