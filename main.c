/*
 * msx-rom-and-floppy-emulator
 * kernel@kernelcrash.com
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "defines.h"
#include "stm32f4xx.h"
#include "util.h"

#ifdef ENABLE_OLED_DISPLAY
#include "i2c.h"
#include "oled.h"
#endif

#include "stm32f4_discovery.h"
#include "stm32f4_discovery_sdio_sd.h"
#include "ff.h"
#include "diskio.h"



extern volatile uint8_t *rom_base;
extern volatile uint8_t *low_64k_base;
extern volatile uint8_t *high_64k_base;

extern volatile uint32_t main_thread_command;
extern volatile uint32_t main_thread_data;
extern volatile uint32_t main_thread_actual_track[2];

extern volatile uint8_t *track_buffer;
extern volatile uint32_t fdc_write_flush_count;
extern volatile uint32_t fdc_write_dirty_bits;


#ifdef ENABLE_SEMIHOSTING
extern void initialise_monitor_handles(void);   /*rtt*/
#endif

GPIO_InitTypeDef  GPIO_InitStructure;

// FATFS stuff
FRESULT res;
FILINFO fno;
//DIR dir;
FATFS fs32;
char* path;
UINT BytesRead;

uint32_t disk_index;
uint32_t disk_index_max;

#ifdef DEBUG_FILENAME
	char debug_filename[128];
#endif

#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
        fno.lfname = lfn;
            fno.lfsize = sizeof lfn;
#endif


// Enable the FPU (Cortex-M4 - STM32F4xx and higher)
// http://infocenter.arm.com/help/topic/com.arm.doc.dui0553a/BEHBJHIG.html
// Also make sure lazy stacking is disabled
void enable_fpu_and_disable_lazy_stacking() {
  __asm volatile
  (
    "  ldr.w r0, =0xE000ED88    \n"  /* The FPU enable bits are in the CPACR. */
    "  ldr r1, [r0]             \n"  /* read CAPCR */
    "  orr r1, r1, #( 0xf << 20 )\n" /* Set bits 20-23 to enable CP10 and CP11 coprocessors */
    "  str r1, [r0]              \n" /* Write back the modified value to the CPACR */
    "  dsb                       \n" /* wait for store to complete */
    "  isb                       \n" /* reset pipeline now the FPU is enabled */
    // Disable lazy stacking (the default) and effectively have no stacking since we're not really using the FPU for anything other than a fast register store
    "  ldr.w r0, =0xE000EF34    \n"  /* The FPU FPCCR. */
    "  ldr r1, [r0]             \n"  /* read FPCCR */
    "  bfc r1, #30,#2\n" /* Clear bits 30-31. ASPEN and LSPEN. This disables lazy stacking */
    "  str r1, [r0]              \n" /* Write back the modified value to the FPCCR */
    "  dsb                       \n" /* wait for store to complete */
    "  isb"                          /* reset pipeline  */
    :::"r0","r1"
    );
}

enum sysclk_freq {
    SYSCLK_42_MHZ=0,
    SYSCLK_84_MHZ,
    SYSCLK_168_MHZ,
    SYSCLK_200_MHZ,
    SYSCLK_240_MHZ,
};
 
