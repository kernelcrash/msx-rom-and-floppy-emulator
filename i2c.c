// Functions from https://stm32f4-discovery.net/2015/05/library-61-ssd1306-oled-i2c-lcd-for-stm32f4xx/
//
#include "stm32f4xx.h"
#include "i2c.h"


/* Private variables */
static uint32_t TM_I2C_Timeout;
static uint32_t TM_I2C_INT_Clocks[3] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

#define I2C_TRANSMITTER_MODE   0
#define I2C_RECEIVER_MODE      1
#define I2C_ACK_ENABLE         1
#define I2C_ACK_DISABLE        0

/* I2C1 settings, change them in defines.h project file */
#define TM_I2C2_ACKNOWLEDGED_ADDRESS    I2C_AcknowledgedAddress_7bit
#define TM_I2C2_MODE                                    I2C_Mode_I2C
#define TM_I2C2_OWN_ADDRESS                             0x00
#define TM_I2C2_ACK                                             I2C_Ack_Disable
#define TM_I2C2_DUTY_CYCLE                              I2C_DutyCycle_2



int16_t TM_I2C_Start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack) {
        /* Generate I2C start pulse */
        I2Cx->CR1 |= I2C_CR1_START;

        /* Wait till I2C is busy */
        TM_I2C_Timeout = TM_I2C_TIMEOUT;
        while (!(I2Cx->SR1 & I2C_SR1_SB)) {
                if (--TM_I2C_Timeout == 0x00) {
                        return 1;
                }
        }

        /* Enable ack if we select it */
        if (ack) {
                I2Cx->CR1 |= I2C_CR1_ACK;
        }

        /* Send write/read bit */
        if (direction == I2C_TRANSMITTER_MODE) {
                /* Send address with zero last bit */
                I2Cx->DR = address & ~I2C_OAR1_ADD0;

                /* Wait till finished */
                TM_I2C_Timeout = TM_I2C_TIMEOUT;
                while (!(I2Cx->SR1 & I2C_SR1_ADDR)) {
                        if (--TM_I2C_Timeout == 0x00) {
                                return 1;
                        }
                }
        }
        if (direction == I2C_RECEIVER_MODE) {
                /* Send address with 1 last bit */
                I2Cx->DR = address | I2C_OAR1_ADD0;

                /* Wait till finished */
                TM_I2C_Timeout = TM_I2C_TIMEOUT;
                while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
                        if (--TM_I2C_Timeout == 0x00) {
                                return 1;
                        }
                }
        }

        /* Read status register to clear ADDR flag */
        I2Cx->SR2;

        /* Return 0, everything ok */
        return 0;
}


uint8_t TM_I2C_Stop(I2C_TypeDef* I2Cx) {
        /* Wait till transmitter not empty */
        TM_I2C_Timeout = TM_I2C_TIMEOUT;
        while (((!(I2Cx->SR1 & I2C_SR1_TXE)) || (!(I2Cx->SR1 & I2C_SR1_BTF)))) {
                if (--TM_I2C_Timeout == 0x00) {
                        return 1;
                }
        }

        /* Generate stop */
        I2Cx->CR1 |= I2C_CR1_STOP;

        /* Return 0, everything ok */
        return 0;
}


void TM_I2C_WriteData(I2C_TypeDef* I2Cx, uint8_t data) {
        /* Wait till I2C is not busy anymore */
        TM_I2C_Timeout = TM_I2C_TIMEOUT;
        while (!(I2Cx->SR1 & I2C_SR1_TXE) && TM_I2C_Timeout) {
                TM_I2C_Timeout--;
        }

        /* Send I2C data */
        I2Cx->DR = data;
}

void TM_I2C_Write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data) {
        TM_I2C_Start(I2Cx, address, I2C_TRANSMITTER_MODE, I2C_ACK_DISABLE);
        TM_I2C_WriteData(I2Cx, reg);
        TM_I2C_WriteData(I2Cx, data);
        TM_I2C_Stop(I2Cx);
}




uint8_t TM_I2C_IsDeviceConnected(I2C_TypeDef* I2Cx, uint8_t address) {
        uint8_t connected = 0;
        /* Try to start, function will return 0 in case device will send ACK */
        if (!TM_I2C_Start(I2Cx, address, I2C_TRANSMITTER_MODE, I2C_ACK_ENABLE)) {
                connected = 1;
        }

        /* STOP I2C */
        TM_I2C_Stop(I2Cx);

        /* Return status */
        return connected;
}


void TM_I2C_Init(I2C_TypeDef* I2Cx, uint32_t clockSpeed) {
        I2C_InitTypeDef I2C_InitStruct;
	GPIO_InitTypeDef GPIO_InitStruct;

        if (I2Cx == I2C2) {
	
		// enable APB1 peripheral clock for I2C1
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
		// enable clock for SCL and SDA pins
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
		/* setup SCL and SDA pins
		 * You can connect I2C1 to two different
		 * pairs of pins:
		 * 1. SCL on PB10
		 * 2. SCA on PB11
		 */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; // we are going to use PB6 and PB7
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;			// set pins to alternate function
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;		// set GPIO speed
		GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;			// set output to open drain --> the line has to be only pulled low, not driven high
		GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;			// enable pull up resistors
		GPIO_Init(GPIOB, &GPIO_InitStruct);					// init GPIOB
	
		// Connect I2C1 pins to AF  
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);	// SCL
		GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2); // SDA

                /* Check clock, set the lowest clock your devices support on the same I2C but */
                if (clockSpeed < TM_I2C_INT_Clocks[0]) {
                        TM_I2C_INT_Clocks[0] = clockSpeed;
                }

                /* Set values */
                I2C_InitStruct.I2C_ClockSpeed = TM_I2C_INT_Clocks[0];
                I2C_InitStruct.I2C_AcknowledgedAddress = TM_I2C2_ACKNOWLEDGED_ADDRESS;
                I2C_InitStruct.I2C_Mode = TM_I2C2_MODE;
                I2C_InitStruct.I2C_OwnAddress1 = TM_I2C2_OWN_ADDRESS;
                I2C_InitStruct.I2C_Ack = TM_I2C2_ACK;
                I2C_InitStruct.I2C_DutyCycle = TM_I2C2_DUTY_CYCLE;
	}
        /* Disable I2C first */
        I2Cx->CR1 &= ~I2C_CR1_PE;

        /* Initialize I2C */
        I2C_Init(I2Cx, &I2C_InitStruct);

        /* Enable I2C */
        I2Cx->CR1 |= I2C_CR1_PE;
}

void TM_I2C_WriteMulti(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t* data, uint16_t count) {
        TM_I2C_Start(I2Cx, address, I2C_TRANSMITTER_MODE, I2C_ACK_DISABLE);
        TM_I2C_WriteData(I2Cx, reg);
        while (count--) {
                TM_I2C_WriteData(I2Cx, *data++);
        }
        TM_I2C_Stop(I2Cx);
}

