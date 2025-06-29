# GVRET device

## Adding GVRET protocol support

```yaml
canbus:
  - platform: esp32_can
    id: can1
    can_id: 100
    rx_pin: GPIO4
    tx_pin: GPIO16
    bit_rate: 500KBPS

gvret:
  can:
  - canbus_id: can1
```

## In SavvyCAN

- Open Connection Window
- Add new device connection -> Network Connection (GVRET)
- In `IP Address` field type ip address or device mDNS name. For example `gvret.local`