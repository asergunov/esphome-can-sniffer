# ESP32C6

ESP32C6 is the only ESP32 SoC with two TWAI 2.0 controllers integrated for today. It doesn't have SDMMC controller so SDCard could be used only in SPI mode.

XIAO board has external antenna connector, 3.3v LDO

## Pin considerations

### Low Power Domain pins

GPIO0-GPIO7 pins can be used in sleep mode to wake controller up.

If connected for transceiver RX could wake controller up by incoming message.

If used to measure battery voltage can wake controller up using low power coprocessor.

### SPI pins

For best SPI performance controller has designated pins via IO MUX.

- GPIO2/FSPIQ for MOSI
- GPIO7/MTDO/FSPID for MISO
- GPIO6/MTCK/FSPICLK for CLK
- GPIO16-GPIO21/FSPICS0-FSPICS5 for CS

## Internal/External Antenna switch

The GPIO14 pin is used to select internal on board antenna or external antenna

### ADC pins

GPIO0-GPIO6 are ADC pins. Can be used for voltage measurements

### RTC Clock Crystal

GPIO0 and GPIO1 pins could be used to connect external Real Time Clock crystal

### Strapping pins

GPIO9/BOOT pin is pulled up if BOOT button is not pressed. Nice choice to SLP pin of transceivers to make sure boot glitches will not interfere CAN bus

## References

- [ESP32c6 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c6_datasheet_en.pdf)
