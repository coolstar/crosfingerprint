# crosfingerprint
Fingerprint driver for Intel and AMD Chromebooks

Tested chromebooks:
* Pixel Slate (Intel 8th gen - SPI)
* HP Elite c1030 (Intel 10th gen - SPI)
* HP Chromebook c645 Enterprise (AMD Ryzen 3000 - UART)

Although this driver is UMDF (so it can run without testsigning), it will need to be signed to actually function with Windows Hello.

This driver is open sourced in the hope that someone figures out how to sign it.

Notes for WHQL verification:
* Fingerprint Data is encrypted by the Chrome MCU firmware (proprietary and provided by Google. Not possible to compile your own)
* Chrome MCU doesn't prevent enrolling the same finger twice (so can't report duplicates)
* Chrome MCU doesn't report bad reads on match, and instead reports it as "no match"

WHQL WBDI tests are expected to pass 100% of sequence and sensor tests, but only 97.37% of storage and 91.67% engine.
* 1 storage test fails due to duplicates
* 2 engine tests fail due to duplicates
* 2 engine tests fail due to no bad reads
