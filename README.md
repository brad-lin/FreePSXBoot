# FreePSXBoot
Exploit to allow loading arbitrary code on the PSX (i.e. PlayStation 1) using only a memory card (no game needed).

In other words, it's a softmod which requires a memory card, and a way to write raw data to it.

For support or discussion about this exploit and associated tools, you may join the PSXDev Network Discord server: [![Discord](https://img.shields.io/discord/642647820683444236)](https://discord.gg/QByKPpH)

To use it, you will need a way to copy **full memory card images** (not individual files) to a memory card. Some possibilities are:

* A PS2 and the software Memory Card Annihilator v2 (use "Restore MC image")
* [Memcarduino](https://github.com/ShendoXT/memcarduino). Requires soldering wires to the memory card.
* Using a [Memcard Pro](https://8bitmods.com/memcard-pro-for-playstation-1/), which lets you create your own virtual memory cards on an sdcard. Simply drop the card image file you want to use as Memory Card 1, Channel 1.
* Using [Unirom](https://github.com/JonathanDotCel/unirom8_bootdisc_and_firmware_for_ps1) and [NOTPSXserial](https://github.com/JonathanDotCel/NOTPSXSerial) with a serial/USB cable, using the command : `nops /fast /mcup 0 FILE.mcd COMPORT` where `FILE` is the mcd file corresponding to your model, and `COMPORT` corresponds to your computer serial port.
* [Unirom bootdisc](https://github.com/JonathanDotCel/unirom8_bootdisc_and_firmware_for_ps1) can be used with the [disc swap trick](https://www.instructables.com/How-to-Disc-Swap-on-PS1-or-PSX/) to install FreePSXBoot on a memory card.
* [MemcardRex](https://github.com/ShendoXT/memcardrex/releases/tag/2b7a018) with a DexDrive, Memcarduino, PS3 Memory Card Adaptor (CECHZM1), or PS1CardLink. Make sure to use the latest release, which allows to write raw data to the memory card (required for FreePSXBoot).
* [ps3mca-ps1](https://github.com/paolo-caroni/ps3mca-ps1) with PS3 Memory Card Adaptor (CECHZM1).
* PSXGameEdit with a DexDrive (success not guaranteed)

# Slot 1 or slot 2?

FreePSXBoot memory card images are provided for slot 1 or slot 2. The memory card must be plugged in the corresponding slot on the PlayStation for FreePSXBoot to work. **It is recommended to use images for slot 2**, as they allow the FreePSXBoot memory card to remain plugged in while using another memory card in slot 1 for normal operations.

When launched from a memory card in slot 2, FreePSXBoot patches the PlayStation kernel to disable the memory card in slot 2, ensuring that the exploit doesn't trigger while playing a game. This means that the memory card doesn't have to be unplugged before launching a game.

If you are using a Memcard Pro, or need to use the two memory card slots normally, use slot 1 images instead.

**In short: use the memory card images for slot 2, unless you have a Memcard Pro.**

# WARNING
**By flashing FreePSXBoot to your Memory Card, you need to be aware of the following:**

* The .mcd image files replace the whole contents of your card, meaning that your Memory Card will be **ENTIRELY WIPED** after flashing a .mcd image, so **creating a backup of your saves is compulsory**.

* Because the exploit has **corrupt Memory Card filesystem on purpose** for it to run, your card will become **unusable for normal operations**. That is, you **won't be able to use this card for saving and loading game saves** and **it will cause crashes on your PS1 or your PS2 console** *(if you have any)*.

* Once installed, it may become difficult to uninstall, as the normal software to re-format a memory card won't work, due to the exploit itself. You could end up with no means to recover the memory card; if for example your installation method was Memory Card Annihilator v2, then it will also crash. [Memcarduino](https://github.com/ShendoXT/memcarduino), [Unirom](https://github.com/JonathanDotCel/unirom8_bootdisc_and_firmware_for_ps1), or using the [Memcard Pro](https://8bitmods.com/memcard-pro-for-playstation-1/) would currently be safe bets.

# Usage

* Copy the full memory card image corresponding to your model/BIOS to a memory card.
* Insert it in the correct slot (images for slot 2 are strongly recommended, unless you have a Memcard pro).
* Power up your PlayStation with the lid open, and go to the memory card manager.
* After a few seconds, the screen will be filled with cyan. Wait ~30 seconds for the [Unirom](https://github.com/JonathanDotCel/unirom8_bootdisc_and_firmware_for_ps1) welcome screen to appear.
* If the cyan screen doesn't appear, you have either used a wrong memory card image, or the memory card image was not written properly (the mcd file must be written as raw data to the memory card), or something else went wrong. If you are 100% certain that the memory card image was written properly, and that you are using the correct image, please open an issue.
* Once Unirom is loaded, you can insert a CD, close the lid, and press **R1** to load the game; for more information see [Unirom usage](https://unirom.github.io/usage/#mod-free-booting). Note: Japanese PlayStation cannot have their CD drive unlocked by Unirom, and need a specific manipulation to load backups.
* **Only if you are using a slot 1 image and don't have a Memcard Pro**: remove your memory card, as its exploit will trigger into games as well.

# Restoring the memory card

* Use [MemcardRex](https://github.com/ShendoXT/memcardrex/releases/tag/2b7a018).
* Use [ps3mca-ps1](https://github.com/paolo-caroni/ps3mca-ps1) with PS3 Memory Card Adaptor (CECHZM1).
* Some games that have a save file manager (shows the contents of the memory card before saving) built into them, like *OddWorld: Abe's Oddysee* and *Cool Boarders 4 (suffers from a caveat that keeps the game from loading the memory card with certain exploit versions)* for example, can be used to overwrite FreePSXBoot when saving progress.
* Memory Card Annihilator v2 may be able to format a card, but it has to be inserted at the last moment. This method is not guaranteed to work.
* In general, tools and games crash when attempting to format a memory card loaded with FreePSXBoot, but may be able to format it by first inserting a normal memory card, and switching it with the FreePSXBoot memory card just before the format operation starts.
* Unirom's memory card manager can format memory cards.

# Supported models

* All models are supported and tested on emulator or real hardware, except the debug models (DTL-H) and Net Yaroze (they may work, but are not tested).
* As of version 20210419, the exploit is 100% reliable on all supported models. Nevertheless, some exploit images were only tested on emulators and may not work on real hardware; feedback is welcome.
* See the table below for more details and download links.

# Troubleshooting

* It is recommended to use a normal game pad when running the exploit. Special game pads may interfere with the exploit and prevent it to run.
* Some third party memory cards cannot be written properly with a slim PS2, because they need the 7.5V line to work.
* Some models (e.g. SCPH-102) may have different BIOS versions. Either try them one by one, or use the Unirom CD (with the disc swap trick) to get your BIOS version.

## Changelog

* 2022-04-07: Duplicate payload in images to support fake memcards with 512 kb chips
* 2022-03-18: New kernel patch for slot 2; should fix games which can't handle wrong memory card handshake
* 2021-10-04: Unirom version updated to 8.0.J
* 2021-07-03: FreePSXBoot patches the kernel when run from slot 2 (memory card can remain inserted)
* 2021-07-03: Exploit updated to work on slot 2
* 2021-07-03: Unirom version updated to 8.0.I
* 2021-05-05: Unirom version updated to 8.0.H
* 2021-04-28: Fixed exploit for BIOS versions 2.1 (A) and 2.2 (A) (read dummy frame to avoid errors)
* 2021-04-27: Added support for more BIOS versions, which are now identified by their CRC32
* 2021-04-22: Added support for BIOS 2.0 (1995-05-07), 4.0 and 4.1 (1997-11-14)
* 2021-04-21: Added support for BIOS 1.1, and fixed BIOS 2.0 exploit (needs icache flush to work)
* 2021-04-21: Progress bar added in stage2 payload (thanks Nicolas Noble)
* 2021-04-20: Added support for BIOS 3.0 1996-09-09 (SCPH-5500) (thanks sickle)
* 2021-04-19: Added support for BIOS 1.0 and 4.3 (SCPH-1000 and SCPH-100 respectively)
* 2021-04-19: Exploit 100% reliable for every supported BIOS; now hooks an ISR (thanks sickle)
* 2021-04-19: Unirom version updated to 8.0.F
* 2021-04-14: Exploit uses fastload, which reads the memory card much faster than Sony's code (thanks Nicolas Noble)
* 2021-04-12: New version of Unirom, able to load games. Huge thanks to the psxdev contributors.
* 2021-04-11: 100% reliable exploit for the SCPH-7002, SCPH-7502 and SCPH-9002.

## Information

[Technical details](exploit/EXPLOIT.md)

[Demonstration](https://www.youtube.com/watch?v=29DI-N45V40)

## Downloads
These images are pre-built with Unirom.

There are different downloads for different BIOS versions. Please download the correct ROM for your BIOS version. If a model or BIOS version is missing, it means it is not supported yet.

As more reliable or faster versions of the exploit are developed, the images are updated. Older versions can be found in the `images` directory.

| BIOS version/region | BIOS CRC32 | Models | 100% reliable | Download Link Slot 2 (recommended) | Download Link Slot 1 (For Memcard Pro) |
|---------------------|------------|--------|---------------|------------------------------------|----------------------------------------|
| 1.0 (1994-09-22) I | 3b601fc8 | SCPH-1000 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-1.0-1994-09-22-I-3b601fc8-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-1.0-1994-09-22-I-3b601fc8-slot1.mcd) |
| 1.1 (1995-01-22) I | 3539def6 | SCPH-3000 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-1.1-1995-01-22-I-3539def6-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-1.1-1995-01-22-I-3539def6-slot1.mcd) |
| 2.0 (1995-05-07) A | 55847d8c | SCPH-1001 | **Yes; see note below** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.0-1995-05-07-A-55847d8c-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.0-1995-05-07-A-55847d8c-slot1.mcd) |
| 2.0 (1995-05-10) E | 9bb87c4b | SCPH-1002 | **Yes; see note below** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.0-1995-05-10-E-9bb87c4b-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.0-1995-05-10-E-9bb87c4b-slot1.mcd) |
| 2.1 (1995-07-17) A | aff00f2f | SCPH-1001 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.1-1995-07-17-A-aff00f2f-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.1-1995-07-17-A-aff00f2f-slot1.mcd) |
| 2.1 (1995-07-17) E | 86c30531 | SCPH-1002 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.1-1995-07-17-E-86c30531-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.1-1995-07-17-E-86c30531-slot1.mcd) |
| 2.1 (1995-07-17) I | bc190209 | SCPH-3500 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.1-1995-07-17-I-bc190209-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.1-1995-07-17-I-bc190209-slot1.mcd) |
| 2.2 (1995-12-04) A | 37157331 | SCPH-1001<br/>SCPH-5003 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.2-1995-12-04-A-37157331-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.2-1995-12-04-A-37157331-slot1.mcd) |
| 2.2 (1995-12-04) E | 1e26792f | SCPH-1002 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.2-1995-12-04-E-1e26792f-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.2-1995-12-04-E-1e26792f-slot1.mcd) |
| 2.2 (1995-12-04) I | 24fc7e17 | SCPH-5000 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-2.2-1995-12-04-I-24fc7e17-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-2.2-1995-12-04-I-24fc7e17-slot1.mcd) |
| 3.0 (1996-09-09) I | ff3eeb8c | SCPH-5500 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-3.0-1996-09-09-I-ff3eeb8c-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-3.0-1996-09-09-I-ff3eeb8c-slot1.mcd) |
| 3.0 (1996-11-18) A | 8d8cb7e4 | SCPH-5001<br/>SCPH-5501<br/>SCPH-5503<br/>SCPH-7003 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-3.0-1996-11-18-A-8d8cb7e4-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-3.0-1996-11-18-A-8d8cb7e4-slot1.mcd) |
| 3.0 (1997-01-06) E | d786f0b9 | SCPH-5502<br/>SCPH-5552 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-3.0-1997-01-06-E-d786f0b9-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-3.0-1997-01-06-E-d786f0b9-slot1.mcd) |
| 4.0 (1997-08-18) I | ec541cd0 | SCPH-7000<br/>SCPH-7500<br/>SCPH-9000 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.0-1997-08-18-I-ec541cd0-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.0-1997-08-18-I-ec541cd0-slot1.mcd) |
| 4.1 (1997-11-14) A | b7c43dad | SCPH-7000W | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.1-1997-11-14-A-b7c43dad-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.1-1997-11-14-A-b7c43dad-slot1.mcd) |
| 4.1 (1997-12-16) A | 502224b6 | SCPH-7001<br/>SCPH-7501<br/>SCPH-7503<br/>SCPH-9001<br/>SCPH-9003<br/>SCPH-9903 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.1-1997-12-16-A-502224b6-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.1-1997-12-16-A-502224b6-slot1.mcd) |
| 4.1 (1997-12-16) E | 318178bf | SCPH-7002<br/>SCPH-7502<br/>SCPH-9002 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.1-1997-12-16-E-318178bf-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.1-1997-12-16-E-318178bf-slot1.mcd) |
| 4.3 (2000-03-11) I | f2af798b | SCPH-100 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.3-2000-03-11-I-f2af798b-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.3-2000-03-11-I-f2af798b-slot1.mcd) |
| 4.4 (2000-03-24) A | 6a0e22a0 | SCPH-101 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.4-2000-03-24-A-6a0e22a0-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.4-2000-03-24-A-6a0e22a0-slot1.mcd) |
| 4.4 (2000-03-24) E | 0bad7ea9 | SCPH-102 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.4-2000-03-24-E-0bad7ea9-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.4-2000-03-24-E-0bad7ea9-slot1.mcd) |
| 4.5 (2000-05-25) A | 171bdcec | SCPH-101<br/>SCPH-103 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.5-2000-05-25-A-171bdcec-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.5-2000-05-25-A-171bdcec-slot1.mcd) |
| 4.5 (2000-05-25) E | 76b880e5 | SCPH-102 | **Yes** | [20220318 Slot 2](images/slot2/freepsxboot-unirom-fastload-20220318-bios-4.5-2000-05-25-E-76b880e5-slot2.mcd) | [20211004 Slot 1](images/slot1/freepsxboot-unirom-fastload-20211004-bios-4.5-2000-05-25-E-76b880e5-slot1.mcd) |

**Note for BIOS 2.0 (SCPH-1001 or SCPH-1002) slot 1 only**: the memory card containing FreePSXBoot must be inserted in slot 1, and **another memory card must be present in slot 2**. The memory card in slot 2 can have any content.

See the folder [builder](builder) for a tool that can be used to generate your own payloads and memory cards.

The exploit works in emulators as well. Tested with no$psx, [pcsx-redux](https://github.com/grumpycoders/pcsx-redux/), and [DuckStation](https://github.com/stenzek/duckstation/).
