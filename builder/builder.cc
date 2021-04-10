#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#include <array>
#include <string>
#include <vector>

#include "encoder.hh"
#include "flags.h"

static void banner() {
    printf("FreePSXBoot memory card builder\n");
    printf("https://github.com/brad-lin/FreePSXBoot\n\n");
}

static void usage() {
    printf(
        "Usage: builder -base 0x801ffcd0 -vector 0x09b4 -old 0x4d3c -new 0xbe48 -in payload.bin -out card.mcd -tload "
        "0x801b0000\n");
    printf("       builder -base 0x801ffcd0 -vector 0x09b4 -old 0x4d3c -new 0xbe48 -in payload.ps-exe -out card.mcd\n");
    printf("\n");
    printf("-base     the base address of the stack array being exploited from buInit\n");
    printf("-vector   the address of the function pointer we want to modify\n");
    printf("-old      the previous value of the function pointer\n");
    printf("-new      the value of the function pointer we want to set\n");
    printf("-in       the payload file; if it is a ps-exe, tload is optional\n");
    printf("-out      the output filename to create\n");
    printf("-tload    if 'in' is a binary payload, use this address to load and jump to\n");
}

int main(int argc, char** argv) {
    banner();

    const flags::args args(argc, argv);

    auto baseStr = args.get<std::string>("base");
    auto vectorStr = args.get<std::string>("vector");
    auto oldAddrStr = args.get<std::string>("old");
    auto newAddrStr = args.get<std::string>("new");
    auto inName = args.get<std::string>("in");
    auto outName = args.get<std::string>("out");
    auto tloadStr = args.get<std::string>("tload");

    if (!baseStr.has_value() || !vectorStr.has_value() || !oldAddrStr.has_value() || !newAddrStr.has_value() ||
        !inName.has_value() || !outName.has_value()) {
        usage();
        return -1;
    }

    uint32_t base;
    uint32_t vector;
    uint32_t oldAddr;
    uint32_t newAddr;
    uint32_t tload;

    const char* argname = nullptr;
    try {
        argname = "base";
        base = std::stoul(baseStr.value(), nullptr, 0);
        argname = "vector";
        vector = std::stoul(vectorStr.value(), nullptr, 0);
        argname = "oldAddr";
        oldAddr = std::stoul(oldAddrStr.value(), nullptr, 0);
        argname = "newAddr";
        newAddr = std::stoul(newAddrStr.value(), nullptr, 0);
        if (tloadStr.has_value()) {
            argname = "tload";
            tload = std::stoul(tloadStr.value(), nullptr, 0);
        }
    } catch (...) {
        printf("Error parsing argument %s\n", argname);
        return -1;
    }

    std::array<uint8_t, 128 * 1024> out;
    memset(out.data(), 0, out.size());
    out[0x0000] = 'M';
    out[0x0001] = 'C';
    out[0x007f] = 0x0e;
    out[0x0080] = 0x51;

    uint32_t increment = (newAddr - oldAddr) * 0x4000;
    out[0x0084] = increment & 0xff;
    increment >>= 8;
    out[0x0085] = increment & 0xff;
    increment >>= 8;
    out[0x0086] = increment & 0xff;
    increment >>= 8;
    out[0x0087] = increment & 0xff;

    vector &= 0x001fffff;
    vector |= 0x00200000;
    base &= 0x001fffff;

    uint32_t index = (vector - base) >> 2;
    out[0x0088] = index & 0xff;
    index >>= 8;
    out[0x0089] = index & 0xff;
    out[0x008a] = 'F';
    out[0x008b] = 'r';
    out[0x008c] = 'e';
    out[0x008d] = 'e';
    out[0x008e] = 'P';
    out[0x008f] = 'S';
    out[0x0090] = 'X';
    out[0x0091] = 'B';
    out[0x0092] = 'o';
    out[0x0093] = 'o';
    out[0x0094] = 't';

    uint8_t crc = 0;
    for (unsigned i = 0; i < 0x80; i++) {
        crc ^= out[0x0080 + i];
    }
    out[0x00ff] = crc;

    #if 0
    FILE* inFile = fopen(inName.value().c_str(), "rb");
    if (!inFile) {
        printf("Failed to open input file %s\n", inName.value().c_str());
        return -1;
    }

    fseek(inFile, 0, SEEK_END);
    std::vector<uint8_t> in;
    in.resize(ftell(inFile));
    fseek(inFile, 0, SEEK_SET);
    auto n = fread(in.data(), in.size(), 1, inFile);
    if (n != 1) {
        printf("Failed to read input file %s\n", inName.value().c_str());
        return -1;
    } 
    fclose(inFile);
    #endif

    FILE* outFile = fopen(outName.value().c_str(), "wb");
    if (!outFile) {
        printf("Failed to open output file %s\n", outName.value().c_str());
        return -1;
    }

    fwrite(out.data(), out.size(), 1, outFile);
    fclose(outFile);

    return 0;
}
