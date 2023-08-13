#ifndef __OLED_H
#define __OLED_H

#define SSD1306_I2C_ADDR         0x78

#define SSD1306_WRITECOMMAND(command)      TM_I2C_Write(I2C2, SSD1306_I2C_ADDR, 0x00, (command))
/* Write data */
#define SSD1306_WRITEDATA(data)            TM_I2C_Write(I2C2, SSD1306_I2C_ADDR, 0x40, (data))

/* SSD1306 data buffer */
#define SSD1306_WIDTH            128
#define SSD1306_HEIGHT           64

typedef enum {
        SSD1306_COLOR_BLACK = 0x00, /*!< Black color, no pixel */
        SSD1306_COLOR_WHITE = 0x01  /*!< Pixel is set. Color depends on LCD */
} SSD1306_COLOR_t;


typedef struct {
        uint16_t CurrentX;
        uint16_t CurrentY;
        uint8_t Inverted;
        uint8_t Initialized;
} SSD1306_t;


typedef struct {
        uint8_t FontWidth;    /*!< Font width in pixels */
        uint8_t FontHeight;   /*!< Font height in pixels */
        const uint16_t *data; /*!< Pointer to data font data array */
} TM_FontDef_t;


void TM_SSD1306_UpdateScreen(void);
void TM_SSD1306_Fill(SSD1306_COLOR_t color);
void TM_SSD1306_DrawPixel(uint16_t x, uint16_t y, SSD1306_COLOR_t color);
void TM_SSD1306_GotoXY(uint16_t x, uint16_t y);
char TM_SSD1306_Putc(char ch, TM_FontDef_t* Font, SSD1306_COLOR_t color);
char TM_SSD1306_Puts(char* str, TM_FontDef_t* Font, SSD1306_COLOR_t color);
void oled_init();
void oled_display(int x, int y, char *text) ;



#endif
