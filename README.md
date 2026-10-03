# Bluetooth Receiver

A modular system that allows playing audio through Bluetooth on devices that don't support it out of the box. The base module consists of ESP32 microcontroller with PCM5102A DAC converter. Depending on the need, modules like a screen displaying current playback status or buttons to control the playback can be added.

## Bill of Materials
- ESP-WROOM-32 Dev Board
- PCM5102A DAC
- ILI9341 Screen
- AO3401 P-MOSFET
- SMD-to-THT adapter for the A19T
- 2 standard 2-leg buttons
- 1000uF electrolytic capacitor
- 0.1uF ceramic capacitor
- 10k resistor
- Breadboard
- Jumper wires

## Repo structure

Note: `esp-idf` here refers to the directory where esp-idf is installed, for example `~/.espressif/v6.0/esp-idf`.

```bash
.
├── components/
│   ├── a2dp_utils              # Copied from esp-idf/examples/bluetooth/bluedroid/classic_bt/common/...
│   ├── bredr_app_common_utils  # as above
│   ├── bt_app_core_utils       # as above
│   └── bt                      # Copied from esp-idf/components/bt
├── core                        # Core bluetooth receiver logic
├── screen                      # Playback status screen module
├── pins                        # Hardware pins & buttons module
├── main                        # Main source code
├── power                       # Power management module
└── sdkconfig                   # Menuconfig configuration
```

## Workarounds

Bluetooth is a complex protocol with a lot of caveats. It's hard to emulate proprietary drivers at home that have been fine-tuned for all kinds of devices for years. There are some workarounds, though, that make the quality of this device way better. These workarounds, along with the issues they solve, are listed below.

**1. Issue: initiating a connection with previously paired device from Linux (blueman) was unreliable. For some reason I still haven't figured out, the PC sometimes opened a GAP tunnel but didn't open an A2DP tunnel and it timed out after exactly 60 seconds.** I didn't want to dig into the components/examples code, so this is simply solved by restarting ESP32 when A2DP connection is not initiated in 5 seconds after opening a GAP connection and initiating the connection from ESP32 to the PC. This might seem like an overkill, but it's the only way it finally worked for me - restarting only the BT stack is not a standard procedure and would be harder to implement, and the stack has to be restarted in order to initiate a new connection.

**2. Issue: the audio was constantly glitching when playing from an iPhone.** This was solved by changing I2S data output pin from 22 to 19. iPhones use a different, more demanding codec than other devices I tested, and it's likely that there was a bus conflict, which resulted in overloading of the GPIO pin. Or it was just malfunctioning, since I used a cheap Chinese board. Who knows..

**3. Issue: There was terrible clipping when volume was too high on some devices.** The "right" way to solve this would be to use an amplifier and balance its gain and ESP's output amplitude accordingly. But since my primary use is plugging the receiver into a guitar amp anyway, it's solved by simply multiplying each sample by a number like 0.7.
