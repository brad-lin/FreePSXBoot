#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#include <array>
#include <string>
#include <vector>
#include <unordered_map>

#include "encoder.hh"
#include "flags.h"

struct ExploitSettings {
    uint32_t stackBase;
    uint32_t addressToModify;
    uint32_t originalValue;
    uint32_t newValue;

    constexpr static uint32_t maxFileSizeToUse = 0x7fffc000;
    constexpr static uint32_t maxIncrementPerSlot = 0x1ffff;

    uint16_t index() const noexcept {
        return static_cast<uint16_t>((addressToModify - stackBase) / 4);
    }

    uint32_t difference() const noexcept {
        return newValue - originalValue;
    }

    int nbSlotsNeeded() const noexcept {
        // Each slot allows to increment at most by 0x1ffff.
        const uint32_t diff = difference();
        int result = (diff / maxIncrementPerSlot);
        result += (diff % maxIncrementPerSlot) == 0 ? 0 : 1;
        return result;
    }

    uint32_t fileSizeToUseForLastSlot() const noexcept {
        return (difference() % maxIncrementPerSlot) * 0x4000;
    }

    bool valid() const noexcept {
        // Index must be <= 0xfffe.
        if ((addressToModify - stackBase) / 4 > 0xfffe) {
            return false;
        }
        // 15 slots are usable to increment the value.
        else if (nbSlotsNeeded() > 15) {
            return false;
        } else {
            return true;
        }
    }
};

// Maps BIOS version (e.g. 9002) to its exploits settings.
static std::unordered_map<uint32_t, ExploitSettings> biosExploitSettings = {
    // Overwrite "jal set_card_auto_format", called right after buInit
    {9002, {0x801ffcd0, 0x80231f50, 0x0c01a144, 0x0c082f92}}
};

static void banner() {
    printf("FreePSXBoot memory card builder\n");
    printf("Exploit created by brad-lin\n");
    printf("Builder created by Nicolas \"Pixel\" Noble\n");
    printf("https://github.com/brad-lin/FreePSXBoot\n\n");
}

