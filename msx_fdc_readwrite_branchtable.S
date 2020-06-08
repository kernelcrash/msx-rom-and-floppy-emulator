branchtable_fdc:
// READS
	.short     ((fdc_status_read - branchtable_fdc)/2) /* 0x7FF8 Status */
	.short     ((fdc_track_read - branchtable_fdc)/2) /* 0x7FF9 Track */
	.short     ((fdc_sector_read - branchtable_fdc)/2) /* 0x7FFA Sector */
	.short     ((fdc_data_read - branchtable_fdc)/2) /* 0x7FFB Data */
	.short     ((fdc_unused_read - branchtable_fdc)/2) /* 0x7FFC */
	.short     ((fdc_0x7ffd_read - branchtable_fdc)/2) /* 0x7FFD */
	.short     ((fdc_unused_read - branchtable_fdc)/2) /* 0x7FFE */
	.short     ((fdc_ready_read - branchtable_fdc)/2) /* 0x7FFF non FDC control*/
// WRITES
	.short     ((fdc_command_write - branchtable_fdc)/2) /* 0x7FF8 Command */
	.short     ((fdc_track_write - branchtable_fdc)/2) /* 0x7FF9 Track */
	.short     ((fdc_sector_write - branchtable_fdc)/2) /* 0x7FFA Sector */
	.short     ((fdc_data_write - branchtable_fdc)/2) /* 0x7FFB Data */
	.short     ((fdc_side_write - branchtable_fdc)/2) /* 0x7FFC */
	.short     ((fdc_driveselect_write - branchtable_fdc)/2) /* 0x7FFD */
	.short     ((fdc_unused_write - branchtable_fdc)/2) /* 0x7FFE */
	.short     ((fdc_unused_write - branchtable_fdc)/2) /* 0x7FFF non FDC control*/



