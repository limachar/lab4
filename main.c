#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

#include "adc.h"
#include "gpio.h"
#include "i2c.h"
#include "serial.h"
#include "timer.h"
uint8_t address = 0x30;
uint8_t rw = 0;
uint8_t data = 'L';
char name[] = "Linn";
uint8_t i = 0;
char buffer[10];



void main (void) {

	i2c_init();
	uart_init();

	sei();
	/*for(i = 0; i<sizeof(name); i++){
	eeprom_write_byte((address+i),name[i]);
	}
	uint8_t bytees = eeprom_read_byte(0x21);
	*/

	

	eeprom_write_page(0x30, name);
	
	
	
	while (1) {
		eeprom_sequential_read(buffer, 0x30, sizeof(name));
		printf_P(PSTR("%s\n"), buffer);
        
	}
}

