#include "mcp2515.h"

uint8_t mcp2515_readRegister(uint8_t address) {
	uint8_t value;
	uint8_t arr[2];

	arr[0] = 0x03;
	arr[1] = address;

	gpio_put(CS_PIN, 0);

	spi_write_blocking(spi1, arr, 2);
	spi_read_blocking(spi1, 0, &value, 1);

	gpio_put(CS_PIN, 1);

	return value;
}

uint8_t mcp2515_modifyRegister(uint8_t address, uint8_t mask, uint8_t value) {
	uint8_t data[4];

	data[0] = 0x05;
	data[1] = address;
	data[2] = mask;
	data[3] = value;

	gpio_put(CS_PIN, 0);

	spi_write_blocking(SPI_INSTANCE, data, 4);

	gpio_put(CS_PIN, 1);
}

void mcp2515_writeRegister(uint8_t address, uint8_t value) {
	uint8_t data[3];

	data[0] = 0x02;
	data[1] = address;
	data[2] = value;

	gpio_put(CS_PIN, 0);
	
	spi_write_blocking(SPI_INSTANCE, data, 3);

	gpio_put(CS_PIN, 1);
}

void mcp2515_reset() {
	uint8_t data = 0xc0;

	gpio_put(CS_PIN, 0);

	spi_write_blocking(SPI_INSTANCE, &data, 1);

	gpio_put(CS_PIN, 1);

	sleep_us(10);
}

int mcp2515_begin(mcp2515* controller, long baudRate) {
	mcp2515_reset();

	mcp2515_writeRegister(REG_CANCTRL, 0x80);
	if(mcp2515_readRegister(REG_CANCTRL) != 0x80) {
		return 0;
	}

	controller->_clockFrequency = 8E6;

	const struct {
		long clockFrequency;
		long baudRate;
		uint8_t cnf[3];
	} CNF_MAPPER[] = {
		{  (long)8E6, (long)1000E3, { 0x00, 0x80, 0x00 } },
		{  (long)8E6,  (long)500E3, { 0x00, 0x90, 0x02 } },
		{  (long)8E6,  (long)250E3, { 0x00, 0xb1, 0x05 } },
		{  (long)8E6,  (long)200E3, { 0x00, 0xb4, 0x06 } },
		{  (long)8E6,  (long)125E3, { 0x01, 0xb1, 0x05 } },
		{  (long)8E6,  (long)100E3, { 0x01, 0xb4, 0x06 } },
		{  (long)8E6,   (long)80E3, { 0x01, 0xbf, 0x07 } },
		{  (long)8E6,   (long)50E3, { 0x03, 0xb4, 0x06 } },
		{  (long)8E6,   (long)40E3, { 0x03, 0xbf, 0x07 } },
		{  (long)8E6,   (long)20E3, { 0x07, 0xbf, 0x07 } },
		{  (long)8E6,   (long)10E3, { 0x0f, 0xbf, 0x07 } },
		{  (long)8E6,    (long)5E3, { 0x1f, 0xbf, 0x07 } },

		{ (long)16E6, (long)1000E3, { 0x00, 0xd0, 0x82 } },
		{ (long)16E6,  (long)500E3, { 0x00, 0xf0, 0x86 } },
		{ (long)16E6,  (long)250E3, { 0x41, 0xf1, 0x85 } },
		{ (long)16E6,  (long)200E3, { 0x01, 0xfa, 0x87 } },
		{ (long)16E6,  (long)125E3, { 0x03, 0xf0, 0x86 } },
		{ (long)16E6,  (long)100E3, { 0x03, 0xfa, 0x87 } },
		{ (long)16E6,   (long)80E3, { 0x03, 0xff, 0x87 } },
		{ (long)16E6,   (long)50E3, { 0x07, 0xfa, 0x87 } },
		{ (long)16E6,   (long)40E3, { 0x07, 0xff, 0x87 } },
		{ (long)16E6,   (long)20E3, { 0x0f, 0xff, 0x87 } },
		{ (long)16E6,   (long)10E3, { 0x1f, 0xff, 0x87 } },
		{ (long)16E6,    (long)5E3, { 0x3f, 0xff, 0x87 } },
	};

	const uint8_t* cnf = NULL;

	for (unsigned int i = 0; i < (sizeof(CNF_MAPPER) / sizeof(CNF_MAPPER[0])); i++) {
		if (CNF_MAPPER[i].clockFrequency == controller->_clockFrequency && CNF_MAPPER[i].baudRate == baudRate) {
			cnf = CNF_MAPPER[i].cnf;
			break;
		}
	}

	if (cnf == NULL) {
		return 0;
	}

	mcp2515_writeRegister(REG_CNF1, cnf[0]);
	mcp2515_writeRegister(REG_CNF2, cnf[1]);
	mcp2515_writeRegister(REG_CNF3, cnf[2]);

	mcp2515_writeRegister(REG_CANINTE, FLAG_RXnIE(1) | FLAG_RXnIE(0));
	mcp2515_writeRegister(REG_BFPCTRL, 0x00);
	mcp2515_writeRegister(REG_TXRTSCTRL, 0x00);
	mcp2515_writeRegister(REG_RXBnCTRL(0), FLAG_RXM1 | FLAG_RXM0);
	mcp2515_writeRegister(REG_RXBnCTRL(1), FLAG_RXM1 | FLAG_RXM0);

	mcp2515_writeRegister(REG_CANCTRL, 0x00);
	if (mcp2515_readRegister(REG_CANCTRL) != 0x00) {
		return 0;
	}

	return 1;
}