void rcc_set_frequency(enum sysclk_freq freq)
{
    int freqs[]   = {42, 84, 168, 200, 240};
 
    /* USB freqs: 42MHz, 42Mhz, 48MHz, 50MHz, 48MHz */
    int pll_div[] = {2, 4, 7, 10, 10}; 
 
    /* PLL_VCO = (HSE_VALUE / PLL_M) * PLL_N */
    /* SYSCLK = PLL_VCO / PLL_P */
    /* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
    uint32_t PLL_P = 2;
    uint32_t PLL_N = freqs[freq] * 2;
    uint32_t PLL_M = (HSE_VALUE/1000000);
    uint32_t PLL_Q = pll_div[freq];
 
    RCC_DeInit();
 
    /* Enable HSE osscilator */
    RCC_HSEConfig(RCC_HSE_ON);
 
    if (RCC_WaitForHSEStartUp() == ERROR) {
        return;
    }
 
    /* Configure PLL clock M, N, P, and Q dividers */
    RCC_PLLConfig(RCC_PLLSource_HSE, PLL_M, PLL_N, PLL_P, PLL_Q);
 
    /* Enable PLL clock */
    RCC_PLLCmd(ENABLE);
 
    /* Wait until PLL clock is stable */
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);
 
    /* Set PLL_CLK as system clock source SYSCLK */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
 
    /* Set AHB clock divider */
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
 
    //FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;
    FLASH->ACR =  FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

    /* Set APBx clock dividers */
    switch (freq) {
        /* Max freq APB1: 42MHz APB2: 84MHz */
        case SYSCLK_42_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div1); /* 42MHz */
            RCC_PCLK2Config(RCC_HCLK_Div1); /* 42MHz */
            break;
        case SYSCLK_84_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div2); /* 42MHz */
            RCC_PCLK2Config(RCC_HCLK_Div1); /* 84MHz */
            break;
        case SYSCLK_168_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div4); /* 42MHz */
            RCC_PCLK2Config(RCC_HCLK_Div2); /* 84MHz */
            break;
        case SYSCLK_200_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div4); /* 50MHz */
            RCC_PCLK2Config(RCC_HCLK_Div2); /* 100MHz */
            break;
        case SYSCLK_240_MHZ:
            RCC_PCLK1Config(RCC_HCLK_Div4); /* 60MHz */
            RCC_PCLK2Config(RCC_HCLK_Div2); /* 120MHz */
            break;
    }
 
    /* Update SystemCoreClock variable */
    SystemCoreClockUpdate();
}


void config_backup_sram() {
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_BKPSRAM, ENABLE);
  /* disable backup domain write protection */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);   // set RCC->APB1ENR.pwren
  PWR_BackupAccessCmd(ENABLE);                          // set PWR->CR.dbp = 1;
  /** enable the backup regulator (used to maintain the backup SRAM content in
    * standby and Vbat modes).  NOTE : this bit is not reset when the device
    * wakes up from standby, system reset or power reset. You can check that
    * the backup regulator is ready on PWR->CSR.brr, see rm p144 */
  PWR_BackupRegulatorCmd(ENABLE);     // set PWR->CSR.bre = 1;
}


// For some weird reason the optimizer likes to delete this whole function, hence set to O0
void __attribute__((optimize("O0")))  SD_NVIC_Configuration(void)
{
        NVIC_InitTypeDef NVIC_InitStructure;


        NVIC_InitStructure.NVIC_IRQChannel = SDIO_IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SDIO_IRQ_PREEMPTION_PRIORITY;    // This must be a lower priority (ie. higher number) than the _MREQ and _IORQ interrupts
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

	// DMA2 STREAMx Interrupt ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = SD_SDIO_DMA_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = SDIO_DMA_PREEMPTION_PRIORITY;
	NVIC_Init(&NVIC_InitStructure);

}

void SDIO_IRQHandler(void)
{
	/* Process All SDIO Interrupt Sources */
	SD_ProcessIRQSrc();
}

void SD_SDIO_DMA_IRQHANDLER(void)
{
	SD_ProcessDMAIRQ();
}

//
// _IORQ interrupt
void config_PC0_int(void) {
        EXTI_InitTypeDef EXTI_InitStruct;
        NVIC_InitTypeDef NVIC_InitStruct;

        /* Enable clock for SYSCFG */
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource0);

        EXTI_InitStruct.EXTI_Line = EXTI_Line0;
        /* Enable interrupt */
        EXTI_InitStruct.EXTI_LineCmd = ENABLE;
        /* Interrupt mode */
        EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
        /* Triggers on rising and falling edge */
        //EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
        /* Add to EXTI */
        EXTI_Init(&EXTI_InitStruct);

        /* Add IRQ vector to NVIC */
        /* PC0 is connected to EXTI_Line0, which has EXTI0_IRQn vector */
        NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
        /* Set priority */
        NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = IORQ_PREEMPTION_PRIORITY;
        /* Set sub priority */
        NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
        /* Enable interrupt */
        NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        /* Add to NVIC */
        NVIC_Init(&NVIC_InitStruct);
}


