BOSSA 1.7.0-mattairtech-3
-------------------------------

Updated timeout for D51 1MB FLASH (5 second erase).
Adds support for:

 * ATSAMD21J16B
 * ATSAMD21J15B
 * ATSAMD21G16B
 * ATSAMD21G15B
 * ATSAMD21E16B (thanks [aksonlyaks](https://github.com/aksonlyaks))
 * ATSAMD21E15B


BOSSA 1.7.0-mattairtech-2
-------------------------------

This version improves support for the SAML21 and SAMC21.
It also adds support for the SAMD51 (M4F):

 * ATSAMD51P20A
 * ATSAMD51P19A
 * ATSAMD51N20A
 * ATSAMD51N19A
 * ATSAMD51J20A
 * ATSAMD51J19A
 * ATSAMD51J18A
 * ATSAMD51G19A
 * ATSAMD51G18A


BOSSA 1.7.0-mattairtech-1
-------------------------------

This version merges in upstream changes from BOSSA 1.7.0-arduino.


BOSSA 1.6.1-arduino-mattairtech-2
-------------------------------

1.6.1-arduino-mattairtech-2 corrects SAML CHIPID information in Devices.h.

This version of BOSSA is a fork of BOSSA 1.6.1-arduino.
It adds support for more M0+ chips, the SAML and SAMC:

  * ATSAML21E18B
  * ATSAML21E17B
  * ATSAML21E16B
  * ATSAML21E15B
  * ATSAML21G18B
  * ATSAML21G17B
  * ATSAML21G16B
  * ATSAML21J18B
  * ATSAML21J17B
  * ATSAML21J16B
  * ATSAMC21E18A
  * ATSAMC21E17A
  * ATSAMC21E16A
  * ATSAMC21E15A
  * ATSAMC21G18A
  * ATSAMC21G17A
  * ATSAMC21G16A
  * ATSAMC21G15A
  * ATSAMC21J18A
  * ATSAMC21J17A
  * ATSAMC21J16A
  * ATSAMC21J15A


BOSSA 1.5-arduino-mattairtech-1
-------------------------------

This version of BOSSA is a fork of BOSSA 1.5-arduino.
It adds support for more SAMD chips:

  * ATSAMD21E17A
  * ATSAMD21E16A
  * ATSAMD21E15A
  * ATSAMD21G17A
  * ATSAMD21G16A
  * ATSAMD21G15A
  * ATSAMD21J17A
  * ATSAMD21J16A
  * ATSAMD21J15A
  * ATSAMD11D14AM
  * ATSAMD11D14AS
  * ATSAMD11C14A

This adds to the previously supported SAMD chips:

  * ATSAMD21J18A
  * ATSAMD21G18A
  * ATSAMD21E18A
  * ATSAMDR1E18A

Only the ATSAMD21E18A, ATSAMD21E17A, and ATSAMD11D14AM have been tested
with this fork.

All of the D21 chips must use an 8KB bootloader, while the DR1 and D11
must use a 4KB bootloader. See Devices.h for more information.


BOSSA 1.4 for Arduino

This version of BOSSA is a fork of the original project and contains some
patches specific for the Arduino products that are unlike to be accepted
upstream.

The original software is developed by Scott Shumate and can be found here:

http://www.shumatech.com/web/products/bossa

BOSSA 1.5-arduino
-----------------

* Added Codelite project for easier development and debug.
* Improved support for SAMD21 chip by using existing applet already in use for other devices.
  * Added time count for operations erase, write, read and verify.
  * Added Devices.h for global definitions for each devices.
  * NVMFlash::write() rewritten.
  * Removed some useless functions from 1.4 release.
  * Added a mask for SAMD chipid: DIE and REV bitfields are not taken into account as they may vary in the time without impacting the features.
  * Improved write time performance by a factor of 2x


BOSSA 1.4-arduino
-----------------

* Added support for SAMD21 chip


BOSSA 1.2
---------

FILES
bossa-1.2.msi -- Windows 2000+
bossa64-1.2.msi -- Windows 2000+ 64-bit
bossa-i686-1.2.tgz -- Linux GTK
bossa-x86_64-1.2.tgz -- Linux GTK 64-bit
bossa-1.2.dmg -- MAC OS X 10.6+

NEW IN THIS RELEASE
* New BOSSA shell command line application to do basic memory, flash, and PIO diagnostics
* Workaround for SAM3U firmware bug
* Fixed a bug with setting boot to flash bit on SAM3 devices

RELEASE NOTES
* The OS X USB driver detects an Atmel device as a USB modem.  When prompted about a new network interface, click Cancel to continue.
* Some stability issues have been seen with the OS X USB driver using BOSSA.  When running BOSSA a second time to the same Atmel device, the USB driver can lock up causing BOSSA to freeze.  As a workaround, always disconnect and reconnect the Atmel device before running BOSSA again.
* The firmware inside of SAM3U devices has a bug where non-word flash reads return zero instead of the real data.  BOSSA implements a transparent workaround for flash operations that copies flash to SRAM before reading.  Direct reads using the BOSSA shell will see the bug.
* There are reports that the USB controller in some AMD-based systems has difficulty communicating with SAM devices.  The only known workaround is to use a different, preferrably Intel-based, system.