int mcp2515_parsePacket(mcp2515* controller)
{
	int n;

	uint8_t intf = mcp2515_readRegister(REG_CANINTF);

	if (intf & FLAG_RXnIF(0)) {
		n = 0;
	} else if (intf & FLAG_RXnIF(1)) {
		n = 1;
	} else {
		controller->_rxId = -1;
		controller->_rxExtended = false;
		controller->_rxRtr = false;
		controller->_rxLength = 0;
		return 0;
	}

	controller->_rxExtended = (mcp2515_readRegister(REG_RXBnSIDL(n)) & FLAG_IDE) ? true : false;

	uint32_t idA = ((mcp2515_readRegister(REG_RXBnSIDH(n)) << 3) & 0x07f8) | ((mcp2515_readRegister(REG_RXBnSIDL(n)) >> 5) & 0x07);
	if (controller->_rxExtended) {
		uint32_t idB = (((uint32_t)(mcp2515_readRegister(REG_RXBnSIDL(n)) & 0x03) << 16) & 0x30000) | ((mcp2515_readRegister(REG_RXBnEID8(n)) << 8) & 0xff00) | mcp2515_readRegister(REG_RXBnEID0(n));

		controller->_rxId = (idA << 18) | idB;
		controller->_rxRtr = (mcp2515_readRegister(REG_RXBnDLC(n)) & FLAG_RTR) ? true : false;
	} else {
		controller->_rxId = idA;
		controller->_rxRtr = (mcp2515_readRegister(REG_RXBnSIDL(n)) & FLAG_SRR) ? true : false;
	}
	controller->_rxDlc = mcp2515_readRegister(REG_RXBnDLC(n)) & 0x0f;
	controller->_rxIndex = 0;

	if (controller->_rxRtr) {
		controller->_rxLength = 0;
	} else {
		controller->_rxLength = controller->_rxDlc;

		for (int i = 0; i < controller->_rxLength; i++) {
			controller->_rxData[i] = mcp2515_readRegister(REG_RXBnD0(n) + i);
		}
	}

	mcp2515_modifyRegister(REG_CANINTF, FLAG_RXnIF(n), 0x00);

	return controller->_rxDlc;
}