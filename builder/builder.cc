#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#include <array>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "flags.h"
#include "stage2-slot1-kernel1-bin.h"
#include "stage2-slot1-kernel2-bin.h"
#include "stage2-slot2-kernel1-bin.h"
#include "stage2-slot2-kernel2-bin.h"
#include "stage2/common/util/encoder.hh"

#include <iostream>

using namespace Mips::Encoder;

constexpr uint32_t loaderAddrSlot1 = 0xbe48;
constexpr uint32_t loaderAddrSlot2 = 0xbec8;
constexpr size_t frameSize = 0x80;

enum class ExploitType {
    Standard,    // One or more directory entries, which change an instruction. The payload loads and runs stage2.
    MemcardISR,  // Hooks an instruction of the memcard ISR, while it is being read (BIOS 1.1). The payload must be
                 // stored in the last directory entry (with the correct checksum), and in the first broken sector (with
                 // a bad checksum). The exploit must complete the current read command before loading stage 2.
    ICacheFlush,  // The exploit is sensible to icache (BIOS 2.0). To force the icache to be flushed, the reading of the
                  // directory of the first memory card must be complete, and another memory card must be present in
                  // slot 2. As a result, the payload must be duplicated in every broken sector entry, with a correct
                  // checksum. It will trigger when the second memory card starts being read.
};

struct ExploitSettings {
    uint32_t stackBase;
    uint32_t addressToModify;
    uint32_t originalValue;
    uint32_t newValue;
    bool inIsr = false;
    uint32_t loopsPerIncrement = 2;
    uint32_t memCardDirEntryIndex = 0;
    ExploitType type = ExploitType::Standard;
    int16_t isrBeqDistance = 0;
    uint32_t kernelVersion; // 1 or 2; version 1 is used in BIOSes up to 2.0 included.

    constexpr static uint32_t maxFileSizeToUse = 0x7fffc000;

    uint16_t index() const noexcept { return static_cast<uint16_t>((addressToModify - stackBase) / 4); }

    uint32_t difference() const noexcept { return newValue - originalValue; }

    uint32_t maxIncrementPerSlot() const noexcept { return 0x7fffffff / (0x2000 * loopsPerIncrement); }

    int nbSlotsNeeded() const noexcept {
        // Each slot allows to increment at most by 0x1ffff.
        const uint32_t diff = difference();
        int result = (diff / maxIncrementPerSlot());
        result += (diff % maxIncrementPerSlot()) == 0 ? 0 : 1;
        return result;
    }

    uint32_t fileSizeToUseForLastSlot() const noexcept {
        return (difference() % maxIncrementPerSlot()) * (0x2000 * loopsPerIncrement);
    }

    bool valid() const noexcept {
        // Index must be <= 0xfffe.
        if ((addressToModify - stackBase) / 4 > 0xfffe) {
            return false;
        }
        // 15 slots are usable to increment the value.
        // However, we count slots usable starting for the mem card dir entry index.
        // This could be improved by using all the slots even if memCardDirEntryIndex is not 0.
        else if (memCardDirEntryIndex + nbSlotsNeeded() > 15) {
            return false;
        } else {
            return true;
        }
    }
};

// A BIOS is identifiend by its version number, date, region ('A' for US/NTSC, 'E' for Europe/PAL, 'I' for
// Japan/NTSC-J), CRC32
// For example, version 4.1 (1997-12-16), PAL (Europe), 0x318178bf. This is used as a key with the
// format: 41, 19971216, 'E', 0x318178bf.
typedef std::tuple<uint32_t, uint32_t, char, uint32_t> BIOSKey;

static std::map<BIOSKey, ExploitSettings> slot1ExploitSettings{
    // Exploit settings in order:
    // base of stack array, address to modify, original value, new value,
    // calls payload from ISR, number of loops per increment, index of directory entry, exploit type.
    {{10, 19940922, 'I', 0x3b601fc8}, {0x801ffcb0, 0x80204f04, 0x0c0006c1, 0x0c002f92, true, 3}},

    {{11, 19950122, 'I', 0x3539def6},
     {0x801ffcc0, 0x80204d6c, 0x10000004, 0x10001c36, true, 3, 0, ExploitType::MemcardISR, -0x70cc}},

    {{20, 19950507, 'A', 0x55847d8c},
     {0x801ffcc0, 0x80206a38, 0x24090025, 0x24091cd0, false, 3, 0}},
    {{20, 19950510, 'E', 0x9bb87c4b},
     {0x801ffcb8, 0x80206aa8, 0x24090017, 0x24091cd0, false, 3, 0}},

    {{21, 19950717, 'A', 0xaff00f2f}, {0x801ffcc8, 0x80204de8, 0x1000003d, 0x10001c17, true}},
    {{21, 19950717, 'E', 0x86c30531}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 2, 8}},
    {{21, 19950717, 'I', 0xbc190209}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002f92, true}},

    {{22, 19951204, 'A', 0x37157331}, {0x801ffcc8, 0x80204de8, 0x1000003d, 0x10001c17, true}},
    {{22, 19951204, 'E', 0x1e26792f}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 2, 8}},
    {{22, 19951204, 'I', 0x24fc7e17}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002f92, true}},

    {{30, 19960909, 'I', 0xff3eeb8c}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002f92, true}},
    {{30, 19961118, 'A', 0x8d8cb7e4}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},
    {{30, 19970106, 'E', 0xd786f0b9}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true}},

    {{40, 19970818, 'I', 0xec541cd0}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},

    {{41, 19971114, 'A', 0xb7c43dad}, {0x801ffcd0, 0x80204f74, 0x0c0006d1, 0x0c002f92, true, 3}},
    // BIOS 4.1-19971216 A and E are exactly the same, except 1 character
    {{41, 19971216, 'A', 0x502224b6}, {0x801ffcd0, 0x80204f74, 0x0c0006d1, 0x0c002f92, true, 3}},
    {{41, 19971216, 'E', 0x318178bf}, {0x801ffcd0, 0x80204f74, 0x0c0006d1, 0x0c002f92, true, 3}},

    {{43, 20000311, 'I', 0xf2af798b}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},

    // BIOS 4.4 A and E are exactly the same, except 1 character
    {{44, 20000324, 'A', 0x6a0e22a0}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},
    {{44, 20000324, 'E', 0x0bad7ea9}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},

    // BIOS 4.5 A and E are exactly the same, except 1 character
    {{45, 20000525, 'A', 0x171bdcec}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},
    {{45, 20000525, 'E', 0x76b880e5}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002f92, true, 3}},
};

