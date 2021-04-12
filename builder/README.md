This program can build a memory card using known exploit values for supported model versions, or build an experimental memory card which gives full control of the value modified by the exploit. If you just want to run something on a PlayStation with a supported model or BIOS version, use -model or -bios with the corresponding value (e.g. -model 9002 or -bios 4.4). If you know what you are doing and want to experiment, use the advanced options.

Its usage as follow:

```
Usage:
        To run a PS-EXE payload (standard usage):
                builder -model 9002 -in payload.exe -out card.mcd
        To run a raw payload (advanced usage):
                builder -model 9002 -in payload.bin -out card.mcd -tload 0x801b0000
        To experiment (expert usage; these example values work on most 7000+ model versions):
                builder -base 0x801ffcd0 -vector 0x802009b4 -old 0x4d3c -new 0xbe48 -in payload.bin -out card.mcd -tload 0x801b0000

-model    the model version, as 3 or 4 digits (e.g. 9002). If you use this option, don't use base, vector, old, and new.
-bios     the BIOS version, as X.Y (e.g. 4.4). If you use this option, don't use base ,vector, old, and new.
-base     the base address of the stack array being exploited from buInit
-vector   the address of the value we want to modify. Use 0x802 as a prefix, e.g. 0x802009b4 to modify value at address 0x09b4.
-old      the original value to modify
-new      the new value we want to set
-in       the payload file; if it is a ps-exe, tload is optional
-out      the output filename to create
-tload    if 'in' is a binary payload, use this address to load and jump to
-return   make a payload that allows returning to the shell
-norestore do not restore the value overwritten by the exploit
-noint    disable interrupts during payload
-nogp     disable setting $gp during payloads
-deleted  use deleted fake entries
```

When looking at a random version of the kernel, `base` needs to be the base address of the stack array that's being overflowed in the function buInit. The `vector` parameter should be the address of the B0:50 entry in the B0 syscall table, for the mcAllowNewCard function that will be replaced. The `old` parameter needs to be that of the current address in the kernel of the mcAllowNewCard function. The `new` parameter needs to be that of the memory card frame reading buffer where our small exploit payload will be loaded. The `in` string is a filename for the input payload we will want to execute. This file can either be a raw binary file, or a ps-exe file. If it is a raw file, you are expected to provide the `tload` argument to indicate where the binary needs to be loaded in memory, and it is expected to also be its entry point. And finally the `out` string is the filename of the raw memory card file to create.

Caveats:
- When using a ps-exe file, the BSS section won't be cleared when starting it. The crt0 of the binary needs to take care of that.
- The kernel still will be at the state of what the shell left it at, with all of the events opened and all the interrupt handlers. The ps-exe that boots needs to try and reset all this properly for a successful bootstrap.
