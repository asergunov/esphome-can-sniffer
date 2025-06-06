# Powering

Car BAT+ is very unstable. It can easily go 150v positive and negative for short period of time.

If you design your power supply considering 12-15v you'll have your device roasted.

## Car USB power adapter

Pros:

- Cheap
- Reliable
- Zero knowledge needed

Cons:

- Some car sockets are powered only when ignition is on
- Often has light indications which could drain battery

Make sure GND is connected to Car GND to have correct bus signals

Chose this option. The rest of the file exists to explain why.

## Custom power supply

Design goals:

- Safety
- Affordable
- 12/24v compatible
- Low consumption while device sleeping

### Overvoltage protection

To limit voltage on regulator put Dual Zener diode to short voltage spikes trough itself. For example SMAJ24CA limiting voltage by 38v9 and works fine in both 12v and 24v cases.

Since Zener diode makes short it should have fuse to prevent diode overheat. Polyfuse is good option. It increases own resistance when heating up. Make sure your fuse cut current before Zener diode reaches 150C.

- BAT+ -> polyfuse pin 1
- polyfuse pin 1 -> BAT_FUSED
- SMAJ24CA pin 1 -> GND
- SMAJ24CA pin 2 -> BAT_FUSED

### Reverse voltage protection

Diode can be used, but it will introduce voltage drop. MOSFET is better option. Select N-channel MOSFET with low Rds(on) and Drain-Source voltage rating higher then 39v. To limit gate voltage Zener diode can be used.

- BAT_FUSED -> MOSFET source
- MOSFET drain -> BAT_REVERSE_PROTECTED, Zener diode anode
- MOSFET gate -> Zener diode cathode -> resistor -> GND

### Noise filter

It's good idea to add ferrite bead to filter out high frequency noise. It can be placed between BAT_REVERSE_PROTECTED and voltage regulator.

### Voltage regulator

Use buck converter to step down voltage to 5v. Make sure it has low quiescent current to save power while device is sleeping. For example LMR14010A has low quiescent current and can handle up to 40v input voltage.

Use capacitors on input and output to filter out noise and let your device survive short drops.
