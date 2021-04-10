This tool can create raw memory card files for the purpose of loading any payload using this FreePSXBoot exploit.

Its usage as follow:

```
Usage: builder -base 0x801ffcd0 -vector 0x09b4 -old 0x4d3c -new 0xbe48 -in payload.bin -out card.mcd -tload 0x801b0000
       builder -base 0x801ffcd0 -vector 0x09b4 -old 0x4d3c -new 0xbe48 -in payload.ps-exe -out card.mcd

-base     the base address of the stack array being exploited from buInit
-vector   the address of the function pointer we want to modify
-old      the previous value of the function pointer
-new      the value of the function pointer we want to set
-in       the payload file; if it is a ps-exe, tload is optional
-out      the output filename to create
-tload    if 'in' is a binary payload, use this address to load and jump to
```

When looking at a random version of the kernel, `base` needs to be the base address of the stack array that's being overflowed in the function buInit. The `vector` parameter should be the address of the B0:50 entry in the B0 syscall table, for the mcAllowNewCard function that will be replaced. The `old` parameter needs to be that of the current address in the kernel of the mcAllowNewCard function. The `new` parameter needs to be that of the memory card frame reading buffer where our small exploit payload will be loaded. The `in` string is a filename for the input payload we will want to execute. This file can either be a raw binary file, or a ps-exe file. If it is a raw file, you are expected to provide the `tload` argument to indicate where the binary needs to be loaded in memory, and it is expected to also be its entry point. And finally the `out` string is the filename of the raw memory card file to create.

TODO: add presets for the various versions of the kernel.

Caveats:
- When using a ps-exe file, the BSS section won't be cleared when starting it. The crt0 of the binary needs to take care of that.
- The kernel still will be at the state of what the shell left it at, with all of the events opened and all the interrupt handlers. The ps-exe that boots needs to try and reset all this properly for a successful bootstrap.
