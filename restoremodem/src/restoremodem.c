#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "at91sam9260.h"
#include "at91_pio.h"

#define BASE (AT91_BASE_SYS & 0xfffff000) //Page alignment
#define PAGEALIGN (AT91_BASE_SYS - BASE)
#define MAPLEN 0x2000

#define MODEM_PINS_B ((0x3 << 6) | (0xf << 10) | (0x3f << 16) | (0x3 << 28))

void print_usage ( char *prgname ) {

	printf("Usage: %s [delay]\n\n  delay is decimal seconds\n\n", prgname);

}

int main(int argc, char *argv[]) {
	int handler;
	volatile unsigned char *baseptr;
	volatile unsigned char *PIOBbaseptr;
	unsigned long pusr_b, psr_b;
	struct timespec delay;
	double cmddelay;

	delay.tv_sec = 0;
	delay.tv_nsec = 1e7;

	if (argc == 2) {
		errno = 0;
		cmddelay = strtod(argv[1], NULL);
		if (errno) {
			printf("Unable to parse delay\n");
			print_usage(argv[0]);
			return -1;
		} else {
			delay.tv_sec = (time_t) cmddelay;
			delay.tv_nsec = (long) ((cmddelay - delay.tv_sec) * 1e9);
		}
	} else if (argc > 2) {
		printf("Too many arguments!\n");
		print_usage(argv[0]);
		return -1;
	}

	handler = open("/dev/mem", O_RDWR);
	if (handler < 0) {
		printf("Unable to open /dev/mem\n");
		return -1;
	}

	baseptr = (unsigned char *)mmap(NULL,  MAPLEN, PROT_READ | PROT_WRITE, 
				  MAP_SHARED, handler, BASE);
	if(baseptr == MAP_FAILED) {
		printf("mmap failed: %s (%d)\n", strerror(errno), errno);
		return -1;
	}

	baseptr += PAGEALIGN;

	PIOBbaseptr = baseptr + AT91_PIOB; 

	//Save the original state
	pusr_b = *((volatile unsigned long*)( PIOBbaseptr + PIO_PUSR ));
	psr_b = *((volatile unsigned long*)( PIOBbaseptr + PIO_PSR ));

	//Set the pins to input PU
	*((volatile unsigned long*)( PIOBbaseptr + PIO_PER )) = MODEM_PINS_B;
	*((volatile unsigned long*)( PIOBbaseptr + PIO_PUER )) = MODEM_PINS_B;

	//Sleep
	nanosleep(&delay, NULL);

	//Restore the original values
	*((volatile unsigned long*)( PIOBbaseptr + PIO_PUDR )) = pusr_b;
	*((volatile unsigned long*)( PIOBbaseptr + PIO_PDR )) = ~(psr_b);

	return 0;
}

