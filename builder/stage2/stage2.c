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
        case 5:
            o = frame >> 8;
            // fallthrough
        case 3:
        case 4:
        case 7:
        case 8:
        case 9:
        case 10:
            SIOS[0].fifo;
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
        waitCardIRQ();
        busyLoop(0x20);
    }

    SIOS[0].ctrl = 0;

    IMASK = imask;
}

static const union Color c_fgIdle = {.r = 156, .g = 220, .b = 218};
static const int SIG_LEN = 28;
// djbHash("FREEPSXBOOT FASTLOAD PAYLOAD", SIG_LEN) = 0x7d734e54
static const uint32_t c_signatureHash = 0x7d734e54;

// this union represents the structure of the magic frame
// inserted by the builder between stage2 and stage3
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

__attribute__((noreturn)) void main() {
    // disable all interrupts
    writeCOP0Status(0);
    IMASK = 0;

    // fast change gpu settings and display a solid color
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
    waitGPU();
    GPU_DATA = 1 << 10;
    fill(c_fgIdle);

    // reset the SIO in case the stage1 occurred midway through
    // a transaction with the card
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
    // parse the memory card starting from frame 64
    // to try and locate our magic header, then to
    // load our stage3
    for (uint32_t frame = 64; frame < 1024; frame++) {
        if (gotHeader) {
            mcReadFrame(frame, header.tload);
            header.tload += 128;
            if (--header.frames == 0) jump(&header);
        } else {
            mcReadFrame(frame, header.buffer);
            // we don't store the actual signature string
            // to save on some rodata space
            if (djbHash(header.signature, SIG_LEN) == c_signatureHash) gotHeader = 1;
        }
    }

    // this shouldn't happen, but just in case
    while (1)
        ;
}
