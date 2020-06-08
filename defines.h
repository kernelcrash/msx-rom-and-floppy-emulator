// defines
//
// This needs to be able to be included from C and asm. defines work in .S and .c files

// Stringify functions
#define XSTR(x) STR(x)
#define STR(x) #x

// true/false
#define TRUE 1
#define FALSE 0

// registers
#define reg_zero	s0
#define reg_bit0_high	s1
#define reg_bit1_high	s2
#define reg_gpioc_base	s3
#define reg_bit2_high	s4

#define reg_gpioa_base	s5
#define reg_high_64k_base	s6
#define reg_exti_base	s7
#define reg_moder_dataout	s8
#define reg_moder_datain	s9
// reg_mapper_pages
//   b31/b30 - rom type
//   b15-b12 - page 1 bank (8K)
//   b11-b8  - page 0 bank (8K)
//   b7 -b4  - page 3 bank (8k)
//   b3 -b0  - page 2 bank (8k)
#define reg_mapper_pages	s10
#define reg_rowcache	s11
#define reg_gpiob_base	s12
#define reg_bit6_high	s13
#define reg_ccmram_log	s14

#define reg_fdc_status	s15
// NOTE: FPU registers d8 and d9 are used somehow, so you cannot use s16,s17,s18 and s19


#define reg_fdc_write_length	s20
#define reg_fdc_system	s21
#define reg_bit8_high	s22
#define reg_track_buffer_ptr	s23
#define reg_fdc_track	s24
#define reg_fdc_actual_track	s25
#define reg_fdc_sector	s26
#define reg_fdc_data	s27

#define reg_fdc_irq_drq	s28
#define reg_fdc_command	s29

#define reg_fake_stack	s30

#define reg_fdc_read_length	s31

// the thread command is a special one. Its hard to use a floating reg from C when we disable the FPU
#define reg_main_thread_command	r10

// ============================
//
#define         IORQ_MASK             0x0001       // ie PC0
#define         SLOT_SELECT_MASK      0x0002       // ie PC1
#define         MREQ_MASK             0x0004       // ie PC2
#define         RD_MASK               0x0008       // ie PC3
#define         BUSDIR_HIGH           0x0010       // ie PC4

// =============================
#define		NEXT_ROM_OR_DISK_MASK 0x0002	   // PA1
#define		PREV_ROM_OR_DISK_MASK 0x0004	   // PA2
// =============================

#define		MREQ_PREEMPTION_PRIORITY	0
#define		IORQ_PREEMPTION_PRIORITY	1
#define		SDIO_IRQ_PREEMPTION_PRIORITY	3
#define		SDIO_DMA_PREEMPTION_PRIORITY	4


// ============================
// straight from fMSX
//
                           /* Common status bits:               */
#define F_BUSY     0x01    /* Controller is executing a command */
#define F_READONLY 0x40    /* The disk is write-protected       */
#define F_NOTREADY 0x80    /* The drive is not ready            */

                           /* Type-1 command status:            */
#define F_INDEX    0x02    /* Index mark detected               */
#define F_TRACK0   0x04    /* Head positioned at track #0       */
#define F_CRCERR   0x08    /* CRC error in ID field             */
#define F_SEEKERR  0x10    /* Seek error, track not verified    */
#define F_HEADLOAD 0x20    /* Head loaded                       */

                           /* Type-2 and Type-3 command status: */
#define F_DRQ      0x02    /* Data request pending              */
#define F_LOSTDATA 0x04    /* Data has been lost (missed DRQ)   */
#define F_ERRCODE  0x18    /* Error code bits:                  */
#define F_BADDATA  0x08    /* 1 = bad data CRC                  */
#define F_NOTFOUND 0x10    /* 2 = sector not found              */
#define F_BADID    0x18    /* 3 = bad ID field CRC              */
#define F_DELETED  0x20    /* Deleted data mark (when reading)  */
#define F_WRFAULT  0x20    /* Write fault (when writing)        */

#define C_DELMARK  0x01
#define C_SIDECOMP 0x02
#define C_STEPRATE 0x03
#define C_VERIFY   0x04
#define C_WAIT15MS 0x04
#define C_LOADHEAD 0x08
#define C_SIDE     0x08
#define C_IRQ      0x08
#define C_SETTRACK 0x10
#define C_MULTIREC 0x10

#define S_DRIVE    0x03
#define S_RESET    0x04
#define S_HALT     0x08
#define S_SIDE     0x10
#define S_DENSITY  0x20

#define S_LASTSTEPDIR	0x80
#define S_FDC_PRESENT	0x80000000

#define WD1793_IRQ     0x80
#define WD1793_DRQ     0x40



// -------------
#define MAIN_THREAD_SEEK_COMMAND 1
#define MAIN_THREAD_CHANGE_DISK_COMMAND 2

// ---------------
#define ROM_TYPE_GENERIC	0
#define ROM_TYPE_KONAMI4	1
#define ROM_TYPE_ASCII8		2
#define ROM_TYPE_ASCII16	3

#define ROM_SUFFIX_KONAMI4	".konami4"
#define ROM_SUFFIX_ASCII8	".ascii8"
#define ROM_SUFFIX_ASCII16	".ascii16"
#define DSK_SUFFIX	".dsk"

// ------------
#define SINGLE_SIDED_DISK_SIZE 368640
#define DOUBLE_SIDED_DISK_SIZE 737280

//---------
#define CCMRAM_BASE	0x10000000

//------------
#define FDC_WRITE_FLUSH_DEFAULT	10000

//----------
#define MAIN_COMMAND_IN_PROGRESS	0x40000000
#define MAIN_COMMAND_COMPLETE		0x80000000
