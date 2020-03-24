#include <stdint.h>
#include "utils.h"

void split_length(uint32_t length, uint16_t* ua, uint16_t* ub){
    *ua = (uint16_t) (length >> 16);
    *ub = (uint16_t) (length & 0x0000FFFFuL);
}

unsigned short checksum1(const char *buf, unsigned size)
{
	unsigned sum = 0;
	int i;

	/* Accumulate checksum */
	for (i = 0; i < size - 1; i += 2)
	{
		unsigned short word16 = *(unsigned short *) &buf[i];
		sum += word16;
	}

	/* Handle odd-sized case */
	if (size & 1)
	{
		unsigned short word16 = (unsigned char) buf[i];
		sum += word16;
	}

	/* Fold to get the ones-complement result */
	while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

	/* Invert to get the negative in ones-complement arithmetic */
	return ~sum;
}


unsigned short checksum2(const char *header, const char *buf, unsigned size)
{
	unsigned sum = 0;
	int i;
    
    /* Accumulate checksum */
	for (i = 0; i < 8; i += 2)
	{
		unsigned short word16 = *(unsigned short *) &header[i];
		sum += word16;
	}

	/* Accumulate checksum */
	for (i = 0; i < size - 1; i += 2)
	{
		unsigned short word16 = *(unsigned short *) &buf[i];
		sum += word16;
	}

	/* Handle odd-sized case */
	if (size & 1)
	{
		unsigned short word16 = (unsigned char) buf[i];
		sum += word16;
	}

	/* Fold to get the ones-complement result */
	while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

	/* Invert to get the negative in ones-complement arithmetic */
	return ~sum;
}