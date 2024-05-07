mbr.s:  load the bootloader
loader.s : 1.detect memory size
	   2.create GDT
	   3.open protected mode    cr0 -> 0x00000001       a20 -> 0x92 -> 0b0000_0010
	   4.create PDT
	   4.create PDE and PTE
	   5.open page mode         cr0 -> 0x80000000

