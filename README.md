# arduino_clock_7_segment
Arduino clock using Adafruit 7 Segment Backpack, DS1307 RTC module & SparkFun Red/Grn Rotary Encoder

Uses the following additional libraries not included in the default arduino installation:
- https://github.com/adafruit/Adafruit-LED-Backpack-Library
- https://github.com/adafruit/Adafruit-GFX-Library
- https://www.pjrc.com/teensy/td_libs_Encoder.html

The A and B connections from the rotary encoder are connected to arduino pins 2 and 3 which are used as interrupts.
