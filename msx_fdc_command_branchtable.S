branchtable_fdc_command:
	.short     ((fdc_command_restore - branchtable_fdc_command)/2) /* 0x00 RESTORE (seek track 0) Command */
	.short     ((fdc_command_seek - branchtable_fdc_command)/2) /* 0x10 SEEK Command */
	.short     ((fdc_command_step - branchtable_fdc_command)/2) /* 0x20 STEP Command */
	.short     ((fdc_command_step_and_update - branchtable_fdc_command)/2) /* 0x30 STEP-AND-UPDATE Command */
	.short     ((fdc_command_step_in - branchtable_fdc_command)/2) /* 0x40 STEP-IN Command */
	.short     ((fdc_command_step_in_and_update - branchtable_fdc_command)/2) /* 0x50 STEP-IN-AND-UPDATE Command */
	.short     ((fdc_command_step_out - branchtable_fdc_command)/2) /* 0x60 STEP-OUT Command */
	.short     ((fdc_command_step_out_and_update - branchtable_fdc_command)/2) /* 0x70 STEP-OUT-AND-UPDATE Command */
	.short     ((fdc_command_read_sector - branchtable_fdc_command)/2) /* 0x80 READ SECTOR Command */
	.short     ((fdc_command_read_sector - branchtable_fdc_command)/2) /* 0x90 READ SECTOR Command */
	.short     ((fdc_command_write_sector - branchtable_fdc_command)/2) /* 0xA0 READ SECTOR Command */
	.short     ((fdc_command_write_sector - branchtable_fdc_command)/2) /* 0xB0 READ SECTOR Command */
	.short     ((fdc_command_read_address - branchtable_fdc_command)/2) /* 0xC0 READ ADDRESS Command */
	.short     ((fdc_command_unused - branchtable_fdc_command)/2) /* 0xD0 FORCE INTERRUPT is dealt with earlier Command */
	.short     ((fdc_command_unused - branchtable_fdc_command)/2) /* 0xE0 READ TRACK Command */
	.short     ((fdc_command_unused - branchtable_fdc_command)/2) /* 0xF0 WRITE TRACK Command */



