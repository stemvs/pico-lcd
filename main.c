
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <stdlib.h>
#include <stdio.h>
#include "lcd1602.h"
#include "mcp2515.h"

const char ok[] = {'o','k'};
const char nok[] = {'n','o','t',' ','o','k'};
const char rec[] = {'r','e','c',':'};
const char ex[] = {' ','e','x'};
const char rtr[] = {' ','r','t','r'};

lcd1602* lll;

int sendlcd(uint8_t value) {
	lcd_send(lll, value, Rs);
	return 1;
}

size_t write(char *buffer, size_t size)
{
  size_t n = 0;
  while (size--) {
    if (sendlcd(*buffer++)) n++;
    else break;
  }
  return n;
}

size_t printC(char c)
{
  return write(&c, 1);
}

size_t printNumber(unsigned long n, uint8_t base)
{
	char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
	char *str = &buf[sizeof(buf) - 1];
	size_t size = 0;

	// prevent crash if called with base == 1
	if (base < 2) base = 10;

	do {
		char c = n % base;
		n /= base;

		*--str = c < 10 ? c + '0' : c + 'A' - 10;
		size++;
	} while(n);

	return write(str, size);
}

size_t printI(long n, int base) {
	if (base == 0) {
		return 0;
	} else if (base == 10) {
		if (n < 0) {
			int t = printC((char)'-');
			n = -n;
			return printNumber(n, 10) + t;
		}
		return printNumber(n, 10);
	} else {
		return printNumber(n, base);
	}
}

int main() {
	mcp2515* controller = malloc(sizeof(mcp2515));
	lcd1602* lcd = malloc(sizeof(lcd1602));
	uint8_t val;

	lcd->dispFunc = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
	lcd->dispCtrl = LCD_DISPLAYON | LCD_BLINKOFF | LCD_CURSOROFF;
	lcd->dispMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	lcd->bckLight = LCD_BACKLIGHT;

	gpio_init(CS_PIN);
	gpio_set_dir(CS_PIN, GPIO_OUT);
	gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);
	gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
	gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

	gpio_put(CS_PIN, 1);

	spi_init(SPI_INSTANCE, 400 * 1000);
	i2c_init(I2C_HWBLOCK, 400 * 1000);

	spi_set_format(spi1, 8, 1, 1, SPI_MSB_FIRST);

	val = mcp2515_begin(controller, 500E3);

	sleep_ms(50);

	lcd_init(lcd);

	lll = lcd;

	if(val == 0x1) {
		write((char*)ok, sizeof(ok));
	} else {
		write((char*)nok, sizeof(nok));
		return 1;
	}

	sleep_ms(500);

	while(1) {
		int packetSize = mcp2515_parsePacket(controller);
		char hex[20];

		if(packetSize) {
			lcd_clear(lcd);
			write((char*)rec, sizeof(rec));

			if(controller->_rxExtended) {
				write((char*)ex, sizeof(ex));
			}

			if(controller->_rxRtr) {
				write((char*)rtr, sizeof(rtr));
			}

			lcd_setCursor(lcd, 0, 1);

			write(hex, sprintf(hex, "%x", controller->_rxId) - 1);

			break;
		}
	}

	return 0;
}
