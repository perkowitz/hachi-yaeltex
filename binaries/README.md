*Firmware MAIN*
v0.15
- Added handlers to be able to receive MIDI messages: Timecode Quarter Frame, Song Position, Song Select, Tune Request, Clock, Start, Stop, Continue
- Minor fixes to BURST_INIT and BURST_END messages
- All serial data is received in IRQ 
- Now relative modes also work for VU CC (before only for CC)
- Added Watchdog reset functionality to prevent hangs on OSX start-up.
- Reset to bootloader option added to the Serial test-mode.
- Turn LEDs off when resetting to bootloader.
- External and internal feedback are separated, and only waits for more data if external feedback is being received. This fixes the encoder movement that messed up the timing on the feedback update.
- Encoder color change with same message as configured, on channel 16 (not available yet! needs Kilowhat update)
- Split mode for analog components (not available yet! needs Kilowhat update)
- **IMPORTANT:** To compile this version with Arduino IDE, you need to replace *hardware/yaeltex* folder for the one on the repo to *Documents/Arduino/hardware* and add copy *Adafruit_SleepyDog* folder from *libraries* folder into *Documents/Arduino/libraries*

*Firmware AUX*
v0.15
- No change. Match with MAIN

-------------

*Firmware MAIN*
v0.14
- Bugfix for encoder priority feature
- Bugfix for double click not updating shifted rotary values
- Automatically update new min and max for analog raw values.
- Bugfix for encoder switch with color range enable function not always updating feedback
- Bugfix for stucked pixels
- Burst bank data fixed for digitals. Was hardcoded 128, but needed to use count for each port.
- Monitor incoming midi messages added to Serial test mode

*Firmware AUX*
v0.14
- Only send SHOW_END if show() actually begins
- Every data that arrives, is treated as a burst of data. 
- Only update strips that have changes.
- Signal SHOW_END regularly.

-------------

*Firmware MAIN*
v0.13
- Bugfix for encoder ring

*Firmware AUX*
v0.13
- No change. Match with MAIN

-------------

*Bootloader MAIN*
v0.11
- Check MIDI IN pin (RX) for low state, to stay in bootloader.

*Bootloader AUX*
v0.11
- No change. Match with MAIN

-------------

*Firmware MAIN*
v0.12
- Configuration update with unused bytes for future implementation
- Re worked bank change feedback to send smaller bursts of data 
- Dropped 3 bytes on protocol.
- Check for free ram before allocating memory
- Analog split
- Max component check
- Local startup. Start digitals and encoder switches locally, until a matching message for each component is received. Disabled for now.
- Rainbow on demand en el test mode
- Send feedback only when encoder ring state changed.

*Firmware AUX*
v0.12
- Dropped 3 bytes on protocol. Rainbow duration improved to match with different configurations.

-------------

*Firmware MAIN*
v0.11
- Program change + and - limited to MIN and MAX values

*Firmware AUX*
v0.11
- No change. Match with MAIN

-------------

*Bootloader MAIN*
v0.10
- Boot flag for bootloader address match with config change

*Bootloader AUX*
v0.10
- No change. Match with MAIN

-------------