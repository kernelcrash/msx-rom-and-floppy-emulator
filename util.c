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
	if (suffix_match(fname,ROM_SUFFIX_ASCII8)) {
		romtype = ROM_TYPE_ASCII8;
	}
	if (suffix_match(fname,ROM_SUFFIX_ASCII16)) {
		romtype = ROM_TYPE_ASCII16;
	}
	return romtype;
}

uint32_t get_pagemap_default_by_romtype(uint32_t romtype) {
	switch(romtype) {
		case ROM_TYPE_GENERIC :
			return 0x00001032;
		case ROM_TYPE_KONAMI4 :
			return 0x00001000;
		case ROM_TYPE_ASCII8:
			return 0x00000000;
		case ROM_TYPE_ASCII16:
			return 0x00000000;
		default:
			return 0x00001032;

	}
}

// Load fname file with the first 64k going to lowbuffer , and the next 64k going to highbuffer
void load_rom(char *fname,char *lowbuffer, char* highbuffer) {
	FRESULT res;
        FIL     fil;
        UINT BytesRead;
	volatile uint32_t romtype;
	volatile uint32_t pagemap_default;

	memset(&fil, 0, sizeof(FIL));

	romtype = get_romtype_from_filename(fname);
	pagemap_default = get_pagemap_default_by_romtype(romtype);

	// set romtype in upper portion of page map registers
	__asm volatile("\t vmov "XSTR(reg_mapper_pages)", %0" :: "rm"(pagemap_default | (romtype <<30)));



	res =  f_open(&fil, fname, FA_READ);

	if (res == FR_OK) {
#ifdef ENABLE_SEMIHOSTING
                printf("TEST.ROM opened OK\n");
#endif

		// f_read into the highbuffer since it won't work into ccmram
		res = f_read(&fil,highbuffer, 0x10000, &BytesRead);
		if (res != FR_OK) {
			blink_pa6(500);
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
				blink_pa7(500);
			} else {
#ifdef ENABLE_SEMIHOSTING
		                printf("Read %d bytes in to %0x08x\n",BytesRead,highbuffer);
#endif

			}

		}
	}
	f_close(&fil);
}



