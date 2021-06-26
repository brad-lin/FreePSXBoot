TARGET = stage2
TYPE = bin

SRCS = \
common/crt0/crt0.s \
common/syscalls/printf.s \
stage2.c \
reboothook.s \

LDSCRIPT = stage2.ld

# Pass FPSXB_SLOT2=1 on the command line to build stage2 for slot 2, e.g. make FPSXB_SLOT2=1
ifdef FPSXB_SLOT2
CFLAGS += -DFPSXB_SLOT2
ASFLAGS += -Wa,--defsym,FPSXB_SLOT2=1
endif

include common.mk
