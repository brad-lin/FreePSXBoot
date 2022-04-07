.section .text

.global reboothook

.set noreorder
.set noat

# This address is safe to use across reboots: the kernel never writes to it.
.equ load_params, 0x8000dff0

# Params:
# $a0 = pointer to struct:
# * address to jump to after reboot
# * value of gp
# * value of sp
reboothook:
	# Save the load params to a safe place
	li $t0, load_params
	lw $t1, 0x0($a0)
	lw $t2, 0x4($a0)
	lw $t3, 0x8($a0)
	sw $t1, 0x0($t0)
	sw $t2, 0x4($t0)
	sw $t3, 0x8($t0)

	# The following code sets a hardware breakpoint on access to 0x80030000.
	# This is the address of the shell. The breakpoint will trigger when the kernel is copying the shell code to RAM.
	# At this point, the kernel is fully loaded and set up, and the breakpoint will trigger.
	# We'll use this to execute the payload instead of the shell.
	# Credits for this method go to Nicolas Noble and sickle.

	# hook mask
	li $t1, 0;

	# BP on Data access

	li $a0, 0x80030000
	mtc0 $a0, $5				# reg5 = BPC/Breakpoint on Access
	nop
	li $t0, 0xFFFFFFFF
	mtc0 $t0, $9				# reg9 = BDAM/Breakpoint on Access Address Mask
	nop

	# BP on Exec

	mtc0 $a0, $3				# reg3 = BPC/Breakpoint on Exec
	nop
	li $t0, 0xFFFFFFFF
	mtc0 $t0, $11 # reg11 = BPCM/Breakpoint on Exec Mask
	nop

	li $t1, 0xEB800000

	mtc0 $t1, $7				# a3 = reg7 = DCIC/Breakpoint Control

	# now copy the hook handler in

	#src
	la $t0, copHook

	#dest
	li $t5, 0x80000040

	lw $t1, 0x0($t0)
	lw $t2, 0x4($t0)
	lw $t3, 0x8($t0)
	lw $t4, 0xC($t0)
	sw $t1, 0x0($t5)
	sw $t2, 0x4($t5)
	sw $t3, 0x8($t5)
	sw $t4, 0xC($t5)

	# Reboot
.ifdef PSX_KERNEL1
	la $t0, 0xbfc00398
.else
	la $t0, 0xbfc003a8
.endif
	jalr $t0
	nop

copHook:
	# Jump to the actual handler code
	la $k0, cophandler
	jr $k0
	nop

cophandler:
	# Reset COP0 special registers (i.e. remove any breakpoint)

	#bp addr
	mtc0 $0, $5

	#bp addr mask
	mtc0 $0, $9

	#bp exec
	mtc0 $0, $3

	#bp exec mask
	mtc0 $0, $11

	# remove any active hooks
	mtc0 $0, $7

	# Only patch memcard callbacks for slot 2 exploit
.ifdef FPSXB_SLOT2
	b apply_patches # Delay slot executes an instruction without consequences here (li $at, 1 at patch_1_start)

	# There are 2 kernel versions, called here KERNEL1 and KERNEL2.
	# KERNEL1 is in BIOSes up to version 2.0 included.
	# KERNEL2 is in BIOSes from version 2.1 up.
	# The code of the patched functions is the same between the 2 kernel versions; only their locations change,
	# as well as the address of interrupt_registers_base.

.ifdef PSX_KERNEL1
	.equ patch_start_addr, 0x4d70
	.equ mem_card_slot_info, 0x7490
	.equ card_io_register_base, 0x7248
	.equ mc_got_error, 0x7510
.else
	.equ patch_start_addr, 0x4de0
	.equ mem_card_slot_info, 0x74a0
	.equ card_io_register_base, 0x7258
	.equ mc_got_error, 0x7520
