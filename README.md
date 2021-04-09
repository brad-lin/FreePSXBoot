# FreePSXBoot
Exploit allowing to load arbitrary code on the PSX using only a memory card (no game needed)

**ONLY FOR SCPH-9002 AT THE MOMENT**

[Technical details](EXPLOIT.md)

[Memory card image with Unirom (standard size, ~30 seconds load time)](freepsxboot-unirom-standard.mcd)

[Memory card image with Unirom (reduced size, ~20 seconds load time)](freepsxboot-unirom-reduced.mcd)

[Demonstration](https://www.youtube.com/watch?v=29DI-N45V40)

Memory card images are raw data: your memory card must have the exact same content as the files. Use Memcarduino or something similar; don't use a memory card file manager (it won't work).

If the exploit is successful, you will see the screen flashing orange. Otherwise, power cycle your PSX and try again. It may take a few tries.


The exploit works in emulators as well (tested with no$psx).
