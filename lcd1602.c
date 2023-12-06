#include "lcd1602.h"

void lcd_expanderWrite(lcd1602* lcd, uint8_t data) {
	data |= lcd->bckLight;
	i2c_write_blocking(I2C_HWBLOCK, 0x27, &data, 1, 0);
}

void lcd_write4bits(lcd1602* lcd, uint8_t data) {
	lcd_expanderWrite(lcd, data);
	lcd_expanderWrite(lcd, data | En);
	sleep_us(1);
	lcd_expanderWrite(lcd, data & ~En);
	sleep_us(50);
}

void lcd_send(lcd1602* lcd, uint8_t data, uint8_t mode) {
	uint8_t high = data & 0xf0;
	uint8_t low = (data << 4) & 0xf0;

	lcd_write4bits(lcd, high | mode);
	lcd_write4bits(lcd, low | mode);
}

void lcd_command(lcd1602* lcd, uint8_t data) {
	lcd_send(lcd, data, 0);
}

void lcd_clear(lcd1602* lcd) {
	lcd_command(lcd, LCD_CLEARDISPLAY);
	sleep_us(2000);
}

void lcd_home(lcd1602* lcd) {
	lcd_command(lcd, LCD_RETURNHOME);
	sleep_us(2000);
}

void lcd_setCursor(lcd1602* lcd, uint8_t col, uint8_t row) {
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	lcd_command(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_init(lcd1602* lcd) {
	// set 4 bit mode
	lcd_write4bits(lcd, 0x03 << 4);
	sleep_us(4500);
	lcd_write4bits(lcd, 0x03 << 4);
	sleep_us(4500);
	lcd_write4bits(lcd, 0x03 << 4);
	sleep_us(150);
	lcd_write4bits(lcd, 0x02 << 4);

	lcd_command(lcd, LCD_FUNCTIONSET | lcd->dispFunc);
	lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->dispCtrl);
	lcd_command(lcd, LCD_ENTRYMODESET | lcd->dispMode);

	lcd_clear(lcd);
	lcd_home(lcd);
}