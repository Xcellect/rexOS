ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
	. = 0x0100000;
	.text :
	{
		*(.multiboot)
		*(.text*)
		*(.rodata)
	}

	.data :
	{	
		/* start_ctors and end_ctors (contructors of global objects) are 		 * function pointers. These will be known to other object files.
		 * They need to be manually called from the kernel cpp.
		 */
		start_ctors = .;
		KEEP(*( .init_array ));
		KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
		end_ctors = .;

		*(.data)
	}

	.bss :
	{
		*(.bss)
	}
	/DISCARD/ :
	{
		*(.fini_array*)
		*(.comment)
	}
}
