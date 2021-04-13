#include "common/hardware/gpu.h"
#include "common/hardware/hwregs.h"
#include "common/hardware/irq.h"
#include "common/syscalls/syscalls.h"
#include "common/util/djbHash.h"

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

static void waitVSync() {
    uint32_t imask = IMASK;

    IMASK = imask | IRQ_VBLANK;

    while ((IREG & IRQ_VBLANK) == 0)
        ;
    IREG &= ~IRQ_VBLANK;
    IMASK = imask;
}

static void busyLoop(int delay) {
    for (unsigned i = 0; i < delay; i++) __asm__ __volatile__("");
}

static void waitCardIRQ() {
    while ((IREG & IRQ_CONTROLLER) == 0)
        ;
    IREG &= ~IRQ_CONTROLLER;
}

static uint8_t s_readBytes = 0;

int mcReadStepper(uint32_t frame, uint8_t* buffer, int step) {
    uint8_t b;

    switch (step) {
        case 1:
            SIOS[0].ctrl = 0x1003;
            SIOS[0].fifo = 0x81;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            break;
        case 2:
            SIOS[0].fifo = 'R';
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            break;
        case 3:
        case 4:
        case 7:
        case 8:
        case 9:
        case 10:
            SIOS[0].fifo;
            SIOS[0].fifo = 0;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            break;
        case 5:
            SIOS[0].fifo;
            SIOS[0].fifo = frame >> 8;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            break;
        case 6:
            SIOS[0].fifo = frame;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            break;
        case 11:
            SIOS[0].fifo;
            SIOS[0].fifo = 0;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            s_readBytes = 1;
            break;
        case 12:
            b = SIOS[0].fifo;
            SIOS[0].fifo = 0;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            *buffer = b;
            break;
        case 13:
            SIOS[0].fifo;
            SIOS[0].fifo = 0;
            SIOS[0].ctrl |= 0x0010;
            IREG = ~IRQ_CONTROLLER;
            while ((SIOS[0].stat & 2) == 0)
                ;
            SIOS[0].fifo;
            return 1;
    }
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
    }

    IMASK = imask;
}

static const union Color c_fgIdle = {.r = 156, .g = 220, .b = 218};

// djbHash("FREEPSXBOOT FASTLOAD PAYLOAD", 28) = 0x7d734e54
static const uint32_t c_signatureHash = 0x7d734e54;

void main() {
    waitVSync();
    fill(c_fgIdle);
    waitVSync();
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

    union {
        struct {
            uint32_t pc;
            uint32_t gp;
            uint32_t sp;
            uint8_t* tload;
            uint32_t frames;
            char signature[];
        };
        uint8_t buffer[128];
    } header;

    int gotHeader = 0;
    uint32_t counter = 0;
    for (uint32_t frame = 64; frame < 1024; frame++) {
        if (gotHeader) {
            mcReadFrame(frame, header.tload);
            header.tload += 128;
            if (++counter == header.frames) {
                __asm__ volatile("move $v0, %0\nmove $gp, %1\nmove $sp, %2\njr $v0\n"
                                 : "=r"(header.pc), "=r"(header.gp), "=r"(header.sp));
            }
        } else {
            mcReadFrame(frame, header.buffer);
            if (djbHash(header.signature, 28) == c_signatureHash) gotHeader = 1;
        }
    }

    while (1)
        ;
}