// _MREQ interrupt
void config_PC2_int(void) {
        EXTI_InitTypeDef EXTI_InitStruct;
        NVIC_InitTypeDef NVIC_InitStruct;

        /* Enable clock for SYSCFG */
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

        SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource2);

        EXTI_InitStruct.EXTI_Line = EXTI_Line2;
        /* Enable interrupt */
        EXTI_InitStruct.EXTI_LineCmd = ENABLE;
        /* Interrupt mode */
        EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
        /* Triggers on rising and falling edge */
        //EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
        /* Add to EXTI */
        EXTI_Init(&EXTI_InitStruct);

        /* Add IRQ vector to NVIC */
        /* PC2 is connected to EXTI_Line2, which has EXTI2_IRQn vector */
        NVIC_InitStruct.NVIC_IRQChannel = EXTI2_IRQn;
        /* Set priority */
        NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = MREQ_PREEMPTION_PRIORITY;
        /* Set sub priority */
        NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;
        /* Enable interrupt */
        NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
        /* Add to NVIC */
        NVIC_Init(&NVIC_InitStruct);
}

void config_gpio_portb(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOC Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_USART3, DISABLE);

	/* Configure GPIO Settings */
	// Make sure to init the PS2 keyboard pins here.
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12
					| GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIOB->ODR = 0xFFFF;
}

void config_gpio_portc(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOC Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* Configure GPIO Settings */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIOC->ODR = BUSDIR_HIGH;
}

/* Input/Output data GPIO pins on PD{8..15}. Also PD2 is used fo MOSI on the STM32F407VET6 board I have */
void config_gpio_data(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOD Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* Configure GPIO Settings */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | 
		GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

}

/* Input Address GPIO pins on PE{0..15} */
void config_gpio_addr(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOE Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	/* Configure GPIO Settings */
	GPIO_InitStructure.GPIO_Pin = 
		GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | 
		GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | 
		GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | 
		GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

#ifdef ENABLE_KEYBOARD_EMULATION
/* Input Address GPIO pins on PD{4..7}. */
/* I dont think PA0 can be used as an input */
void config_gpio_keyboard_row(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOE Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* Configure GPIO Settings */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}
#endif


/* Debug GPIO pins on PA6  / PA7 / PA8 */
void config_gpio_dbg(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOA Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,DISABLE);


	/* Configure GPIO Settings */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/* push buttons on PA0, PA1 */
void config_gpio_buttons(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;
	/* GPIOA Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);


	/* Configure GPIO Settings */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}



FRESULT __attribute__((optimize("O0")))  load_track(DSK *dsk, uint32_t disk_selects, uint32_t track_number, char *track_buffer) {
        UINT BytesRead;
        FRESULT res;
	uint32_t offset;
	uint32_t track_size;
	uint32_t drive;
	char *b;

	b = track_buffer;
	for (drive=0; drive<MAX_DRIVES;drive++) {
		if ((1<<drive) & disk_selects) {
			if (dsk[drive].fil.obj.id) {	
				if (!dsk[drive].double_sided) {
					// single sided
					offset = track_number * 9 * 512 * 1;
					track_size = SINGLE_SIDED_TRACK;
				} else {
					// double sided
					offset = track_number * 9 * 512 * 2;
					track_size = DOUBLE_SIDED_TRACK;
				}

				res = f_lseek(&dsk[drive].fil, offset);
				if (res == FR_OK) {
					res = f_read(&dsk[drive].fil, b, track_size, &BytesRead);
				} else {
       				         blink_pa6_pa7(1000);
				}
			}
		}
		b+=DOUBLE_SIDED_TRACK;
	}

	return res;
}

FRESULT __attribute__((optimize("O0"))) save_track(DSK *dsk, volatile uint32_t *actual_track, char *track_buffer) {
        UINT BytesWritten;
        FRESULT res;
	uint32_t offset;
	uint32_t track_size;
	uint32_t drive;
	char *b;

	b = track_buffer;

	for (drive=0; drive<MAX_DRIVES;drive++) {
		if ((1<<drive) & fdc_write_dirty_bits) {
			if (dsk[drive].fil.obj.id) {
				f_close(&dsk[drive].fil);
				memset(&dsk[drive].fil, 0, sizeof(FIL));
			

				if (!dsk[drive].double_sided) {
					// single sided
					offset = actual_track[drive] * 9 * 512 * 1;
					track_size = SINGLE_SIDED_TRACK;
				} else {
					// double sided
					offset = actual_track[drive] * 9 * 512 * 2;
					track_size = DOUBLE_SIDED_TRACK;
				}

				res = f_open(&dsk[drive].fil, dsk[drive].disk_filename, FA_WRITE);
				if (res != FR_OK) { blink_pa1(3000);}

				res = f_lseek(&dsk[drive].fil, offset);
				if (res == FR_OK) {
					res = f_write(&dsk[drive].fil, b, track_size, &BytesWritten);
				} else {
					blink_pa1(3000);
				}
				f_close(&dsk[drive].fil);
				memset(&dsk[drive].fil, 0, sizeof(FIL));
				res = f_open(&dsk[drive].fil, dsk[drive].disk_filename, FA_READ);
			}
		}
		b+=DOUBLE_SIDED_TRACK;
	}
	return res;

}






int __attribute__((optimize("O0")))  main(void) {

	TCHAR full_filename[128];
#ifdef ENABLE_OLED_DISPLAY
	TCHAR mangled_filename[128];
#endif
	uint64_t next_button_debounce;
	int first_time;
	uint32_t	button_state;
	int32_t		file_counter;
	uint32_t	direction_hint;
        DSK dsk[MAX_DRIVES];

	// You have to disable lazy stacking BEFORE initialising the scratch fpu registers
	enable_fpu_and_disable_lazy_stacking();
	init_fpu_regs();

	register uint32_t main_thread_command_reg asm("r10") __attribute__((unused)) = 0;

	rcc_set_frequency(SYSCLK_240_MHZ);

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CCMDATARAMEN, ENABLE);

	/* PD{8..15}  and PD2 for SD card MOSI*/
	config_gpio_data();
	/* PE{0..15} */
	config_gpio_addr();
	
	config_gpio_portc();

	/* PA6/7 */
	config_gpio_dbg();

	config_gpio_buttons();

	config_backup_sram();


	  // switch on compensation cell
	RCC->APB2ENR |= 0 |  RCC_APB2ENR_SYSCFGEN ;
	SYSCFG->CMPCR |= SYSCFG_CMPCR_CMP_PD; // enable compensation cell
	while ((SYSCFG->CMPCR & SYSCFG_CMPCR_READY) == 0);  // wait until ready

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); 
	SysTick->CTRL  = 0;



	SD_NVIC_Configuration();


