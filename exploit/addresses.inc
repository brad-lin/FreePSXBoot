.equ loaded_program_address, 0x801b0000 # Address where the real program will be loaded
.equ allow_new_card, 0xbfc0da10
.equ read_card_sector, 0xbfc0da00
.equ wait_sector_read, 0xbfc09144
.equ FlushCache, 0x6a70
.equ cd_io_base, 0x1f801800
.equ DeliverEvent, 0x80068750
