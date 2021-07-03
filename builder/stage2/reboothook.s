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

	# Patch read_card_sector_callback to make it fail at step 4 (card ID).
	# The original code loads the address of the interrupts register using 2 instructions, but this can be reduced to one.
	# The saved instruction can be overwritten with another one that makes the card ID invalid unless it's in slot 0.
	# We do this by adding $a3 (which contains slot number) to the received value from the memcard.
	# If the slot is 0, the received value (0x5a) is unchanged and the memcard is seen as present.
	# If the slot is 1, the value is changed to 0x5b, and the kernel reports "no card present".

	# There are 2 kernel versions, called here KERNEL1 and KERNEL2.
	# KERNEL1 is in BIOSes up to version 2.0 included.
	# KERNEL2 is in BIOSes from versoin 2.1 up.
	# The code of the patched functions is the same between the 2 kernel versions; only their locations change,
	# as well as the address of interrupt_registers_base.

.ifdef PSX_KERNEL1
	.equ lw_t0_interrupt_registers_base, 0x8c08724c
	.equ read_card_sector_callback_patch_addr, 0x57cc
	.equ write_card_sector_callback_patch_addr, 0x5330
	.equ get_card_info_callback_patch_addr, 0x5ce0
.else
	.equ lw_t0_interrupt_registers_base, 0x8c08725c
	.equ read_card_sector_callback_patch_addr, 0x583c
	.equ write_card_sector_callback_patch_addr, 0x53a0
	.equ get_card_info_callback_patch_addr, 0x5d50
.endif

	.equ lw_t8_interrupt_registers_base, (lw_t0_interrupt_registers_base | 0x00100000)

	la $t0, read_card_sector_callback_patch_addr
	li $t1, lw_t8_interrupt_registers_base
	sw $t1, 0($t0)
	li $t1, 0x00872021 # addu $a0, $a3; a0 is the byte read from MC, a3 is 0 if slot 1 or 1 if slot 2.
	sw $t1, 0x1c($t0)

	# Patch write_card_sector_callback in the same way.
	la $t0, write_card_sector_callback_patch_addr
	li $t1, lw_t8_interrupt_registers_base
	sw $t1, 0($t0)
	li $t1, 0x00661821 # addu $v1, $a2; v1 is the byte read from MC, a2 is 0 if slot 1 or 1 if slot 2.
	sw $t1, 0x1c($t0)

	# Patch get_card_info_callback in the same way.
	la $t0, get_card_info_callback_patch_addr
	li $t1, lw_t0_interrupt_registers_base
	sw $t1, 0($t0)
	li $t1, 0x00641821 # addu $v1, $a0; v1 is the byte read from MC, a0 is 0 if slot 1 or 1 if slot 2.
	sw $t1, 0x1c($t0)

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
