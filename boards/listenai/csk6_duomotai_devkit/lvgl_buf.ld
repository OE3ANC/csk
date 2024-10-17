SECTION_PROLOGUE(lvgl_section, (NOLOAD),)
{
	. = ALIGN(32);
	KEEP(*(.lvgl_buf))
	. = ALIGN(32);
} GROUP_LINK_IN(PSRAMAP)
