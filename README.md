# piggyswitch

This is a basic USB interface for a Honeywell Micro Switch SD-16279
keyboard.

The code is for a Teensy 2.0 listening to the original 8048 controller
without interfering.


## Connections:

```
Teensy   8048

GND      7 (EA)
D5       21 (P20)
D4       22 (P21)
D6       23 (P22)
D7       24 (P23)
B4       27 (P10)
B5       28 (P11)
B6       29 (P12)
F7       30 (P13)
F6       31 (P14)
F5       32 (P15)
F4       33 (P16)
F1       34 (P17)
VCC      40 (VCC)
```

Additionally Teensy B0,B1,B2 are directly attached to switches outside
the keyboard matrix, physically I do this at resistors 20,21,22.

All other Teensy pins are NC.

