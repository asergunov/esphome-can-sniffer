# Transceivers

Most of the automotive equipment designed to work with 5v logic. This requires level shifters to work with 3v3 ESP32 logic.

There are few transceivers available natively working 3v3 logic

## TJA1042T/3 CAN

Has separate VIO pin powering controller IO and defining IO levels.

## SN65HVD230 CAN

Is only 3.3v powered so it can't drive CANH to standard 3.5V. Works in most of the cases.
