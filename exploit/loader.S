.include "addresses.inc"

.text
.org 0xbe48 # Address where the loader (this program) is loaded

main:
	# Save ra (s3 is saved but not used by the exploited function)
	move $s3, $ra
	# Make screen orange
	la $a0, gpu_words
	li $a1, 3
	li $t2, 0xa0
	li $t1, 0x4a
	jal 0xa0

# Load sectors
.equ first_sector, 17
.equ last_sector, 481 # last sector is not loaded, i.e. range is first_sector up to last_sector - 1
load_sectors:
	# We can use the s registers; the caller has saved them but not used them yet at this point.
	li $s1, first_sector
	la $s2, loaded_program_address
load_sector:
	beq $s1, last_sector, load_sector_end
	li $a0, 0
	move $a1, $s1
	move $a2, $s2
	li $a3, read_card_sector
	addiu $s1, 1
	jalr $a3
	# assume success
	li $a3, wait_sector_read
	addiu $s2, 0x80
	jalr $a3
	b load_sector
load_sector_end:
	# Flush the cache
	la $s2, loaded_program_address
	jal FlushCache
	# Tail call the program
	move $ra, $s3
	jr $s2

gpu_words:
.word 0x0200a5ff #(paint orange)
.word 0x00000000 #(x, y)
.word 0x01ff03ff #(width, height)
