# crosfingerprint
Fingerprint driver for Intel and AMD Chromebooks

Tested chromebooks:
* Pixel Slate (Intel 8th gen - SPI)
* HP Elite c1030 (Intel 10th gen - SPI)
* HP Chromebook c645 Enterprise (AMD Ryzen 3000 - UART)

Although this driver is UMDF (so it can run without testsigning), it will need to be signed to actually function with Windows Hello.

This driver is open sourced in the hope that someone figures out how to sign it.
