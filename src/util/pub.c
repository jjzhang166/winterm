#include "pub.h"

void print_binary(char * tip, uint8 * buff, uint8 bits) {
	uint8 bit_index = 0;

	for (bit_index = 0; bit_index < bits; bit_index++) {
		printf("%d", (buff[bit_index >> 3] >> (7 - (bit_index % 8))) & 0x01);

		if ((bit_index + 1) % 4 == 0) {
			printf(" ");
		}

		if ((bit_index + 1) % 64 == 0) {
			printf("\r\n");
		}
	}
}

void print_hex(char * tip, uint8 * buff, uint8 bytes) {
	uint8 i = 0;
	for (i = 0; i < bytes; i++) {
		printf("0x%02X ", buff[i]);
	}
}
