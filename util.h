#ifndef __UTIL_H
#define __UTIL_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"

#include "ff.h"

#include "defines.h"

void delay_us(const uint16_t us);
void delay_ms(const uint16_t ms);
void blink_pa6(int delay);
void blink_pa7(int delay);
void blink_pa6_pa7(int delay);

uint32_t suffix_match(char *name, char *suffix);
uint32_t get_romtype_from_filename(char *fname);
uint32_t get_pagemap_default_by_romtype(uint32_t romtype);
void load_rom(char *fname,char *lowbuffer, char* highbuffer);

#endif
