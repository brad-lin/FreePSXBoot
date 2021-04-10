# FreePSXBoot
Exploit allowing to load arbitrary code on the PSX using only a memory card (no game needed)

## Caveat
The current version of this exploit relies on uninitialized memory in kernel space to be at 0 in order to work properly. The SDRAM chips have a fairly slow decay rate, and this exploit will currently only be reliable if the machine has been powered off for long enough.

**ONLY FOR SCPH-9002 AT THE MOMENT**

[Technical details](exploit/EXPLOIT.md)

[Memory card image with Unirom (standard size, ~30 seconds load time)](exploit/freepsxboot-unirom-standard.mcd)

[Memory card image with Unirom (reduced size, ~20 seconds load time)](exploit/freepsxboot-unirom-reduced.mcd)

[Demonstration](https://www.youtube.com/watch?v=29DI-N45V40)

See the folder [builder](builder) for a tool that can be used to generate your own payloads and memory cards.

Memory card images are raw data: your memory card must have the exact same content as the files. Use Memcarduino or something similar; don't use a memory card file manager, as it will try to correct the data we're altering.

If the exploit is successful, you will see the screen flashing orange. Otherwise, power cycle your PSX and try again after a minute or so. It may take a few tries.

The exploit works in emulators as well, and works all the time due to the memory being always initialized to 0. Tested with no$psx and [pcsx-redux](https://github.com/grumpycoders/pcsx-redux/).
