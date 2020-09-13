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

uint32_t suffix_match(char *name, char *suffix) {
	if (strlen(name)>strlen(suffix)) {
		if (strncmp (&name[strlen(name)-strlen(suffix)],suffix,strlen(suffix)) == 0) {
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

