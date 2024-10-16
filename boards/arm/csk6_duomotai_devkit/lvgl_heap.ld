SECTION_PROLOGUE(lvgl_section, (NOLOAD),)
{
	. = ALIGN(32);
	KEEP(*(.lvgl_heap))
	. = ALIGN(32);
} GROUP_LINK_IN(PSRAMAP)