#ifdef ENABLE_SEMIHOSTING
	initialise_monitor_handles();   /*rtt*/
        printf("Starting\n");
#endif

        memset(&fs32, 0, sizeof(FATFS));
        res = f_mount(&fs32, "",0);
        if (res != FR_OK) {
                blink_pa6_pa7(250);
        }


        TCHAR root_directory[15] = "msx";
        DIR dir;
        static FILINFO fno;

        res = f_opendir(&dir, root_directory);
        if (res != FR_OK) {
                blink_pa6_pa7(100);
        }


#ifdef ENABLE_RTC_RAM_BANK_EMULATION
	config_PC0_int();
#endif
	config_PC2_int();


	first_time=TRUE;
	next_button_debounce=0;
	file_counter=-1;
	direction_hint=FORWARDS;

	// attempt to load the special menu rom. See kcmfs
	res = load_rom("menu.rom",(char *)CCMRAM_BASE,(char *)&high_64k_base);
	if (res == FR_OK) {
		first_time=FALSE;
	}
	// This memset is actually really important
	for (int drive=0;drive<MAX_DRIVES;drive++) {
		memset(&dsk[drive].fil, 0, sizeof(FIL));
	}


#ifdef ENABLE_OLED_DISPLAY
	oled_init();
	oled_display(2,4,"Kernelcrash      ");
	oled_display(2,20,"ROM/FDC emulator");
