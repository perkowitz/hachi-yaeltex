Firmware MAIN
v0.14
- Bugfix for encoder priority feature
- Bugfix for double click not updating shifted rotary values
- Automatically update new min and max raw values.
- Bugfix for encoder switch with color range enable function not always updating feedback

Firmware AUX
v0.14
- Only send SHOW_END if show() actually begins
- Every data that arrives, is treated as a burst of data. 
- Only update strips that have changes.
- Signal SHOW_END regularly.

Firmware MAIN
v0.13
- Bugfix for encoder ring

Firmware AUX
v0.13
- No change. Match with MAIN

Bootloader MAIN
v0.11
- Check MIDI IN pin (RX) for low state, to stay in bootloader.

Bootloader AUX
v0.11
- No change. Match with MAIN

Firmware MAIN
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

Firmware AUX
v0.12
- Dropped 3 bytes on protocol. Rainbow duration improved to match with different configurations.

Firmware MAIN
v0.11
- Program change + and - limited to MIN and MAX values

Firmware AUX
v0.11
- No change. Match with MAIN

Bootloader MAIN
v0.10
- Boot flag for bootloader address match with config change

Bootloader AUX
v0.10
- No change. Match with MAIN