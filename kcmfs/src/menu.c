/*
 * KCMFS
 *
 *
 */

#define KCMFS_VERSION	"0.1"
#define STR1(X)
#define STR(X) STR1(X)

#include "conio.h"
//#include "dos.h"


unsigned char getjoy() {
  __asm
        push af
1$:     xor a
        call 0x00d8     ; triggers . 0 = test space
        or a
        jr z, 2$
// space bar pressed
        ld l,#0x80      ; return bit 7 high if trigger pressed
        jr 3$

// test cursor direction
2$:     call 0x00d5
        or a
        jr z, 1$
        ld l,a
3$:     pop af
  __endasm;

}


void my_puts(char *s) {
        while (*s != 0) {
                putchar(*s);
                s++;
        }
}

// From https://www.msx.org/forum/msx-talk/development/problem-sdcc-compiler-ddexe-and-makefileexe?page=7
void cls2(void)
{
  putchar(0x1b);
  putchar(0x45);
}

// Look at map.grauw.nl/resources/msxbios.php
//
#define NUMBER_OF_DIGITS 16  

#define MAX_PAGES 9

void uitoa(unsigned int value, char* string, int radix)
{
  unsigned char index, i;

  index = NUMBER_OF_DIGITS;
  i = 0;

  do {
    string[--index] = '0' + (value % radix);
    if ( string[index] > '9') string[index] += 'A' - ':';   /* continue with A, B,.. */
    value /= radix;
  } while (value != 0);

  do {
    string[i++] = string[index++];
  } while ( index < NUMBER_OF_DIGITS );

  string[i] = 0; /* string terminator */
}

void help_page() {
	cls();
    	my_puts("== Kernelcrash MSX File Selector ==\r\n");
    	my_puts("===================================\r\n");
	my_puts(" ?       - this help page\r\n\r\n");
	my_puts(" A - Z   - select a rom or dsk\r\n");
        my_puts("           and reboot on to it\r\n\r\n");
	my_puts(" 1 - 9   - select a page of files\r\n\r\n");

}

#define TRIGGER_DIRECTORY_LOAD	0x80
#define TRIGGER_FILE_LOAD	0x40

#define FNAME_ENTRY_LENGTH 	0x80


void main(void) {
	int redraw = 1;
	int c;
	char buf[256];
	unsigned char *cmd_reg = (unsigned char *) 0x8000;
	unsigned int *fname_count = (unsigned int *) 0x8002;
	unsigned char *fname_to_load = (unsigned char *) 0x8080;
	unsigned char *fname_list =(unsigned char *) 0x8100;
	unsigned int i,j;
	unsigned int page_number, page_size, offset;

#ifdef JOYMODE
        char selection = 'a';
#endif
	page_number=1;
	page_size=20;


	*cmd_reg = TRIGGER_DIRECTORY_LOAD;
	// set 40 column
	initxt();
#ifndef DISABLE_LOAD_DIRECTORY
	// Wait for directory list to load
	for (i=0;i<65535;i++) {
		if (!(*cmd_reg & 0x80)) {
			break;
		}
		for (j=0;j<1000;j++);
	}
#endif


	while (1) {
		if (redraw) {
			cls2();
			my_puts("== Kernelcrash MSX File Selector ==\r\n");
			my_puts("===================================\r\n");
			for (i=0; i<page_size;i++) {
				buf[0]='a'+i;
				buf[1]='.';
				j=0;
				if ((( (page_number-1)*page_size)+i) < *fname_count) {
					while (j < 34) {
						offset = (((page_number-1)*page_size)*FNAME_ENTRY_LENGTH)+ (i* FNAME_ENTRY_LENGTH);
						if (fname_list[offset+j]) {
							buf[j+2]=fname_list[offset+j];
						} else {
							break;
						}
						j++;
					}
				}
				buf[j+2]='\r';
				buf[j+3]='\n';
				buf[j+4]=0;
				my_puts(buf);
			}
			my_puts("<<<<< PAGE ");
			uitoa(page_number,buf,10);
			my_puts(buf);
			my_puts(" >>>>>   (? - help) ");
			putchar(selection);
			putchar(' ');
		}


#ifdef JOYMODE
                c = getjoy();
#else
                c = getchar();
#endif

#ifdef JOYMODE
                if (c==3) {
                        redraw=1;
                        page_number = (page_number <=MAX_PAGES)? page_number+1 : MAX_PAGES;
                        continue;
                }
                if (c==7) {
                        redraw=1;
                        page_number = (page_number>1)? page_number-1 : 1;
                        continue;
                }
                if ((c==5) || (c==1)) {
                        redraw=0;
                        if (c==5) {
                                selection = (selection=='t')? 't' : selection+1;
                        } else {
                                selection = (selection=='a')? 'a' : selection-1;
                        }
                        putchar(0x1d);
                        putchar(0x1d);
                        putchar(selection);
                        putchar(' ');
                        for (i=0;i<10000;i++) {
				 __asm__ ("nop");
				 __asm__ ("nop");
				 __asm__ ("nop");
			}
                }
                if (c==0x80) {
                        offset = (((page_number-1)*page_size)*FNAME_ENTRY_LENGTH)+ ((selection-'a')* FNAME_ENTRY_LENGTH);
                        for (j=0;j<FNAME_ENTRY_LENGTH;j++) {
                                if (fname_list[offset+j]!=0) {
                                        fname_to_load[j]=fname_list[offset+j];
                                } else {
                                        fname_to_load[j]=0;
                                        break;
                                }
                        }
                        trigger_file_load_and_reset();
                }
#else
		c = getchar();
		// check page number
		if ((c>='1') && (c<='9')) {
			page_number = c-'0';
			continue;
		}
		// check for help screen
		if (c=='?') {
			help_page();
			c= getchar();
			continue;
		}
		// check for file selection
		if ((c>='a') && (c<='z')) {
			offset = (((page_number-1)*page_size)*FNAME_ENTRY_LENGTH)+ ((c-'a')* FNAME_ENTRY_LENGTH);
			for (j=0;j<FNAME_ENTRY_LENGTH;j++) {
				if (fname_list[offset+j]!=0) {
					fname_to_load[j]=fname_list[offset+j];
				} else {
					fname_to_load[j]=0;
					break;
				}
			}
			trigger_file_load_and_reset();
		}
#endif
	
	}
}




