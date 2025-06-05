# ESPHome CAN Sniffer

ESPHome components to build custom CAN Sniffer Firmware

## What in this repo?

It's a set of example configurations, custom components, links to external modules to let you build firmware for your CAN Sniffer device.

## Why ESPHome?

It let you code less.

You describe your device has in yaml:

- device board
- components it has
- pins you using
- what should happen when something happened
- small chunks of c++ code in lambda expressions
- big chunks of c++ code in custom components

ESPHome generates c++ code for your firmware, compiles it and uploaded to device

## Isn't ESPHome for IoT home automation via Home Assistant

Yes it is. But all works just fine without Home Assistant. If you have one - enjoy your sensor data statistics over time and use Home Assistant to control your device in development.

## How to build

- Make sure you have Python installed
- Create Virtual Environment

    ```sh
    python -m venv .venv
    ```

- Activate it
    Linux:

    ```sh
    source .venv/bin/activate
    ```

- Install ESPHome

    ```sh
    pip install ESPHome
    ```

- Switch to folder with yaml device description

    ```sh
    cd <my_device_folder>
    ```

- Build, upload and run your firmware

    ```sh
    esphome run <my_yaml_file>
    ```

## License

The code in this repository is MIT licensed. But your final firmware will use ESPHome and other components licensed under GPLv3. Plese be warned.
