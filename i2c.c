#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <string.h>

#include "i2c.h"

void i2c_init(void) {
    TWSR = 0x00;
    TWBR = 0x48; 

    TWCR = (1 << TWEN);/*set this to 1 to mske pins into i2c interface*/
}

void i2c_meaningful_status(uint8_t status) {
	switch (status) {
		case 0x08: // START transmitted, proceed to load SLA+W/R
			printf_P(PSTR("START\n"));
			break;
		case 0x10: // repeated START transmitted, proceed to load SLA+W/R
			printf_P(PSTR("RESTART\n"));
			break;
		case 0x38: // NAK or DATA ARBITRATION LOST
			printf_P(PSTR("NOARB/NAK\n"));
			break;
		// MASTER TRANSMIT
		case 0x18: // SLA+W transmitted, ACK received
			printf_P(PSTR("MT SLA+W, ACK\n"));
			break;
		case 0x20: // SLA+W transmitted, NAK received
			printf_P(PSTR("MT SLA+W, NAK\n"));
				break;
		case 0x28: // DATA transmitted, ACK received
			printf_P(PSTR("MT DATA+W, ACK\n"));
			break;
		case 0x30: // DATA transmitted, NAK received
			printf_P(PSTR("MT DATA+W, NAK\n"));
			break;
		// MASTER RECEIVE
		case 0x40: // SLA+R recieved, ACK received
			printf_P(PSTR("MR SLA+R, ACK\n"));
			break;
		case 0x48: // SLA+R recieved, NAK received
			printf_P(PSTR("MR SLA+R, NAK\n"));
			break;
		case 0x50: // DATA received, ACK sent
			printf_P(PSTR("MR DATA+R, ACK\n"));
			break;
		case 0x58: // DATA received, NAK sent
			printf_P(PSTR("MR DATA+R, NAK\n"));
			break;
		default:
			printf_P(PSTR("N/A %02X\n"), status);
			break;
	}
}

inline void i2c_start() {
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);/*makes shit happen. TWIE for interrupt, TWEN for enable, TWSTO stop cond, TWSTA start cond, TWEA enable ack, TWINT interrupt flag.*/ 

	while(!(TWCR & (1<<TWINT)));
}

inline void i2c_stop() {
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);/*SDA low, SCL high, then SDA high*/
	
}

inline uint8_t i2c_get_status(void) {
	uint8_t status = 0x00;
	status = TWSR & 0xF8;
	return status;
}

inline void i2c_xmit_addr(uint8_t address, uint8_t rw) { /*sending byte*/
	TWDR = address | rw;/*adress is bit 1-7, bit 0 is r/w. to TWDR you just write the value of the byte you want to send*/
	TWCR = (1<<TWINT)|(1<<TWEN);

	while(!(TWCR & (1<<TWINT)));
	
}

inline void i2c_xmit_byte(uint8_t data) {
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
}

inline uint8_t i2c_read_ACK() {
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);/*TWEA is the acknoledgement enable bit*/
	while(!(TWCR & (1<<TWINT)));
	return TWDR;/*here we recieve a byte*/
}

inline uint8_t i2c_read_NAK() {
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	return TWDR;
}

inline void eeprom_wait_until_write_complete() {/*if slave does not send ack start again*/
	while(i2c_get_status() != 0x18){

	i2c_start();

	TWDR = (0xA0 | 0);
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

	}
}

uint8_t eeprom_read_byte(uint8_t addr) {/*8.2 in the sheet*/
	uint8_t byte_read = 0;
	i2c_start();

	i2c_xmit_addr(0xA0, 0);

	i2c_xmit_byte(addr);

	i2c_start();

	i2c_xmit_addr(0xA0, 1);

	byte_read = i2c_read_NAK();/*sending a nack to tell slave im done recieving*/

	i2c_stop();

	return byte_read;
}

void eeprom_write_byte(uint8_t addr, uint8_t data) {/*6.1 in the sheet*/

	i2c_start();

	i2c_xmit_addr(0xA0, 0);
	
	i2c_xmit_byte(addr);

	i2c_xmit_byte(data);

	i2c_stop();

	eeprom_wait_until_write_complete();/*make sure the write is complete*/
}



void eeprom_write_page(uint8_t addr, uint8_t *data) {
	
	while(addr % 8 != 0){ 
		addr++;
	}
		
	int i = 0;

	i2c_start();

	i2c_xmit_addr(0xA0, 0);

	
	i2c_xmit_byte(addr);

	for(i = 0; i<8; i++){ 	/*writing only 8 bits*/
	i2c_xmit_byte(data[i]);
	}

	i2c_stop();

	eeprom_wait_until_write_complete();
}

void eeprom_sequential_read(uint8_t *buf, uint8_t start_addr, uint8_t len) {
	
	
	while(start_addr % 8 != 0){
		start_addr++;
	}
	
	int i;

	i2c_start();

	i2c_xmit_addr(0xA0, 0);
	
	i2c_xmit_byte(start_addr);

	i2c_start();

	i2c_xmit_addr(0xA0, 1);

	for(i = 0; i < (len-1); i++){
	buf[i] = i2c_read_ACK();/*sending ack so slave knows i what to read more*/
	}

	buf[len-1] = i2c_read_NAK();

	i2c_stop();

}
