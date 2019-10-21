#include <stdio.h>
#include <stdarg.h>

#include "i2cLcd.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

uint8_t SLAVE_ADDR;
uint8_t DISPLAY_CRTL;
uint8_t DISPLAY_FCT;
uint8_t DISPLAY_MODE;

static esp_err_t i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SLAVE_ADDR << 1) | I2C_MASTER_WRITE, 0x1);
    i2c_master_write(cmd, data_wr, size, 0x1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void expander_write(uint8_t data)
{
    data = data | LCD_BACKLIGHT;
    i2c_master_write_slave(I2C_NUMBER(1), &data, 1);
}

void pulse_enable(uint8_t data)
{
    expander_write(data | En);
    ets_delay_us(10);
    expander_write(data & ~En);
    ets_delay_us(50);
}

void write4bits(uint8_t value)
{
	expander_write(value);
	pulse_enable(value);
}

void send(uint8_t value, uint8_t mode)
{
    uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
	write4bits((highnib)|mode);
	write4bits((lownib)|mode);
}

void command(uint8_t value)
{
    send(value, 0);
}

void display()
{
    DISPLAY_CRTL |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | DISPLAY_CRTL);
}


void home()
{
    command(LCD_RETURNHOME);// clear display, set cursor position to zero
	ets_delay_us(4000);  // this command takes a long time!
}

void lcd_clear()
{
    command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	ets_delay_us(4000);  // this command takes a long time!
}

void lcd_write_chr(char c)
{
    send(c, Rs);
}

void lcd_write_str(char* s)
{
    // dope way to check for null terminator David :D
    for (int i = 0; s[i]; ++i)
    {
        lcd_write_chr(s[i]);
    }
}

void lcd_set_cursor(uint8_t x, uint8_t y)
{
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if(y > 1) // only using 2 line display
    {
        y = y -1;
    }
    command(LCD_SETDDRAMADDR | (x + row_offsets[y]));
}

void lcd_off()
{
    DISPLAY_CRTL &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | DISPLAY_CRTL);
}

void lcd_on()
{
    DISPLAY_CRTL |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | DISPLAY_CRTL);
}

void lcd_printf(const char* str, ...)
{
    char buffer[124];
    va_list args;
    va_start (args, str);
    vsnprintf (buffer,124, str, args);
    lcd_write_str(buffer);
    va_end (args);
}

void lcd_start_printf(const char* str, ...)
{
    char buffer[124];
    lcd_clear();
    home();
    
    va_list args;
    va_start (args, str);
    vsnprintf (buffer,124, str, args);
    lcd_write_str(buffer);
    va_end (args);
}

void lcd_init(uint8_t i2c_addr, uint8_t gpio_scl, uint8_t gpio_sda)
{
    char x;

    SLAVE_ADDR = i2c_addr;
    DISPLAY_FCT = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;

    int i2c_master_port = I2C_NUMBER(1);
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = gpio_sda;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = gpio_scl;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);

    ets_delay_us(55000);
    expander_write(LCD_BACKLIGHT);
    ets_delay_us(2000000);

    write4bits(0x03 << 4);
    ets_delay_us(5000);

    write4bits(0x03 << 4);
    ets_delay_us(5000);

    write4bits(0x03 << 4);
    ets_delay_us(5000);

    write4bits(0x02 << 4);

    command(LCD_FUNCTIONSET | DISPLAY_FCT);
    DISPLAY_CRTL = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    display();

    lcd_clear();

    DISPLAY_MODE = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    command(LCD_ENTRYMODESET | DISPLAY_MODE);

    home();
}