.endif

	# Some addresses needed by the patch
	.equ patch_2_start_addr, patch_start_addr + 0x10
	.equ patch_3_start_addr, patch_start_addr + 0x140
	.equ patch_4_start_addr, patch_start_addr + 0x14c

	.equ callback_returned_1_offset, -0xc
	.equ callback_returned_minus_1_offset, 0x3c
	.equ callback_returned_0_offset, 0x10
	.equ disable_memcard_interrupt_offset, 0x104
	.equ interrupt_handler_epilogue_offset, 0x50

	# Some games don't like to receive the 0x8000 (bad card) event when checking for memory cards, and hang.
	# To ensure better compatibility, make it look like a timeout instead. To achieve this, we let the ISR callback do its
	# first operation and return 0. A memcard interrupt is now expected. We prevent it from being processed by
	# redirecting the code flow to disable the memcard interrupt, which will eventually result in a timeout.
	# This is exactly what would happen if no memory card was inserted, and should be handled correctly by all games.

	# The memory card interrupt handler is patched in place (which is possible as the original code has a few useless instructions)
	# as follows (addresses below match KERNEL2):

patch_1_start:
	# 0x4de0
	li $at, 1 # use delay slot to prepare callback result check
patch_1_end:

patch_2_start:
	# 0x4df0
1: # Using labels to allow having immediate offsets in branches
	beq $v0, $at, 1b + callback_returned_1_offset # If callback returned 1, follow the usual path
	# 0x4df4
	lw $t1, mem_card_slot_info($zero) # Load word at 0x74a0. It contains 0 if current memcard slot is 1, and 0x2000 if slot is 2.
	# 0x4df8
1:
	bne $v0, $zero, 1b + callback_returned_minus_1_offset # If callback returned a non zero value, follow the usual path
	# 0x4dfc
	lw $v1, card_io_register_base($zero) # Load in $v1 the address of the card I/O base register
	# 0x4e00
1:
	beq $t1, $zero, 1b + callback_returned_0_offset # If current slot is slot 1, follow the usual path
	# 0x4e04
	nop
	# 0x4e08
1:
	b 1b + disable_memcard_interrupt_offset # Else (slot is 2), jump in the middle of the function, where memcard interrupts are disabled
	# 0x4e0c
	sh $zero, 0xa($v1) # Disable card transfer
patch_2_end:

patch_3_start:
	# 0x4f20
	lw $t5, mc_got_error($zero) # This saves an instruction  at 0x4f2c, allowing to replace it with the jump below
patch_3_end:

patch_4_start:
	# 0x4f2c
1:
	bne $t1, $zero, 1b + interrupt_handler_epilogue_offset # If slot is 2, exit from the function, now that interrupts are disabled
patch_4_end:

copy_patch:
	lw $t3, 0($t1)
	addiu $t1, $t1, 4
	sw $t3, 0($t0)
	bne $t1, $t2, copy_patch
	addiu $t0, $t0, 4
	jr $ra

apply_patches:
	# Patch 1
	la $t0, patch_start_addr
	la $t1, patch_1_start
	jal copy_patch
	addiu $t2, $t1, patch_1_end - patch_1_start

	# Patch 2
	la $t1, patch_2_start
	la $t0, patch_2_start_addr
	jal copy_patch
	addiu $t2, $t1, patch_2_end - patch_2_start

	# Patch 3
	la $t1, patch_3_start
	la $t0, patch_3_start_addr
	jal copy_patch
	addiu $t2, $t1, patch_3_end - patch_3_start

	# Patch 4
	la $t1, patch_4_start
	la $t0, patch_4_start_addr
	jal copy_patch
	addiu $t2, $t1, patch_4_end - patch_4_start

.endif

	# Let UNIROM know where it was loaded from
	la $t0, 0xbe48
	li $t1, 0xf12ee175
	sw $t1, 0($t0)
	# Let it know from which slot it was loaded
.ifdef FPSXB_SLOT2
	li $t1, 0xec5b0072
.else
	li $t1, 0xec5b0071
.endif
	sw $t1, 4($t0)

	# Load the exe and jump to it
	li $t0, load_params
	lw $t1, 0x0($t0)
	lw $gp, 0x4($t0)
	lw $sp, 0x8($t0)
	jr $t1
	rfe
