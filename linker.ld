/* Purpose: Combine the kernel.cpp object and loader.s object files
and output kernel.bin 
*/
ENTRY(loader)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

/* Collect the sections from different .o files and combine them to new section */
SECTIONS
{
	. = 0x0100000;
	/* text section */
	.text :
	{
		*(.multiboot) /* multiboot section from loader.s */
		*(.text*) /* text sections of the other files */
		*(.rodata)
	}

	.data :
	{	
		/* Data segment is for initialized vars in the object files.
		 * start_ctors and end_ctors (contructors of global objects) are 		
		 * function pointers. These will be known to the object files.
		 * These constructors are for the contstruction of global objects. 
		 * i.e. if there are global vars, this makes sure the ctors are called 
		 */
		start_ctors = .;	
		KEEP(*( .init_array ));
		KEEP(*(SORT_BY_INIT_PRIORITY( .init_array.* )));
		end_ctors = .;

		*(.data)	/* contains the data sections from .o files */
	}

	/* take the bss sections */
	.bss :
	{
		*(.bss)
	}
	/* the linker is told to discard the leftover symbols from converting to binary */
	/DISCARD/ :
	{
		*(.fini_array*)
		*(.comment)		/* remove the comments */
	}
}
