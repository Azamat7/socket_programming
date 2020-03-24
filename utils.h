#include <stdint.h>

void split_length(uint32_t length, uint16_t* ua, uint16_t* ub);
unsigned short checksum1(const char *buf, unsigned size);
unsigned short checksum2(const char *header, const char *buf, unsigned size);
void cipher(char *text, int op, int shift, int length);