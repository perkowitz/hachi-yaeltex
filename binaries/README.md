*v0.20*
- Fixed bug with Pitch Bend using MIDI Merge feature
- Fixed missing dump of encoder's second function
- Change banks in Kilowhat when shifting in the controller. Component info messages now send currentBank for Kilowhat to select the right bank.
- Encoders configured with pivot feedback mode now initialize at center value between MIN and MAX
- Fix feedback bug for encoder rings when MIN and MAX have a small range
- Modified some timings of watchdog to prevent restarts.
- NEW FEATURE: Optional [dead zone](https://docs.google.com/document/d/13jk2V8_aGEV3KC9KqYWe2OicseXhESRUln3eTYHGY6o/edit#bookmark=id.pmkbazse6x5h) in the middle for analogs.
- NEW FEATURE: [Value to intensity](https://docs.google.com/document/d/13jk2V8_aGEV3KC9KqYWe2OicseXhESRUln3eTYHGY6o/edit#bookmark=id.nj3a15nyemqx). Send MIDI messages on a special feedback channel to modify brightness for each component.
- NEW FEATURE: [Special features channels](https://docs.google.com/document/d/13jk2V8_aGEV3KC9KqYWe2OicseXhESRUln3eTYHGY6o/edit#bookmark=id.1abxvh1ar05x). Configure on which channel to use the special features Value To color (rotary encoders), vumeter, value to intensity, remote banks, split mode.
- NEW FEATURE: [New options for feedback sources](https://docs.google.com/document/d/13jk2V8_aGEV3KC9KqYWe2OicseXhESRUln3eTYHGY6o/edit#bookmark=id.urfnii3oa3jq). Choose to have local, MIDI and/or USB feedback on each component. Encoder switches and digitals didn't have this option and encoder rotary had local feedback always ON by default.
- NEW: Added 2 more variable [speed modes](https://docs.google.com/document/d/13jk2V8_aGEV3KC9KqYWe2OicseXhESRUln3eTYHGY6o/edit#bookmark=id.ea6z1a44glcp) for rotary encoders. Improved and fixed fast speed to be more pot-like.
- NEW: Support for new module of 100mm faders (F21.100)
- NEW: Auto-select ON by default in Kilowhat. Scroll to the right card.
- NEW: Now firmware is a single file combining AUX and MAIN. New firmware manager version for this.
---

*Firmware MAIN*
v0.16
- New feature: Save controller state in EEPROM and load at startup.
- New feature: Dump current controller state at startup.
- New feature: Low Intensity OFF state for digitals and encoder switch feedback.
- New feature: Encoder ring color change remotely with same message as rotary encoder, on channel 16 (value to color, see table in manual)
- New feature: Split mode for analogs. Split analog control in half, send regular message on one half and same message on channel 15 on the other half of the component's travel.
- IMPORTANT CHANGE: Encoder switch's feedback and Digital's feedback now turn on with MAX VALUE and off with MIN VALUE. Live sets might require rework.
- Solved issue with more than 16 digital modules. Lower SPI Speed in these cases.
- Improved considerably wake up time for every controller.
- Pivot mode feedback encoders now start with value 64. Will be improved to "center value" instead of hardcoded 64.
- Added redirection with USB/MIDI Merge flags for MIDI Start, Clock, Stop, Continue, Tune Request, Song Select, Song Position, TimeCodeQuarterFrame.

*Firmware AUX*
v0.16
- Solved division by zero bug. If number of LEDs was zero, the universe imploded.

-------------

*Firmware MAIN*
v0.15
- Improved LED feedback handling, no more missing notes or stucked LEDs.
- Added handlers to be able to receive MIDI messages: Timecode Quarter Frame, Song Position, Song Select, Tune Request, Clock, Start, Stop, Continue
- Added Watchdog reset functionality to prevent hangs on OSX start-up.
- Now relative modes also work for VU CC (before only for CC)
- Minor fixes to BURST_INIT and BURST_END messages
- All serial data is received in IRQ 
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
