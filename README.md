# FreePSXBoot
Exploit allowing to load arbitrary code on the PSX (i.e. PlayStation 1) using only a memory card (no game needed).

To use it, you will need a way to copy **full memory card images** (not individual files) to a memory card. Some possibilities are:

* A PS2 and the software Memory Card Annihilator v2 (use "Restore MC image")
* [Memcarduino](https://github.com/ShendoXT/memcarduino). Requires soldering wires to the memory card.
* Using a [Memcard Pro](https://8bitmods.com/memcard-pro-for-playstation-1/), which lets you create your own virtual memory cards on an sdcard. Simply drop the card image file you want to use as Memory Card 1, Channel 1.
* Using [Unirom](https://github.com/JonathanDotCel/unirom8_bootdisc_and_firmware_for_ps1) and [NOTPSXserial](https://github.com/JonathanDotCel/NOTPSXSerial) with a serial/USB cable, using the command : `nops /fast /mcup 0 FILE.mcd COMPORT` where `FILE` is the mcd file corresponding to your model, and `COMPORT` corresponds to your computer serial port.


# WARNING
**By flashing FreePSXBoot to your Memory Card, you need to be aware of the following:**

* The .mcd image files replace the whole contents of your card, meaning that your Memory Card will be **ENTIRELY WIPED** after flashing a .mcd image, so **creating a backup of your saves is compulsory**.

* Because the exploit has **corrupt Memory Card filesystem on purpose** for it to run, your card will become **unusable for normal operations**. That is, you **won't be able to use this card for saving and loading game saves** and **it will cause crashes on your PS1 or your PS2 console** *(if you have any)*.

* Once installed, it may become difficult to uninstall, as the normal software to re-format a memory card won't work, due to the exploit itself. You could end up with no means to recover the memory card; if for example your installation method was Memory Card Annihilator v2, then it will also crash.

# Usage

* Copy the full memory card image corresponding to your model/BIOS to a memory card.
* Insert it **in slot 1**.
* Power up your PlayStation with the lid open, and go to the memory card manager.
* After a few seconds, the screen will flash orange. Wait ~30 seconds for the [Unirom](https://github.com/JonathanDotCel/unirom8_bootdisc_and_firmware_for_ps1) welcome screen to appear.
* If the screen doesn't flash orange after 20 seconds, you have either used a wrong memory card image, or your model uses an exploit which is not 100% reliable. In that case, power off your PlayStation, wait for 1 minute, and try again.
* Once Unirom is loaded, you can insert a CD, close the lid, and press **R1** to load the game.
* Don't forget to remove your memory card, as its exploit will trigger into games as well. This isn't an issue when using the Memcard Pro, as it will automatically change the virtual card to the game you're booting.

# Restoring the memory card

* The most reliable way is to use [Memcarduino](https://github.com/ShendoXT/memcarduino) and its FORMAT option.
* Some games that have a save file manager (shows the contents of the memory card before saving) built into them, like *OddWorld: Abe's Oddysee* and *Cool Boarders 4 (suffers from a caveat that keeps the game from loading the memory card with certain exploit versions)* for example, can be used to overwrite FreePSXBoot when saving progress.
* We plan to bundle a complete version of Unirom in the memory card images in the future, with the ability to format memory cards.

# Supported models

* All models are supported except SCPH-1000 and SCPH-3000, which will probably be supported in the future.
* Some models use a 100% reliable exploit, other models use an earlier version of the exploit, which depends on uninitialized memory and is therefore not 100% reliable (see caveat below).
* Certain PSOne consoles appear to not support the exploit in its entirity. We are currently checking on the issue to ensure that it is fully exploitable.
* See the table below for more details and download links.

## Changelog
2021-04-12: New version of Unirom, able to load games. Huge thanks to the psxdev contributors.
2021-04-11: 100% reliable exploit for the SCPH-7002, SCPH-7502 and SCPH-9002.

## Caveat

The earlier version of this exploit relies on uninitialized memory in kernel space to be at 0 in order to work properly. The SDRAM chips have a fairly slow decay rate, and this exploit will only be reliable if the machine has been powered off for long enough.

[Technical details](exploit/EXPLOIT.md)

[Demonstration](https://www.youtube.com/watch?v=29DI-N45V40)

## Downloads
These images are pre-built with Unirom. There are two versions: standard, which has ~30 seconds load time, and reduced, which has ~20 seconds load time.

There are different downloads for different console versions. Please download the correct ROM for your model of PlayStation. If a version is missing, it hasn't been added yet.

As more reliable versions of the exploit are developed, the images are updated.

Links marked with **old** are images which cannot load games yet.

| Model     | BIOS Version | 100% reliable exploit? | Standard Download Link |
|-----------|--------------|------------------------|------------------------|
| SCPH-1000 | 1.0          | N/A | WIP |
| SCPH-1001 | 2.2          | No | [Old](exploit/freepsxboot-unirom-standard-bios3.x.mcd) |
| SCPH-1002 | 2.0/2.1/2.2  | No | [Old](exploit/freepsxboot-unirom-standard-bios3.x.mcd) |
| SCPH-3000 | 1.1          | N/A | WIP |
| SCPH-3500 | 2.1          | No | [Old](exploit/freepsxboot-unirom-standard-bios3.x.mcd) |
| SCPH-5001 | 3.0          | No | [20210412](images/freepsxboot-unirom-20210412-5001-5501-5503-7003.mcd) |
| SCPH-5500 | 3.0          | No | [Old](exploit/freepsxboot-unirom-standard-bios3.x.mcd) |
| SCPH-5501 | 3.0          | No | [20210412](images/freepsxboot-unirom-20210412-5001-5501-5503-7003.mcd) |
| SCPH-5502 | 3.0          | No | [20210412](images/freepsxboot-unirom-20210412-5502-5552.mcd) |
| SCPH-5503 | 3.0          | No | [20210412](images/freepsxboot-unirom-20210412-5001-5501-5503-7003.mcd) |
| SCPH-5552 | 3.0          | No | [20210412](images/freepsxboot-unirom-20210412-5502-5552.mcd) |
| SCPH-5903 | 2.2          | No | [Old](exploit/freepsxboot-unirom-standard-bios3.x.mcd) |
| SCPH-7000 | 4.0          | No | [Old](exploit/freepsxboot-unirom-standard-bios4.x.mcd) |
| SCPH-7000W | 4.1         | No | [Old](exploit/freepsxboot-unirom-standard-bios4.x.mcd) |
| SCPH-7001 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-7002 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-7003 | 3.0          | No | [20210412](images/freepsxboot-unirom-20210412-5001-5501-5503-7003.mcd) |
| SCPH-7500 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-7501 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-7502 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-7503 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-9000 | 4.0          | No | [Old](exploit/freepsxboot-unirom-standard-bios4.x.mcd) |
| SCPH-9001 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-9002 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-9003 | 4.1          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-7001-7002-7500-7501-7502-7503-9001-9002-9003.mcd) |
| SCPH-100  | 4.3          | No | [Old](exploit/freepsxboot-unirom-standard-psone.mcd) |
| SCPH-101  | 4.5          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-101.mcd) |
| SCPH-102  | 4.4          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-102_4.4.mcd) |
| SCPH-102  | 4.5          | **Yes** | [20210412](images/freepsxboot-unirom-20210412-102_4.5.mcd) |

See the folder [builder](builder) for a tool that can be used to generate your own payloads and memory cards.

Memory card images are raw data: your memory card must have the exact same content as the files. Use Memcarduino or something similar; don't use a memory card file manager, as it will try to correct the data we're altering.

If the exploit is successful, you will see the screen flashing orange. Otherwise, power cycle your PSX and try again after a minute or so. It may take a few tries.

The exploit works in emulators as well, and works all the time due to the memory being always initialized to 0. Tested with no$psx and [pcsx-redux](https://github.com/grumpycoders/pcsx-redux/).
