#include "util.h"

// probably have to tweak these if the processor speed changes. Just have to be rough
void delay_us(const uint16_t us)
{
   uint32_t i = us * 60;
   while (i-- > 0) {
      __asm volatile ("nop");
   }
}

void delay_ms(const uint16_t ms)
{
   //uint32_t i = ms * 27778;
   uint32_t i = ms * 30000 *2;
   while (i-- > 0) {
      __asm volatile ("nop");
   }
}

void blink_pa6_pa7(int delay) {
        while(1) {
                GPIOA->ODR = 0x0040;
                delay_ms(delay);
                GPIOA->ODR = 0x0080;
                delay_ms(delay);
        }
}

void blink_pa6(int delay) {
        while(1) {
                GPIOA->ODR = 0x0040;
                delay_ms(delay);
                GPIOA->ODR = 0x0000;
                delay_ms(delay);
        }
}

void blink_pa7(int delay) {
        while(1) {
                GPIOA->ODR = 0x0080;
                delay_ms(delay);
                GPIOA->ODR = 0x0000;
                delay_ms(delay);
        }
}

void blink_pa1(int delay) {
        while(1) {
                GPIOA->ODR = 0x0002;
                delay_ms(delay);
                GPIOA->ODR = 0x0000;
                delay_ms(delay);
        }
}


