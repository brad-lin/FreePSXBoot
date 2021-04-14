#include "common/hardware/cop0.h"
#include "common/hardware/gpu.h"
#include "common/hardware/hwregs.h"
#include "common/hardware/irq.h"
#include "common/syscalls/syscalls.h"
#include "common/util/djbhash.h"

static void fill(const union Color bg) {
    struct FastFill ff = {
        .c = bg,
        .x = 0,
        .y = 0,
        .w = 1024,
        .h = 512,
    };
    fastFill(&ff);
}

static void busyLoop(int delay) {
    for (; delay >= 0; delay--) __asm__ __volatile__("");
}

static void waitCardIRQ() {
    while ((IREG & IRQ_CONTROLLER) == 0)
        ;
    IREG &= ~IRQ_CONTROLLER;
}

static uint8_t s_readBytes = 0;

int mcReadStepper(uint32_t frame, uint8_t* buffer, int step) {
    uint8_t o = 0;

    switch (step) {
        case 1:
            SIOS[0].ctrl = 0x1003;
            o = 0x81;
            break;
        case 2:
            o = 'R';
            break;
        case 3:
        case 4:
        case 7:
        case 8:
        case 9:
        case 10:
            SIOS[0].fifo;
            break;
        case 5:
            SIOS[0].fifo;
            o = frame >> 8;
            break;
        case 6:
            o = frame;
            break;
        case 11:
            SIOS[0].fifo;
            s_readBytes = 1;
            break;
        case 12:
            *buffer = SIOS[0].fifo;
            break;
        case 13:
            SIOS[0].fifo;
            SIOS[0].fifo = 0;
            while ((SIOS[0].stat & 2) == 0)
                ;
            SIOS[0].fifo;
            return 1;
    }
    SIOS[0].fifo = o;
    SIOS[0].ctrl |= 0x0010;
    IREG = ~IRQ_CONTROLLER;
    return 0;
}

static void mcReadFrame(uint32_t frame, uint8_t* buffer) {
    uint32_t imask = IMASK;

    IMASK = imask | IRQ_CONTROLLER;

    SIOS[0].ctrl |= 0x0012;
    busyLoop(200);

    int step = 0;
    int readCount = 0;
    while (1) {
        waitCardIRQ();
        busyLoop(0x20);
        if (!s_readBytes) {
            int r = mcReadStepper(frame, buffer, ++step);
            if (r) break;
        } else {
            *buffer++ = SIOS[0].fifo;
            SIOS[0].fifo = 0;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            if (++readCount > 0x7e) s_readBytes = 0;
        }
    }

    SIOS[0].ctrl = 0;

    IMASK = imask;
}

static const union Color c_fgIdle = {.r = 156, .g = 220, .b = 218};

// djbHash("FREEPSXBOOT FASTLOAD PAYLOAD", 28) = 0x7d734e54
static const uint32_t c_signatureHash = 0x7d734e54;

union Header {
    struct {
        uint32_t pc;
        uint32_t gp;
        uint32_t sp;
        uint8_t* tload;
        uint32_t frames;
        char signature[];
    };
    uint8_t buffer[128];
};

static inline __attribute__((noreturn)) void jump(union Header* header) {
    __asm__ volatile("lw $ra, 0(%0)\nlw $gp, 4(%0)\nlw $sp, 8(%0)\nli $t1, 0x44\nj 0xa0\n" : : "r"(header));
}

void main() {
    writeCOP0Status(0);
    const int isPAL = (*((char*)0xbfc7ff52) == 'E');
    struct DisplayModeConfig config = {
        .hResolution = HR_320,
        .vResolution = VR_240,
        .videoMode = isPAL ? VM_PAL : VM_NTSC,
        .colorDepth = CD_15BITS,
        .videoInterlace = VI_OFF,
        .hResolutionExtended = HRE_NORMAL,
    };
    setDisplayMode(&config);
    setDisplayArea(0, 0);
    setDrawingArea(0, 0, 320, 240);
    setDrawingOffset(0, 0);
    fill(c_fgIdle);

    SIOS[0].ctrl = 0x40;
    SIOS[0].baudRate = 0x88;
    SIOS[0].mode = 13;
    SIOS[0].ctrl = 0;
    busyLoop(10);
    SIOS[0].ctrl = 2;
    busyLoop(10);
    SIOS[0].ctrl = 0x2002;
    busyLoop(10);
    SIOS[0].ctrl = 0;

    union Header header;

    int gotHeader = 0;
    for (uint32_t frame = 64; frame < 1024; frame++) {
        if (gotHeader) {
            mcReadFrame(frame, header.tload);
            header.tload += 128;
            if (--header.frames == 0) jump(&header);
        } else {
            mcReadFrame(frame, header.buffer);
            if (djbHash(header.signature, 28) == c_signatureHash) gotHeader = 1;
        }
    }

    while (1)
        ;
}
