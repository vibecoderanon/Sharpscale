OUTPUT_ARCH(aarch64)
ENTRY(_start)

PHDRS
{
	code   PT_LOAD FLAGS(5) /* Read | Execute */;
	data   PT_LOAD FLAGS(6) /* Read | Write */;
	dyn    PT_DYNAMIC;
}

SECTIONS
{
	PROVIDE(__start__ = 0x0);
	. = __start__;

	.text :
	{
		KEEP (*(.crt0))
		. = ALIGN(8);
		*(.text .text.*)
		. = ALIGN(8);
	} :code

	.rodata :
	{
		*(.rodata .rodata.*)
		. = ALIGN(8);
	} :code

	. = ALIGN(0x1000);

	.dynamic : { *(.dynamic) } :data :dyn
	.dynsym  : { *(.dynsym) } :data :dyn
	.dynstr  : { *(.dynstr) } :data :dyn
	.rela.dyn : { *(.rela.*) } :data :dyn

	.data :
	{
		*(.data .data.*)
		. = ALIGN(8);
	} :data

	.bss :
	{
		__bss_start__ = .;
		*(.bss .bss.*)
		*(COMMON)
		__bss_end__ = .;
	} :data
}
