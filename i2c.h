#ifndef __I2C_H
#define __I2C_H

#define TM_I2C_TIMEOUT                                  20000

int16_t TM_I2C_Start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack);
uint8_t TM_I2C_Stop(I2C_TypeDef* I2Cx);
void TM_I2C_WriteData(I2C_TypeDef* I2Cx, uint8_t data);
void TM_I2C_Write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data);
void I2C_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction);
uint8_t TM_I2C_IsDeviceConnected(I2C_TypeDef* I2Cx, uint8_t address);
void TM_I2C_Init(I2C_TypeDef* I2Cx, uint32_t clockSpeed);
void TM_I2C_WriteMulti(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t* data, uint16_t count);


#endif
