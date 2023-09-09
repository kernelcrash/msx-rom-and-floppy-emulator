#ifndef __MAIN_H
#define __MAIN_H
#include <stdint.h>
#include "stm32f4xx.h"
#include "ff.h"

typedef struct {
        uint32_t        id;
        uint32_t        double_sided;
        TCHAR           disk_filename[128];
        FIL             fil;
} DSK;

typedef uint8_t boolean;
typedef uint8_t byte;


extern void init_fpu_regs(void);
extern void init_fdc(void);
extern void update_fdc_track_from_intended_track_register(void);

extern void deactivate_fdc(void);



#endif
