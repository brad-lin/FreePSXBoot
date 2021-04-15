#include "common/hardware/cop0.h"
#include "common/hardware/gpu.h"
#include "common/hardware/hwregs.h"
#include "common/hardware/irq.h"
#include "common/syscalls/syscalls.h"
#include "common/util/djbhash.h"

#ifdef DEBUG
#define printf ramsyscall_printf
#define hexdump hexdimp_impl
#else
#define printf(...)
#define hexdump(...)
#endif

static void hexdump_impl(const void* data_, unsigned size) {
    const uint8_t* data = (const uint8_t*)data_;
    char ascii[17];
    ascii[16] = 0;
    for (unsigned i = 0; i < size; i++) {
        if (i % 16 == 0) printf("%08x  |", i);
        printf("%02X ", data[i]);
        ascii[i % 16] = data[i] >= ' ' && data[i] <= '~' ? data[i] : '.';
        unsigned j = i + 1;
        if ((j % 8 == 0) || (j == size)) {
            printf(" ");
            if (j % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (j == size) {
                ascii[j % 16] = 0;
                if (j % 16 <= 8) printf(" ");
                for (j %= 16; j < 16; j++) printf("   ");
                printf("|  %s \n", ascii);
            }
        }
    }
}

#define WIDTH 512
#define HEIGHT 240

static void fill(const union Color bg) {
    struct FastFill ff = {
        .c = bg,
        .x = 0,
        .y = 0,
        .w = WIDTH,
        .h = HEIGHT,
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
    uint8_t b = 0;
    uint8_t b2 = 0;

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
        case 9:
            while ((b = SIOS[0].fifo) != 0x5d)
                ;
            break;
        case 3:
        case 7:
        case 10:
            b = SIOS[0].fifo;
            break;
        case 4:
            while ((b = SIOS[0].fifo) != 0x5a)
                ;
            break;
        case 6:
            o = frame;
            break;
        case 8:
            while ((b = SIOS[0].fifo) != 0x5c)
                ;
            break;
        case 11:
            b = SIOS[0].fifo;
            s_readBytes = 1;
            break;
        case 12:
            *buffer = SIOS[0].fifo;
            break;
        case 13:
            b = SIOS[0].fifo;
            busyLoop(20);
            SIOS[0].fifo = 0;
            while ((SIOS[0].stat & 2) == 0)
                ;
            b2 = SIOS[0].fifo;
            printf("Stepper: step %i, b = %02X, b2 = %02X\n", step, b, b2);
            return 1;
    }
    printf("Stepper: step %i, b = %02X, readBytes = %i\n", step, b, s_readBytes);

    busyLoop(20);
    SIOS[0].fifo = o;
    busyLoop(20);
    SIOS[0].ctrl |= 0x0010;
    IREG = ~IRQ_CONTROLLER;
    return 0;
}

static void mcReadFrame(uint32_t frame, uint8_t* buffer) {
    uint32_t imask = IMASK;

    IMASK = imask | IRQ_CONTROLLER;

    int step = 0;
    int readCount = 0;
    while (1) {
        busyLoop(200);
        SIOS[0].ctrl |= 0x0012;
        busyLoop(2000);
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
    }

    SIOS[0].ctrl = 0;

    IMASK = imask;
}

static const union Color c_fgIdle = {.r = 156, .g = 220, .b = 218};
static const union Color c_fgError = {.r = 220, .g = 156, .b = 156};
static const union Color c_fgSuccess = {.r = 152, .g = 224, .b = 155};
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
    register int a0 asm("a0") = 0;
    register int a1 asm("a1") = 0;
    register int a2 asm("a2") = 0xf12ee175;
    register int a3 asm("a3") = 0xec55b007;
    __asm__ volatile("lw $ra, 0(%0)\nlw $gp, 4(%0)\nlw $sp, 8(%0)\nli $t1, 0x44\nj 0xa0\n"
                     :
                     : "r"(header), "r"(a0), "r"(a1), "r"(a2), "r"(a3));
}

__attribute__((noreturn)) void main() {
    // disable all interrupts
    printf("Stage2 starting...\n");
    writeCOP0Status(0);
    IMASK = 0;
    printf("Interrupts disabled, resetting GPU.\n");

    // Resets CPU and display a solid color
    const int isPAL = (*((char*)0xbfc7ff52) == 'E');
    GPU_STATUS = 0x00000000;  // reset GPU
    struct DisplayModeConfig config = {
        .hResolution = HR_320,
        .vResolution = VR_240,
        .videoMode = isPAL ? VM_PAL : VM_NTSC,
        .colorDepth = CD_15BITS,
        .videoInterlace = VI_OFF,
        .hResolutionExtended = HRE_NORMAL,
    };
    setDisplayMode(&config);
    setHorizontalRange(isPAL ? 30 : 0, 0xa00);
    setVerticalRange(isPAL ? 43 : 16, isPAL ? 283 : 255);
    setDisplayArea(0, 0);
    setDrawingArea(0, 0, WIDTH, HEIGHT);
    setDrawingOffset(0, 0);
    enableDisplay();
    fill(c_fgIdle);

    printf("Resetting SIO0\n");

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
        printf("Reading frame %i, got header = %i\n", frame, gotHeader);
        if (gotHeader) {
            mcReadFrame(frame, header.tload);
            header.tload += 128;
            if (--header.frames == 0) jump(&header);
        } else {
            mcReadFrame(frame, header.buffer);
            hexdump(header.buffer, 128);
            // we don't store the actual signature string
            // to save on some rodata space
            if (djbHash(header.signature, SIG_LEN) == c_signatureHash) {
                gotHeader = 1;
                fill(c_fgSuccess);
            }
        }
    }

    printf("Error, aborting.\n");
    fill(c_fgError);
    // this shouldn't happen, but just in case
    while (1)
        ;
}