uint32_t suffix_match(char *name, char *suffix) {
	if (strlen(name)>strlen(suffix)) {
		if (strncasecmp (&name[strlen(name)-strlen(suffix)],suffix,strlen(suffix)) == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

uint32_t get_romtype_from_filename(char *fname) {
	uint32_t 	romtype;

	romtype = ROM_TYPE_GENERIC;
	if (suffix_match(fname,ROM_SUFFIX_KONAMI4)) {
		romtype = ROM_TYPE_KONAMI4;
	}
	if (suffix_match(fname,ROM_SUFFIX_KONAMI5)) {
		romtype = ROM_TYPE_KONAMI5;
	}
	if (suffix_match(fname,ROM_SUFFIX_ASCII8)) {
		romtype = ROM_TYPE_ASCII8;
	}
	if (suffix_match(fname,ROM_SUFFIX_ASCII16)) {
		romtype = ROM_TYPE_ASCII16;
	}
	if (strncmp(fname,"menu.rom",8) == 0) {
		romtype = ROM_TYPE_MENUROM;
	}

	return romtype;
}

uint32_t get_pagemap_default_by_romtype(uint32_t romtype) {
	switch(romtype) {
		case ROM_TYPE_MENUROM :
			return 0x00001032;
		case ROM_TYPE_GENERIC :
			return 0x00001032;
		case ROM_TYPE_KONAMI4 :
			return 0x00001000;
		case ROM_TYPE_KONAMI5 :
			return 0x00001032;
		case ROM_TYPE_ASCII8:
			return 0x00000000;
		case ROM_TYPE_ASCII16:
			return 0x00000000;
		default:
			return 0x00001032;

	}
}

char *find_disk_n(char *haystack, int n) {
	char *p;

	p = haystack;
	while (*p) {
		if ((*p== 'd') || (*p=='D')) {
			if ((p[1]=='i') || (p[1]=='I')) {
				if ((p[2]=='s') || (p[2]=='S')) {
					if ((p[3]=='k') || (p[3]=='K')) {
						if (((p[4]==' ') && (p[5]==(n+'0'))) || (p[4]==(n+'0'))) {
							return p;
						} else {
							// exit if we matched all the way to Disk, but failed on the number
							// This is so that a game filename like "Some Game (1990)(Blah Blah)(Disk 4 of 4)(Game Disk 1).dsk" will only attempt to match the first case of the word 'Disk'
							return NULL;
						}
					}
				}
			}
		}
		p++;
	}
	return NULL;
}

/*
 * load_disks
 * if filename is a path to a directory then look inside that directory for
 *    something to load into dsk[0]
 *    something to load into dsk[1]
 *
 *    For dsk[0] this can be a file with the string 'Disk n' in the filename (eg. game disk 1.dsk or
 *    game disk 2.dsk and so on. The numeric is based on what disk_index is set to. ie. 1, means 
 *    load game disk 1.dsk.
 *
 *    For dsk[1] it only looks for a file ending in .dsk2 . The expectation is that most games wont
 *    look for the 2nd or 3rd disks in the 2nd drive.
 *
 *    disk_index is a pointer to a uint32_t. So is disk_index_max. The idea is that as it scans the subdir
 *    looking for the filename that matches with 'Disk n', it also calculates the highest number for 'Disk n'
 *    (eg. Disk 6). That highest number '6' is then stored in disk_index_max
 *
 * if the filename is a path to a file
 *
 *    check if the filename ends in .dsk or .dsk1 and load it into dsk[0] only
 *
 * if no dsk files are found at all return '0' (ie failure)
 * */
uint32_t load_disks(DSK *dsk,char *filename,uint32_t *disk_index, uint32_t *disk_index_max) {
	FILINFO fno;
	FRESULT res;
	DIR	dir;
	// tmp_disk_n_name is the disk n we are looking for
	// tmp_disk_n_plus_1_name is the disk name that is n+1 (eg. if we were searching for Disk 1, then this would be used for Disk 2. It is a sort of additional fallback for disk sets where you leave out disk 1 (which might be an intro) and the main game starts at disk 2
	char 	tmp_disk_n_name[128],tmp_disk_n_plus_1_name[128],fallback_fname[128];
	uint32_t	min,max;

	tmp_disk_n_name[0]=0;
	tmp_disk_n_plus_1_name[0]=0;
	fallback_fname[0]=0;


	res = f_stat(filename,&fno);
	if (res == FR_OK) {
		dsk[1].disk_filename[0]=0;
		res = f_opendir(&dir,filename);
		if (res == FR_OK) {
			max = 0;
			min = 100; // arbitrary high number
			for (;;) {
				res = f_readdir(&dir,&fno);
				if (res != FR_OK || fno.fname[0] == 0) break;
				if (suffix_match(fno.fname,".dsk")) {
					// If disk_index is 1 on entry it means we want to load 'the first disk' regardless of whether it has 'Disk 1' in the filename
					// If disk_index is 0 on entry it means we don't know if there are any disks, but if there is one then we would want to load 'Disk 1' OR 'the first .dsk it sees
					if ((*disk_index == 1) || (*disk_index == 0)) {
						// disk_index 1 is a special case where we allow the first file we see as disk 1
						*disk_index = 1;
						strcpy(fallback_fname,fno.fname);
					}
					// work out the highest numbered disk
					for (uint32_t i=9;i>0;i--) {
						if (find_disk_n(fno.fname,i)!=NULL) {
							if (i>max) {
								max = i;
							}
							if (i<min) {
								min = i;
							}
						}
					}
					if (find_disk_n(fno.fname,*disk_index)!=NULL) {
						// Disk n found
						strcpy(tmp_disk_n_name,fno.fname);
					}
					if (find_disk_n(fno.fname,(*disk_index)+1)!=NULL) {
						// Disk n+1 found
						strcpy(tmp_disk_n_plus_1_name,fno.fname);
					}


				}
				if (suffix_match(fno.fname,".dsk2")) {
					strcpy(dsk[1].disk_filename,filename);
					strcat(dsk[1].disk_filename,"/");
					strcat(dsk[1].disk_filename,fno.fname);
				}
			}
			f_closedir(&dir);
			if (max > 0) {
				*disk_index_max = max;
			} else {
				*disk_index_max = 1;
			}

			if (tmp_disk_n_name[0]) {
				// disk n was found
				strcpy(dsk[0].disk_filename,filename);
				strcat(dsk[0].disk_filename,"/");
				strcat(dsk[0].disk_filename,tmp_disk_n_name);
			} else {
				if (tmp_disk_n_plus_1_name[0]) {
					// we could not find the disk n we wanted but we did find n+1
					strcpy(dsk[0].disk_filename,filename);
					strcat(dsk[0].disk_filename,"/");
					strcat(dsk[0].disk_filename,tmp_disk_n_plus_1_name);
					(*disk_index)++;  // push the disk number up. TODO this will probably screw up the PREV button working completely correctly
				} else {
					if (fallback_fname[0]) {
						strcpy(dsk[0].disk_filename,filename);
						strcat(dsk[0].disk_filename,"/");
						strcat(dsk[0].disk_filename,fallback_fname);
					} else {
						// we didnt find anything
						dsk[0].disk_filename[0]=0;
						dsk[1].disk_filename[0]=0;

						*disk_index=0;		// TODO. not sure if this is required.

						return NO_DISKS_FOUND;
					}
				}
			}
			// close both drives
			for (int drive=0;drive<MAX_DRIVES;drive++) {
				if (dsk[drive].fil.obj.id) {
					f_close(&dsk[drive].fil);
					memset(&dsk[drive].fil, 0, sizeof(FIL));
				}
			}

			res = f_open(&dsk[0].fil, dsk[0].disk_filename, FA_READ);
			if (res != FR_OK) blink_pa1(1500);
			dsk[0].double_sided = (f_size(&dsk[0].fil) == SINGLE_SIDED_DISK_SIZE)? 0 : 1;
			if (dsk[1].disk_filename[0]) {
				res = f_open(&dsk[1].fil, dsk[1].disk_filename, FA_READ);
				if (res != FR_OK) blink_pa1(1500);
				dsk[1].double_sided = (f_size(&dsk[1].fil) == SINGLE_SIDED_DISK_SIZE)? 0 : 1;
			}
			return DISKS_FOUND;
		} else if (suffix_match(fno.fname,".dsk")) {
			// Must be a dsk file
			*disk_index = 0;		// default it to Disk 1
			*disk_index_max = 0;		// so when the person hits NEXT it will go to the next file or subdir
			// close both drives
			for (int drive=0;drive<MAX_DRIVES;drive++) {
				if (dsk[drive].fil.obj.id) {
					f_close(&dsk[drive].fil);
					memset(&dsk[drive].fil, 0, sizeof(FIL));
				}
			}
			strcpy(dsk[0].disk_filename,filename);
			res = f_open(&dsk[0].fil, dsk[0].disk_filename, FA_READ);
			dsk[0].double_sided = (f_size(&dsk[0].fil) == SINGLE_SIDED_DISK_SIZE)? 0 : 1;
			return DISK1_FOUND;
		} else {
			return NO_DISKS_FOUND;
		}
	} else {
		return 0;
	}


}

// Load fname file with the first 64k going to lowbuffer , and the next 64k going to highbuffer
FRESULT load_rom(char *fname,char *lowbuffer, char* highbuffer) {
	FRESULT res;
        FIL     fil;
        UINT BytesRead;
	volatile uint32_t romtype;
	volatile uint32_t pagemap_default;

	memset(&fil, 0, sizeof(FIL));

	romtype = get_romtype_from_filename(fname);
	pagemap_default = get_pagemap_default_by_romtype(romtype);

	// set romtype in upper portion of page map registers
	__asm volatile("\t vmov "XSTR(reg_mapper_pages)", %0" :: "rm"(pagemap_default | (romtype <<29)));



	res =  f_open(&fil, fname, FA_READ);

	if (res == FR_OK) {
#ifdef ENABLE_SEMIHOSTING
                printf("TEST.ROM opened OK\n");
#endif

		// f_read into the highbuffer since it won't work into ccmram
		res = f_read(&fil,highbuffer, 0x10000, &BytesRead);
		if (res != FR_OK) {
			blink_pa6(3000);
		} else {
#ifdef ENABLE_SEMIHOSTING
	                printf("Read %d bytes in to %0x08x\n",BytesRead,highbuffer);
#endif
		}
		
		// copy what we jsut read to ccmram
		for (int i=0;i<0x10000;i++) {
			lowbuffer[i]=highbuffer[i];
		}
		// Only read second 64K if necessary
		if (BytesRead == 0x10000) {
			res = f_read(&fil,highbuffer, 0x10000, &BytesRead);
			if (res != FR_OK) {
				blink_pa7(4000);
			} else {
#ifdef ENABLE_SEMIHOSTING
		                printf("Read %d bytes in to %0x08x\n",BytesRead,highbuffer);
#endif

			}

		}
	}
	f_close(&fil);
	return res;
}

// buffer[0] is the command/status
// buffer[2] is the number of entries read
// buffer[0x80 - 0xff] is where the menu program writes the rom/dsk to load
// buffer[0x100 - 0x17f] is the first file in the directory
// buffer[0x180 - 0x1ff] is the 2nd file
// buffer[0x200 - 0x27f] is the 3rd and so on
// buffer should be 16K so at 128bytes per filename you kind of are limited to 126 files
void load_directory(char *dirname, unsigned char*buffer) {
	FRESULT res;
        DIR dir;
        static FILINFO fno;
	int file_index;
	int i;

	memset(&dir, 0, sizeof(DIR));
        res = f_opendir(&dir, (TCHAR *) dirname);
        if (res != FR_OK) {
                blink_pa6_pa7(2000);
        }

	file_index=0;
	while (file_index<126) {
		res = f_readdir(&dir, &fno);
		if (res != FR_OK || fno.fname[0] == 0) {
			break;
		}
		i=0;
		do {
			buffer[0x100+(file_index*0x80)+i] = fno.fname[i];
			if (i>126) {
				buffer[0x100+(file_index*0x100)+i]=0;
				break;
			}
		} while (fno.fname[i++]!=0);
		file_index++;
	}
	buffer[0x02] = (unsigned char) file_index;	// write total number of files read
	buffer[0x03] = 0;				// this effectively makes it a 16 bit number (ie. the menu program can use an int.
	res = f_closedir(&dir);
}