static void usage() {
    printf(
        "This program can build a memory card using known exploit values for supported BIOS versions,\n"
        "or build an experimental memory card which gives full control of the value modified by the exploit.\n"
        "If you just want to run something on a PlayStation with a supported BIOS version,\n"
        "use -bios with the corresponding option.\n"
        "If you know what you are doing and want to experiment, use the advanced options.\n");
    printf(
        "Usage:\n"
        "\tTo run a payload (standard usage) :\n"
        "\t\tbuilder -bios 9002 -in payload.bin -out card.mcd -tload 0x801b0000\n"
        "\tTo experiment (advanced usage; these example values work on 7000+ BIOS versions):\n"
        "\t\tbuilder -base 0x801ffcd0 -vector 0x802009b4 -old 0x4d3c -new 0xbe48 -in payload.bin -out card.mcd -tload "
        "0x801b0000\n");
    printf("\n");
    printf("-bios     the BIOS version, as 3 or 4 digits (e.g. 9002). If you use this option, don't use base, vector, old, and new.\n");
    printf("-base     the base address of the stack array being exploited from buInit\n");
    printf("-vector   the address of the value we want to modify. Use 0x802 as a prefix, e.g. 0x802009b4 to modify value at address 0x09b4.\n");
    printf("-old      the original value to modify\n");
    printf("-new      the new value we want to set\n");
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

    auto biosVersionStr = args.get<std::string>("bios");
    auto baseStr = args.get<std::string>("base");
    auto vectorStr = args.get<std::string>("vector");
    auto oldAddrStr = args.get<std::string>("old");
    auto newAddrStr = args.get<std::string>("new");
    auto inName = args.get<std::string>("in");
    auto outName = args.get<std::string>("out");
    auto tloadStr = args.get<std::string>("tload");

    const bool biosVersionPresent = biosVersionStr.has_value();
    const bool anyExpertArgPresent = baseStr.has_value() || vectorStr.has_value() || oldAddrStr.has_value() || newAddrStr.has_value();
    const bool allExpertArgsPresent = baseStr.has_value() && vectorStr.has_value() && oldAddrStr.has_value() && newAddrStr.has_value();
    // Check in and out presence
    if (!inName.has_value() || !outName.has_value()) {
        usage();
        return -1;
    }
    // Check that none or all expert args are present
    if (anyExpertArgPresent && !allExpertArgsPresent) {
        usage();
        return -1;
    }
    // Check that bios version xor expert args are used
    if (biosVersionPresent && anyExpertArgPresent) {
        usage();
        return -1;
    }

    uint32_t biosVersion;
    uint32_t base;
    uint32_t vector;
    uint32_t oldAddr;
    uint32_t newAddr;
    uint32_t tload = 0;

    const char* argname = nullptr;
    try {
        if (biosVersionPresent) {
            argname = "bios";
            biosVersion = std::stoul(biosVersionStr.value(), nullptr, 0);
        } else {
            argname = "base";
            base = std::stoul(baseStr.value(), nullptr, 0);
            argname = "vector";
            vector = std::stoul(vectorStr.value(), nullptr, 0);
            argname = "oldAddr";
            oldAddr = std::stoul(oldAddrStr.value(), nullptr, 0);
            argname = "newAddr";
            newAddr = std::stoul(newAddrStr.value(), nullptr, 0);
        }
        if (tloadStr.has_value()) {
            argname = "tload";
            tload = std::stoul(tloadStr.value(), nullptr, 0);
        }
    } catch (...) {
        printf("Error parsing argument %s\n", argname);
        return -1;
    }

    struct ExploitSettings exploitSettings;
    if (biosVersionPresent) {
        auto it = biosExploitSettings.find(biosVersion);
        if (it == biosExploitSettings.end()) {
            printf("Unsupported BIOS version %u\n", biosVersion);
            return -1;
        }
        exploitSettings = it->second;
    } else {
        exploitSettings.stackBase = base;
        exploitSettings.addressToModify = vector;
        exploitSettings.originalValue = oldAddr;
        exploitSettings.newValue = newAddr;
    }

    if (!exploitSettings.valid()) {
        printf("Exploit settings are not valid; either too many slots are needed, or the address to modify is out of range.\n");
        return -1;
    }

    static std::array<uint8_t, 128 * 1024> out;
    memset(out.data(), 0, out.size());
    out[0x0000] = 'M';
    out[0x0001] = 'C';
    out[0x007f] = 0x0e;

    // Write directory entries (as many as needed)
    for (int i = 0; i < exploitSettings.nbSlotsNeeded(); i++) {
        const int offset = (i + 1) * 0x80;
        const bool last = i == exploitSettings.nbSlotsNeeded() - 1;
        uint32_t fileSize = last ? exploitSettings.fileSizeToUseForLastSlot() : ExploitSettings::maxFileSizeToUse;
        // Note: exploit will fail if the value at the fake directory entry is 0xAX or 0x5X, depending of this value.
        // If it fails, change to 0xA1.
        out[offset] = 0x51;

        out[offset + 4] = fileSize & 0xff;
        fileSize >>= 8;
        out[offset + 5] = fileSize & 0xff;
        fileSize >>= 8;
        out[offset + 6] = fileSize & 0xff;
        fileSize >>= 8;
        out[offset + 7] = fileSize & 0xff;

        uint32_t index = exploitSettings.index();
        out[offset + 8] = index & 0xff;
        index >>= 8;
        out[offset + 9] = index & 0xff;
        out[offset + 0xa] = 'F';
        out[offset + 0xb] = 'r';
        out[offset + 0xc] = 'e';
        out[offset + 0xd] = 'e';
        out[offset + 0xe] = 'P';
        out[offset + 0xf] = 'S';
        out[offset + 0x10] = 'X';
        out[offset + 0x11] = 'B';
        out[offset + 0x12] = 'o';
        out[offset + 0x13] = 'o';
        out[offset + 0x14] = 't';

        uint8_t crc = 0;
        for (unsigned i = 0; i < 0x80; i++) {
            crc ^= out[offset + i];
        }
        out[offset + 0x7f] = crc;
    }

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

    //TODO: option to keep stack pointer as is, so that the shell can resume its execution.
    uint32_t payload[] = {
        // disabling all interrupts except for memory card
        Mips::Encoder::addiu(Mips::Encoder::Reg::V0, Mips::Encoder::Reg::R0, 128),
        Mips::Encoder::lui(Mips::Encoder::Reg::V1, 0x1f00),
        Mips::Encoder::sw(Mips::Encoder::Reg::V0, 0x1074, Mips::Encoder::Reg::V1),

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
    //TODO: ensure that checksum of payload sector is wrong (if it is good, the BIOS will erase it with data from the next sector).
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