#endif


	disk_index=0;	// if this is 1 then it means a subdir was previously selected
	while(1) {
		button_state = GPIOA->IDR;
		if (!(button_state & NEXT_ROM_OR_DISK_MASK) || !(button_state & PREV_ROM_OR_DISK_MASK) || first_time) {
			first_time = FALSE;
			if (next_button_debounce == 0 ) {
				if (!(button_state & PREV_ROM_OR_DISK_MASK)) {
					// if we hit the prev button do the worlds worst way of finding the previous file
					// if disk_index_max is set to 1 or more it means we are in a subdir. We should bump the disk_index down. If we let ot go to zero, that's fine, as the next bit of code will go tot eh previous entry in the list
					if (disk_index_max >0) {
						disk_index--;
					}

					if (disk_index==0) {
						if (file_counter >0) {
							res = f_closedir(&dir);
							res = f_opendir(&dir, root_directory);
							for (uint32_t i=0;i< file_counter; i++) {
								res = f_readdir(&dir, &fno);                 
							}
							file_counter--;
						} else {
							res = f_readdir(&dir, &fno);                 
						}
					}
					direction_hint=BACKWARDS;
				} else {
					// if we hit the next button or this is the first time through
					if (disk_index >=1) {
						disk_index++;
					}
					if ((disk_index==0) || (disk_index > disk_index_max)) {
						disk_index = 0;
						disk_index_max = 0;

						file_counter++;
						res = f_readdir(&dir, &fno);                 
						if (res != FR_OK || fno.fname[0] == 0) {
							// allow buttonpushes to 'wrap round'
							f_closedir(&dir);
							res = f_opendir(&dir, root_directory);
							res = f_readdir(&dir, &fno);
							file_counter=0;
						}
					}
					direction_hint=FORWARDS;
				}
				strcpy(full_filename,root_directory);
				strcat(full_filename,"/");
				strcat(full_filename,fno.fname);
#ifdef DEBUG_FILENAME
				strcpy(debug_filename,full_filename);
#endif
				if (load_disks(dsk,full_filename,&disk_index,&disk_index_max,direction_hint)) {
					load_rom("disk.rom",(char *)CCMRAM_BASE,(char *)&high_64k_base);
					// trigger a seek in the next block of code.
					main_thread_data = 0 | 0x40000000 | 0x20000000;
					main_thread_command_reg = MAIN_THREAD_BUTTON_COMMAND;
#ifdef ENABLE_OLED_DISPLAY
					mangle_oled_filename(dsk[0].disk_filename, mangled_filename);
					oled_display(2,4,mangled_filename);
#endif
					// this willl activate the FDC emulation 
					init_fdc();
				} else {
					// TODO. If you have a disk with Disk 1 then Disk 2 and you press the NEXT button, then when it gets to disk 3 load_disks will return 0 and it will attempt to load
					// garbage as a ROM and crash. Of course then if you press NEXT again it will see disk_index is 0 and then advance to the next file.
					disk_index=0;
					disk_index_max=0;
					// Try to close any dsk file that might be open
					for (int drive=0;drive<MAX_DRIVES;drive++) {
						if (dsk[drive].fil.obj.id) {
							f_close(&dsk[drive].fil);
							memset(&dsk[drive].fil, 0, sizeof(FIL));
						}
					}
#ifdef ENABLE_OLED_DISPLAY
					mangle_oled_filename(full_filename, mangled_filename);
					oled_display(2,4,mangled_filename);
#endif
					// Must be a ROM
					deactivate_fdc();
					load_rom(full_filename,(char *)CCMRAM_BASE,(char *)&high_64k_base);
				}
				//next_button_debounce=5850000;
				next_button_debounce=3000000;
			} 
		}
		next_button_debounce = (next_button_debounce>0)?next_button_debounce-1:0;

		if (!(main_thread_command_reg & 0xc0000000) && (main_thread_command_reg & 0xff)) {
			switch(main_thread_command_reg) {
				case (MAIN_THREAD_BUTTON_COMMAND):
				case (MAIN_THREAD_SEEK_COMMAND): {
					main_thread_command_reg |= MAIN_COMMAND_IN_PROGRESS;
					// Check if there is any pending write
					if (fdc_write_flush_count) {
						fdc_write_flush_count = 0;
						save_track(dsk, main_thread_actual_track,(char *) &track_buffer);
						fdc_write_dirty_bits = 0;
					}
					// main_thread_data contains the track number
					// bit 31 is DSK3 (not implemented) , bit 30 is DSK2, bit 29 is DSK1
					load_track(dsk, (main_thread_data>>29), (main_thread_data & 0xff), (char *) &track_buffer);
					delay_ms(1);
					for (int drive = 0 ; drive<MAX_DRIVES ; drive++) {
						if (1<<drive & (main_thread_data>>29)) {
							main_thread_actual_track[drive] = (main_thread_data & 0xff);
						}
					}
					update_fdc_track_from_intended_track_register();
					if ((main_thread_command_reg & 0xff) == MAIN_THREAD_SEEK_COMMAND) {
						main_thread_command_reg |= MAIN_COMMAND_COMPLETE;
					} else {
						// must be a next/prev button push so make sure we don't feed an INTRQ in
						main_thread_command_reg = 0;
					}

					break;
				}
				case (MAIN_THREAD_COMMAND_LOAD_DIRECTORY): {
					main_thread_command_reg |= MAIN_COMMAND_IN_PROGRESS;
					load_directory("msx",(unsigned char *)(CCMRAM_BASE+0x4000));
					unsigned char * p = (unsigned char *) (CCMRAM_BASE+0x4000);
					*p = 0x00;
					main_thread_command_reg |= MAIN_COMMAND_COMPLETE;
					break;
				}
				case (MAIN_THREAD_COMMAND_LOAD_ROM): {
					main_thread_command_reg |= MAIN_COMMAND_IN_PROGRESS;
					// The filename to load will be in CCMRAM+0x4000+0x80
					//
					char *fname = (char *) (CCMRAM_BASE+0x4000+0x80);
					file_counter=0;
					res = f_closedir(&dir);
        				res = f_opendir(&dir, root_directory);
					// Search for the file that the user selected. This is really just a cheap way
					// to have the directory pointer at the right spot, such that hitting NEXT
					// will advance you to the rom/disk the user expects to be next
					while (1) {
						res = f_readdir(&dir, &fno);                 
						if (res != FR_OK || fno.fname[0] == 0) {
							f_closedir(&dir);
							res = f_opendir(&dir, root_directory);
							res = f_readdir(&dir, &fno);
							file_counter=0;
							break;
						}
						if (strcmp(fno.fname,fname)==0) {
							break;
						}
						file_counter++;
					}
					strcpy(full_filename,root_directory);
					strcat(full_filename,"/");
					strcat(full_filename,fno.fname);

					// load_disks returns 0 if if could not find a disk
					disk_index=1;
					direction_hint=FORWARDS;
					if (load_disks(dsk,full_filename,&disk_index,&disk_index_max,direction_hint)) {
						load_rom("disk.rom",(char *)CCMRAM_BASE,(char *)&high_64k_base);
						// trigger a seek in the next block of code.
                                                main_thread_data = 0;
                                                main_thread_command_reg = MAIN_THREAD_SEEK_COMMAND;
#ifdef ENABLE_OLED_DISPLAY
						mangle_oled_filename(dsk[0].disk_filename, mangled_filename);
						oled_display(2,4,mangled_filename);
#endif
						// this willl activate the FDC emulation 
						init_fdc();
					} else {
						disk_index=0;
						// try to close any previous dsk
						for (int drive=0;drive<MAX_DRIVES;drive++) {
							if (dsk[drive].fil.obj.id) {
								f_close(&dsk[drive].fil);
								memset(&dsk[drive].fil, 0, sizeof(FIL));
							}
							dsk[drive].disk_filename[0] = 0;	// null out the first char of the filename (helpful in debugging)
						}
#ifdef ENABLE_OLED_DISPLAY
						mangle_oled_filename(full_filename, mangled_filename);
						oled_display(2,4,mangled_filename);
#endif
						// Must be a ROM
						deactivate_fdc();
						load_rom(full_filename,(char *)CCMRAM_BASE,(char *)&high_64k_base);
					}
					main_thread_command_reg |= MAIN_COMMAND_COMPLETE;
					break;
				}
			}
		}

		if (fdc_write_flush_count) {
			fdc_write_flush_count--;
			if (fdc_write_flush_count == 0) {
				save_track(dsk, main_thread_actual_track, (char *) &track_buffer);
				fdc_write_dirty_bits = 0;
			}
		}
	}
}

