/* None of these handlers should fire. This is mainly here for debugging */

#include "stm32f4xx.h"
#include "stm32f4_discovery_sdio_sd.h"


void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);


uint32_t magic;

void NMI_Handler(void)
{
	magic = 0x12345678;
	while(1);
}

void HardFault_Handler(void)
{
	magic = 0x56785678;
	while(1);
}

void MemManage_Handler(void)
{
	magic = 0x11223344;
	while(1);
}

void BusFault_Handler(void)
{
	magic = 0x66556655;
	while(1);
}

void UsageFault_Handler(void)
{
	magic = 0x99aa99aa;
	while(1);
}

void SVC_Handler(void)
{
	magic = 0x11cc11cc;
	while(1);
}

void DebugMon_Handler(void)
{
	magic = 0xffeeffee;
	while(1);
}

void PendSV_Handler(void)
{
	magic = 0x11881188;
	while(1);
}

void SysTick_Handler(void)
{
	magic = 0x77557755;
	while(1);
}

