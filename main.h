#ifndef __MAIN_H
#define __MAIN_H
#include <stdint.h>
#include "stm32f4xx.h"

typedef uint8_t boolean;
typedef uint8_t byte;


extern void init_fpu_regs(void);
extern void init_fdc(void);
extern void deactivate_fdc(void);


#endif
