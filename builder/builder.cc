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
    printf("Exploit created by brad-lin\n");
    printf("Builder created by Nicolas \"Pixel\" Noble\n");
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

static uint32_t getU32(const uint8_t* ptr) {
    uint32_t ret = 0;
    ptr += 3;

    ret |= *ptr--;
    ret <<= 8;
    ret |= *ptr--;
    ret <<= 8;
    ret |= *ptr--;
    ret <<= 8;
    ret |= *ptr--;
    return ret;
}

static int16_t getHI(uint32_t v) {
    int16_t lo = v & 0xffff;
    int16_t hi = v >> 16;
    return lo < 0 ? hi + 1 : hi;
}

static int16_t getLO(uint32_t v) {
    int16_t ret = v & 0xffff;
    return ret;
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
    uint32_t tload = 0;

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

    static std::array<uint8_t, 128 * 1024> out;
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

    uint32_t tsize = in.size();
    uint32_t gp = 0;
    uint32_t pc = tload;
    uint32_t sp = 0;

    uint8_t* payloadPtr = in.data();

    if (memcmp(in.data(), "PS-X EXE", 8) == 0) {
        pc = getU32(in.data() + 0x10);
        gp = getU32(in.data() + 0x14);
        tload = getU32(in.data() + 0x18);
        tsize = getU32(in.data() + 0x1c);
        sp = getU32(in.data() + 0x30);
        sp += getU32(in.data() + 0x34);
        payloadPtr = in.data() + 2048;
    } else {
        if (!tloadStr.has_value()) {
            printf("Raw binary and no tload argument.\n");
            usage();
            return -1;
        }
    }

    if (sp == 0) sp = 0x801ffff0;
    unsigned frames = (tsize + 127) >> 7;
    constexpr unsigned availableFrames = ((128 * 1024) >> 7) - 17;
    if (frames > availableFrames) {
        printf("Payload binary too big. Maximum = %i bytes\n", availableFrames * 128);
        return -1;
    }
    memcpy(out.data() + 17 * 128, payloadPtr, tsize);

    uint32_t payload[] = {
        // restoring old vector
        Mips::Encoder::addiu(Mips::Encoder::Reg::V0, Mips::Encoder::Reg::R0, oldAddr),
        Mips::Encoder::sw(Mips::Encoder::Reg::V0, vector, Mips::Encoder::Reg::R0),

        // S0 = number of frames to load
        Mips::Encoder::addiu(Mips::Encoder::Reg::S0, Mips::Encoder::Reg::R0, frames),
        // S1 = destination address
        Mips::Encoder::lui(Mips::Encoder::Reg::S1, getHI(tload)),
        Mips::Encoder::addiu(Mips::Encoder::Reg::S1, Mips::Encoder::Reg::S1, getLO(tload)),
        // S2 = frame counter (set to 0)
        Mips::Encoder::addiu(Mips::Encoder::Reg::S2, Mips::Encoder::Reg::R0, 0),
        // read_loop:
        // A0 = deviceID (0)
        Mips::Encoder::addiu(Mips::Encoder::Reg::A0, Mips::Encoder::Reg::R0, 0),
        // A1 = frame counter + 17
        Mips::Encoder::addiu(Mips::Encoder::Reg::A1, Mips::Encoder::Reg::S2, 17),
        // A2 = current destination address pointer
        Mips::Encoder::addiu(Mips::Encoder::Reg::A2, Mips::Encoder::Reg::S1, 0),
        // increment frame counter
        Mips::Encoder::addiu(Mips::Encoder::Reg::S2, Mips::Encoder::Reg::S2, 1),
        // mcReadSector(0, frame, dest)
        Mips::Encoder::jal(0xb0),
        Mips::Encoder::addiu(Mips::Encoder::Reg::T1, Mips::Encoder::Reg::R0, 0x4f),
        // mcWaitForStatus(0);
        Mips::Encoder::addiu(Mips::Encoder::Reg::A0, Mips::Encoder::Reg::R0, 0),
        Mips::Encoder::jal(0xb0),
        Mips::Encoder::addiu(Mips::Encoder::Reg::T1, Mips::Encoder::Reg::R0, 0x5d),
        // if we still haven't reached our frame counter, go back 40 bytes (aka read_loop)
        Mips::Encoder::bne(Mips::Encoder::Reg::S0, Mips::Encoder::Reg::S2, -40),
        // increment destination pointer by sizeof(frame) in the delay slot
        Mips::Encoder::addiu(Mips::Encoder::Reg::S1, Mips::Encoder::Reg::S1, 128),

        // flush cache
        Mips::Encoder::jal(0xa0),
        Mips::Encoder::addiu(Mips::Encoder::Reg::T1, Mips::Encoder::Reg::R0, 0x44),

        // bootstrap
        Mips::Encoder::lui(Mips::Encoder::Reg::V0, getHI(pc)),
        Mips::Encoder::addiu(Mips::Encoder::Reg::V0, Mips::Encoder::Reg::V0, getLO(pc)),
        Mips::Encoder::lui(Mips::Encoder::Reg::GP, getHI(gp)),
        Mips::Encoder::addiu(Mips::Encoder::Reg::GP, Mips::Encoder::Reg::GP, getLO(gp)),
        Mips::Encoder::lui(Mips::Encoder::Reg::SP, getHI(sp)),
        Mips::Encoder::jr(Mips::Encoder::Reg::V0),
        Mips::Encoder::addiu(Mips::Encoder::Reg::SP, Mips::Encoder::Reg::SP, getLO(sp)),
    };
    static_assert(sizeof(payload) < 128, "Payload too big");
    unsigned o = 0;
    for (auto p : payload) {
        out[0x800 + o++] = p & 0xff;
        p >>= 8;
        out[0x800 + o++] = p & 0xff;
        p >>= 8;
        out[0x800 + o++] = p & 0xff;
        p >>= 8;
        out[0x800 + o++] = p & 0xff;
        p >>= 8;
    }

    FILE* outFile = fopen(outName.value().c_str(), "wb");
    if (!outFile) {
        printf("Failed to open output file %s\n", outName.value().c_str());
        return -1;
    }

    fwrite(out.data(), out.size(), 1, outFile);
    fclose(outFile);

    return 0;
}