static std::map<BIOSKey, ExploitSettings> slot2ExploitSettings{
    {{10, 19940922, 'I', 0x3b601fc8},
     {0x801ffcb0, 0x80204d80, 0x10400007, 0x10421c51, true, 4, 0, ExploitType::MemcardISR, -0x712c}},

    {{11, 19950122, 'I', 0x3539def6},
     {0x801ffcc0, 0x80204d6c, 0x10000004, 0x10001c56, true, 3, 0, ExploitType::MemcardISR, -0x712c}},

    {{20, 19950507, 'A', 0x55847d8c}, {0x801ffcc0, 0x80204e94, 0x0c000501, 0x0c002fb2, true, 2, 5}},
    {{20, 19950510, 'E', 0x9bb87c4b}, {0x801ffcb8, 0x80204f04, 0x0c0006c1, 0x0c002fb2, true, 3, 0}},

    {{21, 19950717, 'A', 0xaff00f2f}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002fb2, true}},
    {{21, 19950717, 'E', 0x86c30531}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002fb2, true}},
    {{21, 19950717, 'I', 0xbc190209},
     {0x801ffcc8, 0x80204ddc, 0x10000004, 0x10001c3a, true, 2, 13, ExploitType::MemcardISR, -0x70dc}},

    {{22, 19951204, 'A', 0x37157331}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002fb2, true}},
    {{22, 19951204, 'E', 0x1e26792f}, {0x801ffcc0, 0x80204f64, 0x0c001acc, 0x0c002fb2, true}},
    {{22, 19951204, 'I', 0x24fc7e17},
     {0x801ffcc8, 0x80204ddc, 0x10000004, 0x10001c3a, true, 2, 13, ExploitType::MemcardISR, -0x70dc}},

    {{30, 19960909, 'I', 0xff3eeb8c},
     {0x801ffcc8, 0x80204ddc, 0x10000004, 0x10001c3a, true, 2, 13, ExploitType::MemcardISR, -0x70dc}},
    {{30, 19961118, 'A', 0x8d8cb7e4}, {0x801ffcc8, 0x80204f64, 0x0c001acc, 0x0c002fb2, true}},
    {{30, 19970106, 'E', 0xd786f0b9}, {0x801ffcc0, 0x80204f04, 0x0c000511, 0x0c002fb2, true, 2, 3}},

    {{40, 19970818, 'I', 0xec541cd0}, {0x801ffcc8, 0x80204f04, 0x0c000511, 0x0c002fb2, true}},

    {{41, 19971114, 'A', 0xb7c43dad}, {0x801ffcd0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},
    // BIOS 4.1-19971216 A and E are exactly the same, except 1 character
    {{41, 19971216, 'A', 0x502224b6}, {0x801ffcd0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},
    {{41, 19971216, 'E', 0x318178bf}, {0x801ffcd0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},

    {{43, 20000311, 'I', 0xf2af798b}, {0x801ffcc0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},

    // BIOS 4.4 A and E are exactly the same, except 1 character
    {{44, 20000324, 'A', 0x6a0e22a0}, {0x801ffcc0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},
    {{44, 20000324, 'E', 0x0bad7ea9}, {0x801ffcc0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},

    // BIOS 4.5 A and E are exactly the same, except 1 character
    {{45, 20000525, 'A', 0x171bdcec}, {0x801ffcc0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},
    {{45, 20000525, 'E', 0x76b880e5}, {0x801ffcc0, 0x80204f74, 0x0c0006d1, 0x0c002fb2, true}},
};

// Maps model version (e.g. 9002) to its BIOS version.
static std::unordered_map<uint32_t, BIOSKey> modelToBios{
    {1000, {10, 19940922, 'I', 0x3b601fc8}},
    // 1001 and 1002 can be either 2.0, 2.1 or 2.2
    {3000, {11, 19950122, 'I', 0x3539def6}},
    {3500, {21, 19950717, 'I', 0xbc190209}},
    {5000, {22, 19951204, 'I', 0x24fc7e17}},
    {5001, {30, 19961118, 'A', 0x8d8cb7e4}},
    {5003, {22, 19951204, 'A', 0x37157331}},
    {5500, {30, 19960909, 'I', 0xff3eeb8c}},
    {5501, {30, 19961118, 'A', 0x8d8cb7e4}},
    {5502, {30, 19970106, 'E', 0xd786f0b9}},
    {5503, {30, 19961118, 'A', 0x8d8cb7e4}},
    {5552, {30, 19970106, 'E', 0xd786f0b9}},
    {7000, {40, 19970818, 'I', 0xec541cd0}},
    // 70000 means 7000W
    {70000, {41, 19971114, 'A', 0xb7c43dad}},
    {7001, {41, 19971216, 'A', 0x502224b6}},
    {7002, {41, 19971216, 'E', 0x318178bf}},
    {7003, {30, 19961118, 'A', 0x8d8cb7e4}},
    {7500, {40, 19970818, 'I', 0xec541cd0}},
    {7501, {41, 19971216, 'A', 0x502224b6}},
    {7502, {41, 19971216, 'E', 0x318178bf}},
    {7503, {41, 19971216, 'A', 0x502224b6}},
    {9000, {40, 19970818, 'I', 0xec541cd0}},
    {9001, {41, 19971216, 'A', 0x502224b6}},
    {9002, {41, 19971216, 'E', 0x318178bf}},
    {9003, {41, 19971216, 'A', 0x502224b6}},
    {9903, {41, 19971216, 'A', 0x502224b6}},
    {100, {43, 20000311, 'I', 0xf2af798b}},
    // 101 and 102 can be either 4.4 or 4.5
    {103, {45, 20000525, 'A', 0x171bdcec}},
};

// Contains all the settings needed to create a memory card image.
struct ImageSettings {
    ExploitSettings exploitSettings;
    std::string inputFileName;
    std::string outputFileName;
    bool fastload;
    uint32_t stage3_tload;
    bool returnToShell;
    bool noInterrupts;
    bool restoreOriginalValue;
    bool noGP;
    bool backwards;
    bool useDeletedEntries;
    bool slot2;
};

static void banner() {
    printf("FreePSXBoot memory card builder\n");
    printf("Exploit created by brad-lin\n");
    printf("Builder created by Nicolas \"Pixel\" Noble\n");
    printf("https://github.com/brad-lin/FreePSXBoot\n\n");
}

static void usage() {
    printf(
        "This program can build a memory card using known exploit values for supported model versions,\n"
        "or build an experimental memory card which gives full control of the value modified by the exploit.\n"
        "If you just want to run something on a PlayStation with a supported model or BIOS version,\n"
        "use -model or -bios with the corresponding value (e.g. -model 9002 or -bios 4.4).\n"
        "If you know what you are doing and want to experiment, use the advanced options.\n");
    printf(
        "Usage:\n"
        "\tTo run a PS-EXE payload (standard usage):\n"
        "\t\tbuilder -model 9002 -in payload.exe -out card.mcd\n"
        "\tTo run a raw payload (advanced usage):\n"
        "\t\tbuilder -model 9002 -in payload.bin -out card.mcd -tload 0x801b0000\n"
        "\tTo experiment (expert usage; these example values work on most 7000+ model versions):\n"
        "\t\tbuilder -base 0x801ffcd0 -vector 0x802009b4 -old 0x4d3c -new 0xbe48 -in payload.bin -out card.mcd -tload "
        "0x801b0000\n");
    printf("\n");
    printf(
        "-model    the model version, as 3 or 4 digits (e.g. 9002). If you use this option, don't use base, vector, "
        "old, and new.\n");
    printf(
        "-bios     the BIOS version, as X.Y (e.g. 4.4), or X.Y-YYYYMMDD or X.Y-YYYYMMDD-R (with R being A, E, or I) if "
        "disambiguation is needed. If you use this option, don't use base, vector, old, and new.\n");
    printf(
        "-slot     the slot where the memory card will be inserted (1 or 2). If slot is 1, the memory card must be "
        "removed after the exploit is triggered. If slot is 2, the exploit disables the memory card in slot 2, and it "
        "can be left there.\n");
    printf("-base     the base address of the stack array being exploited from buInit\n");
    printf(
        "-vector   the address of the value we want to modify. Use 0x802 as a prefix, e.g. 0x802009b4 to modify value "
        "at address 0x09b4.\n");
    printf("-old      the original value to modify\n");
    printf("-new      the new value we want to set\n");
    printf("-in       the payload file; if it is a ps-exe, tload is optional\n");
    printf("-out      the output filename to create\n");
    printf("-tload    if 'in' is a binary payload, use this address to load and jump to\n");
    printf("-return   make a payload that allows returning to the shell\n");
    printf("-norestore do not restore the value overwritten by the exploit\n");
    printf("-noint    disable interrupts during payload\n");
    printf("-nogp     disable setting $gp during payloads\n");
    printf("-bw       load binary backwards (start with last frame; saves 1 payload instruction)\n");
    printf("-deleted  use deleted fake entries\n");
    printf("-fastload use fastload method\n");
    printf("-all      generate images for all supported BIOS versions\n");
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

static void putU32(uint8_t* ptr, uint32_t d) {
    *ptr++ = d & 0xff;
    d >>= 8;
    *ptr++ = d & 0xff;
    d >>= 8;
    *ptr++ = d & 0xff;
    d >>= 8;
    *ptr++ = d & 0xff;
}

static void putU32Vector(uint8_t* ptr, const std::vector<uint32_t>& data) {
    for (uint32_t word : data) {
        putU32(ptr, word);
        ptr += sizeof(uint32_t);
    }
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

static std::string biosVersionToString(uint32_t version) {
    return std::to_string(version / 10) + "." + std::to_string(version % 10);
}

static std::string toHexString(uint32_t number) {
    char result[9];
    snprintf(result, sizeof(result), "%08x", number);
    return std::string(result);
}

static void createImage(ImageSettings settings, const uint8_t* stage2, uint32_t stage2_size) {
    const ExploitSettings& exploitSettings = settings.exploitSettings;
    const bool returnToShell = settings.returnToShell;
    if (returnToShell && exploitSettings.inIsr) {
        throw std::runtime_error("Payload wants to return to shell, but exploit is in ISR");
    }
    static std::array<uint8_t, frameSize * 1024> out;
    memset(out.data(), 0, out.size());
    out[0x0000] = 'M';
    out[0x0001] = 'C';
    out[0x007f] = 0x0e;

    // Write directory entries (as many as needed)
    const int startIndex = exploitSettings.memCardDirEntryIndex;
    for (int i = startIndex; i < startIndex + exploitSettings.nbSlotsNeeded(); i++) {
        const int offset = (i + 1) * frameSize;
        const bool last = i == startIndex + exploitSettings.nbSlotsNeeded() - 1;
        uint32_t fileSize = last ? exploitSettings.fileSizeToUseForLastSlot() : ExploitSettings::maxFileSizeToUse;
        // Note: exploit will fail if the value at the fake directory entry is 0xAX or 0x5X, depending of this value.
        // If it fails, change to 0xA1.
        out[offset] = settings.useDeletedEntries ? 0xa1 : 0x51;

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
        for (unsigned i = 0; i < frameSize; i++) {
            crc ^= out[offset + i];
        }
        out[offset + 0x7f] = crc;
    }

    FILE* inFile = fopen(settings.inputFileName.c_str(), "rb");
    if (!inFile) {
        throw std::runtime_error("Failed to open input file " + settings.inputFileName);
    }

    fseek(inFile, 0, SEEK_END);
    std::vector<uint8_t> in;
    in.resize(ftell(inFile));
    fseek(inFile, 0, SEEK_SET);
    auto n = fread(in.data(), in.size(), 1, inFile);
    if (n != 1) {
        throw std::runtime_error("Failed to read input file " + settings.inputFileName);
    }
    fclose(inFile);

    uint32_t stage3_tsize = static_cast<uint32_t>(in.size());
    uint32_t stage3_gp = 0;
    uint32_t stage3_pc = settings.stage3_tload;
    uint32_t stage3_sp = 0;

    uint8_t* payloadPtr = in.data();

    if (memcmp(in.data(), "PS-X EXE", 8) == 0) {
        stage3_pc = getU32(in.data() + 0x10);
        stage3_gp = getU32(in.data() + 0x14);
        settings.stage3_tload = getU32(in.data() + 0x18);
        stage3_tsize = getU32(in.data() + 0x1c);
        stage3_sp = getU32(in.data() + 0x30);
        stage3_sp += getU32(in.data() + 0x34);
        payloadPtr = in.data() + 2048;
    } else {
        if (settings.stage3_tload == 0) {
            throw std::runtime_error("Raw binary and no tload argument.");
        }
    }

    uint32_t stage2_tload = settings.stage3_tload;
    uint32_t stage2_pc = stage3_pc;
    uint32_t stage2_tsize = stage3_tsize;

    if (settings.fastload) {
        stage2_tload = 0x8000d000;
        stage2_pc = 0x8000d000;
        stage2_tsize = stage2_size;

        uint32_t stage2_end = stage2_tload + stage2_tsize;
        uint32_t stage3_end = settings.stage3_tload + stage3_tsize;

        if (((settings.stage3_tload <= stage2_end) && (stage2_end <= stage3_end)) ||
            ((settings.stage3_tload <= stage2_tload) && (stage2_tload <= stage3_end))) {
            std::ostringstream error;
            error << "Stage2 and stage3 payloads are intersecting." << std::endl;
            error << "Stage2: 0x" << std::hex << stage2_tload << " - 0x" << stage2_end << std::endl;
            error << "Stage3: 0x" << std::hex << settings.stage3_tload << " - 0x" << stage3_end << std::endl;
            throw std::runtime_error(error.str());
        }
    }

    constexpr unsigned firstUsableFrame = 64;
    // First stage 2 frame is a dummy frame.
    constexpr unsigned firstRealFrame = firstUsableFrame + 1;
    const bool backwards = settings.backwards;

    if (stage3_sp == 0) stage3_sp = 0x801ffff0;
    unsigned stage2_frames = (stage2_tsize + 127) >> 7;
    unsigned stage3_frames = (stage3_tsize + 127) >> 7;
    constexpr unsigned availableFrames = ((frameSize * 1024) >> 7) - firstRealFrame;
    if (settings.fastload) {
        if ((stage2_frames + stage3_frames + 1) > availableFrames) {
            throw std::runtime_error(
                "Payload binary too big. Maximum = " + std::to_string(availableFrames * frameSize) + " bytes");
        }
        memcpy(out.data() + firstRealFrame * frameSize, stage2, stage2_tsize);
        uint8_t* configPtr = out.data() + (firstRealFrame + stage2_frames) * frameSize;
        putU32(configPtr, stage3_pc);
        configPtr += 4;
        putU32(configPtr, stage3_gp);
        configPtr += 4;
        putU32(configPtr, stage3_sp);
        configPtr += 4;
        putU32(configPtr, settings.stage3_tload);
        configPtr += 4;
        putU32(configPtr, stage3_frames);
        configPtr += 4;
        memcpy(configPtr, "FREEPSXBOOT FASTLOAD PAYLOAD", 28);
        memcpy(out.data() + (firstRealFrame + stage2_frames + 1) * frameSize, payloadPtr, stage3_tsize);
    } else {
        if (stage2_frames > availableFrames) {
            throw std::runtime_error(
                "Payload binary too big. Maximum = " + std::to_string(availableFrames * frameSize) + " bytes");
        }
        memcpy(out.data() + firstRealFrame * frameSize, payloadPtr, stage2_tsize);
    }

    std::vector<uint32_t> disableInterrupts = {
        // disabling all interrupts except for memory card
        addiu(Reg::V0, Reg::R0, 128),
        lui(Reg::V1, 0x1f80),
        sw(Reg::V0, 0x1074, Reg::V1),
    };

    std::vector<uint32_t> saveRegisters = {
        addiu(Reg::S8, Reg::RA, 0),
    };

    std::vector<uint32_t> restoreValue = {
        // restoring old value
        lui(Reg::V0, getHI(exploitSettings.originalValue)),
        addiu(Reg::V0, Reg::V0, getLO(exploitSettings.originalValue)),
        lui(Reg::V1, getHI(exploitSettings.addressToModify)),
        sw(Reg::V0, getLO(exploitSettings.addressToModify), Reg::V1),
        // flush cache
        jal(0xa0),
        addiu(Reg::T1, Reg::R0, 0x44),
    };

    unsigned lastLoadAddress = stage2_tload + frameSize * (stage2_frames - 1);
    std::vector<uint32_t> loadBinary;
    const uint32_t mcSlot = settings.slot2 ? 1 : 0;
    const uint32_t mcDeviceId = mcSlot * 0x10;
    if (backwards) {
        loadBinary = {
            // S0 = current frame
            addiu(Reg::GP, Reg::R0, stage2_frames),
            // K1 = destination address of current frame
            lui(Reg::K1, getHI(lastLoadAddress)),
            addiu(Reg::K1, Reg::K1, getLO(lastLoadAddress)),
            // read_loop:
            // decrement frame counter
            addi(Reg::GP, Reg::GP, -1),
            // A0 = deviceID (0 or 0x10)
            addiu(Reg::A0, Reg::R0, mcDeviceId),
            // A1 = frame counter + firstRealFrame
            addiu(Reg::A1, Reg::GP, firstRealFrame),
            // A2 = current destination address pointer
            addiu(Reg::A2, Reg::K1, 0),
            // mcReadSector(0 or 0x10, frame, dest)
            jal(0xb0),
            addiu(Reg::T1, Reg::R0, 0x4f),
            // mcWaitForStatus(0 or 1);
            addiu(Reg::A0, Reg::R0, mcSlot),
            jal(0xb0),
            addiu(Reg::T1, Reg::R0, 0x5d),
            // if frame counter is not zero, go back 40 bytes (aka read_loop)
            bne(Reg::GP, Reg::R0, -40),
            // decrement destination pointer by sizeof(frame) in the delay slot
            addi(Reg::K1, Reg::K1, -128),
        };
    } else {
        loadBinary = {
            // GP = first frame
            addiu(Reg::GP, Reg::R0, firstUsableFrame),
            // K1 = destination address of first frame. Take into account the dummy frame.
            lui(Reg::K1, getHI(stage2_tload - frameSize)),
            addiu(Reg::K1, Reg::K1, getLO(stage2_tload - frameSize)),
            // read_loop:
            // A0 = deviceID (0 or 0x10)
            addiu(Reg::A0, Reg::R0, mcDeviceId),
            // A1 = frame counter
            addiu(Reg::A1, Reg::GP, 0),
            // increment frame counter
            addi(Reg::GP, Reg::GP, 1),
            // A2 = current destination address pointer
            addiu(Reg::A2, Reg::K1, 0),
            // mcReadSector(0 or 0x10, frame, dest)
            jal(0xb0),
            addiu(Reg::T1, Reg::R0, 0x4f),
            // mcWaitForStatus(0 or 1);
            addiu(Reg::A0, Reg::R0, mcSlot),
            jal(0xb0),
            addiu(Reg::T1, Reg::R0, 0x5d),
            // if frame counter is not last frame, go back 44 bytes (aka read_loop)
            addiu(Reg::AT, Reg::R0, firstRealFrame + stage2_frames),
            bne(Reg::GP, Reg::AT, -44),
            // increment destination pointer by sizeof(frame) in the delay slot
            addiu(Reg::K1, Reg::K1, 128),
        };
    }

    std::vector<uint32_t> setGP = {
        lui(Reg::GP, getHI(stage3_gp)),
        addiu(Reg::GP, Reg::GP, getLO(stage3_gp)),
    };

    std::vector<uint32_t> bootstrapReturn = {
        // flush cache
        jal(0xa0),
        addiu(Reg::T1, Reg::R0, 0x44),

        lui(Reg::V0, getHI(stage2_pc)),
        addiu(Reg::V0, Reg::V0, getLO(stage2_pc)),
        jr(Reg::V0),
        addiu(Reg::RA, Reg::S8, 0),
    };

    std::vector<uint32_t> bootstrapNoReturn = {
        // bootstrap
        lui(Reg::RA, getHI(stage2_pc)),
        addiu(Reg::RA, Reg::RA, getLO(stage2_pc)),
        lui(Reg::SP, getHI(stage3_sp)),
        addiu(Reg::SP, Reg::SP, getLO(stage3_sp)),

        // flush cache
        j(0xa0),
        addiu(Reg::T1, Reg::R0, 0x44),
    };

    std::vector<uint32_t> payload;
    auto append = [&payload](std::vector<uint32_t>& block) {
        std::copy(begin(block), end(block), std::back_inserter(payload));
    };

    if (settings.exploitSettings.type == ExploitType::MemcardISR) {
        // Workaround for BIOS 1.1: return to ISR unless read is complete (v0 != 0)
        payload.push_back(beq(Reg::V0, Reg::R0, settings.exploitSettings.isrBeqDistance));
        payload.push_back(nop());
    }

    if (settings.noInterrupts) append(disableInterrupts);
    if (returnToShell) append(saveRegisters);
    // If the payload can hold the code to flash the screen, it will be placed here
    const std::size_t flashScreenIndex = payload.size();
    if (settings.restoreOriginalValue) append(restoreValue);
    if (settings.exploitSettings.inIsr) payload.push_back(rfe());
    append(loadBinary);
    // as GP is used for relative accessing, unless user-overriden, will be set only if non-zero (aka is used at all)
    const bool doSetGP = stage3_gp != 0 && !settings.noGP;
    if (doSetGP) append(setGP);

    if (returnToShell) {
        append(bootstrapReturn);
    } else {
        append(bootstrapNoReturn);
    }
    if (payload.size() > 32) {
        throw std::runtime_error("Payload is too big, using " + std::to_string(payload.size()) +
                                 " instructions out of 32.");
    }

    // Add screen flashing, if remaining size allows, and fastload not enabled
    const uint32_t loaderAddr = settings.slot2 ? loaderAddrSlot2 : loaderAddrSlot1;
    std::vector<uint32_t> flashScreen = {
        // GP is always 0xa0010ff0. Use this to save an instruction.
        addiu(Reg::A0, Reg::GP, static_cast<uint16_t>((loaderAddr + 16 + payload.size() * 4)) - 0x10ff0),
        addiu(Reg::A1, Reg::R0, 3),
        jal(0xa0),
        addiu(Reg::T1, Reg::R0, 0x4a),
    };

    std::vector<uint32_t> gpuWords = {
        0x0200a5ff,  // paint orange
        0,           // x, y
        0x01ff03ff,  // width, height
    };

    if (!settings.fastload && (payload.size() + flashScreen.size() + gpuWords.size() <= 32)) {
        printf("Adding screen flashing in initial loader\n");
        payload.insert(payload.begin() + flashScreenIndex, flashScreen.begin(), flashScreen.end());
        payload.insert(payload.end(), gpuWords.begin(), gpuWords.end());
    } else if (!settings.fastload) {
        printf("Not enough space to add screen flashing in initial loader\n");
    }

    // Compute payload checksum
    uint8_t crc = 0;
    int crcIndex = 0;
    size_t payloadSize = payload.size() * sizeof(decltype(payload)::value_type);
    const uint8_t* payloadBytes = reinterpret_cast<const uint8_t*>(payload.data());
    while (crcIndex < payloadSize && crcIndex < 127) {
        crc ^= payloadBytes[crcIndex];
        crcIndex++;
    }
    // If exploit type is not standard, the checksum must be controllable
    if (payloadSize == frameSize && exploitSettings.type != ExploitType::Standard) {
        throw std::runtime_error("Payload is 128 bytes but its checksum must be controllable.");
    }
    // If payload has 128 bytes, check that the checksum is bad
    if (payloadSize == frameSize && crc == payloadBytes[127]) {
        throw std::runtime_error("Payload is 128 bytes and its checksum is not bad.");
    }

    constexpr std::size_t payloadOffset = frameSize * 16;
    if (exploitSettings.type == ExploitType::MemcardISR) {
        // Store the payload with correct checksum in the last directory entry (frame 15)
        putU32Vector(&out[payloadOffset - frameSize], payload);
        out[0x7ff] = crc;
        // Also store the payload with bad checksum in the first broken sector entry (frame 16)
        putU32Vector(&out[payloadOffset], payload);
        out[0x87f] = ~crc;
    } else if (exploitSettings.type == ExploitType::ICacheFlush) {
        // Store the payload with correct checksum in all broken sector entries (frames 16 to 35 included)
        for (int i = 16; i < 36; i++) {
            putU32Vector(&out[i * frameSize], payload);
            out[i * frameSize + 0x7f] = crc;
        }
    } else {
        // Store the payload with bad checksum in the first broken sector entry (frame 16)
        putU32Vector(&out[payloadOffset], payload);
        out[0x87f] = ~crc;
    }

    // If total size is less than 64 kB, duplicate the content to the next 64 kB.
    // This cheap fix makes it possible to use fake memory cards with a 512 kbits (64 kB) chip.
    if (payloadOffset + payload.size() <= out.size() / 2) {
        memcpy(out.data() + out.size() / 2, out.data(), out.size() / 2);
    } else {
        printf("Note: payload size is more than half the card size.\n");
    }

    FILE* outFile = fopen(settings.outputFileName.c_str(), "wb");
    if (!outFile) {
        throw std::runtime_error("Failed to open output file " + settings.outputFileName);
    }

    fwrite(out.data(), out.size(), 1, outFile);
    fclose(outFile);
}

int main(int argc, char** argv) {
    banner();
    const flags::args args(argc, argv);

    auto modelVersionStr = args.get<std::string>("model");
    auto biosVersionStr = args.get<std::string>("bios");
    auto slot2Exploit = args.get<int>("slot", 1) == 2;
    auto baseStr = args.get<std::string>("base");
    auto vectorStr = args.get<std::string>("vector");
    auto oldAddrStr = args.get<std::string>("old");
    auto newAddrStr = args.get<std::string>("new");
    auto inName = args.get<std::string>("in");
    auto outName = args.get<std::string>("out");
    auto tloadStr = args.get<std::string>("tload");

    const bool generateAll = args.get<bool>("all", false);
    const bool modelOrBiosPresent = modelVersionStr.has_value() || biosVersionStr.has_value();
    const bool anyExpertArgPresent =
        baseStr.has_value() || vectorStr.has_value() || oldAddrStr.has_value() || newAddrStr.has_value();
    const bool allExpertArgsPresent =
        baseStr.has_value() && vectorStr.has_value() && oldAddrStr.has_value() && newAddrStr.has_value();
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
    // Check that model version xor expert args are used
    if ((generateAll || modelOrBiosPresent) && anyExpertArgPresent) {
        usage();
        return -1;
    }

    uint32_t modelVersion = 0;
    uint32_t biosVersion = 0;
    uint32_t biosDate = 0;
    char biosRegion = 0;
    uint32_t base;
    uint32_t vector;
    uint32_t oldAddr;
    uint32_t newAddr;
    uint32_t stage3_tload = 0;

    auto& biosExploitSettings = slot2Exploit ? slot2ExploitSettings : slot1ExploitSettings;

    // Set kernel version according to BIOS version.
    for (auto& it : biosExploitSettings) {
        const BIOSKey& biosKey = it.first;
        it.second.kernelVersion = (std::get<0>(biosKey) <= 20) ? 1 : 2;
    }

    const char* argname = nullptr;
    try {
        if (modelVersionStr.has_value()) {
            argname = "model";
            modelVersion = std::stoul(modelVersionStr.value(), nullptr, 0);
        } else if (biosVersionStr.has_value()) {
            // Accepted BIOS version formats:
            // 2.2
            // 2.2-19951204
            // 2.2-19951204-A
            argname = "bios";
            biosVersion = (biosVersionStr->at(0) - '0') * 10;
            biosVersion += biosVersionStr->at(2) - '0';
            if (biosVersionStr->size() > 4) {
                biosDate = std::stoul(biosVersionStr->substr(4));
            }
            if (biosVersionStr->size() >= 14) {
                biosRegion = biosVersionStr->at(13);
            }
        } else if (!generateAll) {
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
            stage3_tload = std::stoul(tloadStr.value(), nullptr, 0);
        }
    } catch (...) {
        printf("Error parsing argument %s\n", argname);
        return -1;
    }

    struct ExploitSettings exploitSettings;
    if (!generateAll) {
        if (modelVersion != 0) {
            auto it = modelToBios.find(modelVersion);
            if (it == modelToBios.end()) {
                printf("Unsupported model %u. Supported models are:\n", modelVersion);
                for (const auto& it : modelToBios) {
                    printf("%u ", it.first);
                }
                printf("\n");
                return -1;
            }
            exploitSettings = biosExploitSettings.at(it->second);
            printf("Using exploit settings for model %u\n", modelVersion);
        } else if (biosVersion != 0) {
            bool found = false;
            for (const auto& it : biosExploitSettings) {
                auto [version, date, region, crc] = it.first;
                if (biosVersion == version && (biosDate == 0 || biosDate == date) &&
                    (biosRegion == 0 || biosRegion == region)) {
                    exploitSettings = it.second;
                    found = true;
                    printf("Using exploit settings for BIOS %s-%u-%c (CRC32: %08x)\n",
                           biosVersionToString(version).c_str(), date, region, crc);
                    break;
                }
            }
            if (!found) {
                printf("Unsupported BIOS %s. Supported BIOS are:\n", biosVersionToString(biosVersion).c_str());
                for (const auto& it : biosExploitSettings) {
                    auto [version, date, region, crc] = it.first;
                    printf("%s-%u-%c (CRC32: %08x)\n", biosVersionToString(version).c_str(), date, region, crc);
                }
                printf("\n");
                return -1;
            }
        } else {
            exploitSettings.stackBase = base;
            exploitSettings.addressToModify = vector;
            exploitSettings.originalValue = oldAddr;
            exploitSettings.newValue = newAddr;
        }

        if (!exploitSettings.valid()) {
            printf(
                "Exploit settings are not valid; either too many slots are needed, or the address to modify is out of "
                "range.\n");
            return -1;
        }
    }

    ImageSettings imageSettings;
    imageSettings.inputFileName = inName.value();
    imageSettings.outputFileName = outName.value();
    imageSettings.fastload = args.get<bool>("fastload", false);
    imageSettings.stage3_tload = stage3_tload;
    imageSettings.returnToShell = args.get<bool>("return", false);
    imageSettings.noInterrupts = args.get<bool>("noint", false);
    imageSettings.restoreOriginalValue = !args.get<bool>("norestore", false);
    imageSettings.noGP = args.get<bool>("nogp", false);
    imageSettings.backwards = args.get<bool>("bw", false);
    imageSettings.useDeletedEntries = args.get<bool>("deleted", false);
    imageSettings.slot2 = slot2Exploit;

    std::map<std::string, ExploitSettings> exploitSettingsToUse;
    if (generateAll) {
        for (const auto& it : biosExploitSettings) {
            auto [version, date, region, crc] = it.first;
            std::ostringstream biosId;
            biosId << biosVersionToString(version) << '-';
            // Add date as YYYY-MM-DD
            const std::string& dateStr = std::to_string(date);
            biosId << dateStr.substr(0, 4) << '-' << dateStr.substr(4, 2) << '-' << dateStr.substr(6, 2) << "-";
            // Add region and CRC32
            biosId << region << '-' << toHexString(crc);
            exploitSettingsToUse[biosId.str()] = it.second;
        }
    } else {
        exploitSettingsToUse[std::string()] = exploitSettings;
    }

    for (const auto& currentExploitSettings : exploitSettingsToUse) {
        try {
            imageSettings.exploitSettings = currentExploitSettings.second;
            if (generateAll) {
                // Check if $ character is in output file name, and replace it with BIOS version
                auto it = imageSettings.outputFileName.find('$');
                if (it != std::string::npos) {
                    std::string newName = imageSettings.outputFileName.substr(0, it);
                    newName += currentExploitSettings.first;
                    newName += imageSettings.outputFileName.substr(it + 1);
                    imageSettings.outputFileName = std::move(newName);
                } else {
                    // Otherwise, append BIOS version
                    imageSettings.outputFileName += "-" + currentExploitSettings.first;
                }
            }
            // Choose the right stage2 payload
            const uint8_t* stage2;
            uint32_t stage2_size;
            if (imageSettings.slot2) {
                if (imageSettings.exploitSettings.kernelVersion == 1) {
                    stage2 = stage2_slot2_kernel1;
                    stage2_size = sizeof(stage2_slot2_kernel1);
                } else {
                    stage2 = stage2_slot2_kernel2;
                    stage2_size = sizeof(stage2_slot2_kernel2);
                }
            } else {
                if (imageSettings.exploitSettings.kernelVersion == 1) {
                    stage2 = stage2_slot1_kernel1;
                    stage2_size = sizeof(stage2_slot1_kernel1);
                } else {
                    stage2 = stage2_slot1_kernel2;
                    stage2_size = sizeof(stage2_slot1_kernel2);
                }
            }
            createImage(imageSettings, stage2, stage2_size);
            printf("Created %s for slot %d\n", imageSettings.outputFileName.c_str(), slot2Exploit ? 2 : 1);
            imageSettings.outputFileName = outName.value();
        } catch (const std::exception& ex) {
            printf("Failed to create memory card image: %s\n", ex.what());
            return -1;
        }
    }

    return 0;
}
