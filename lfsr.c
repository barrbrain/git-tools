/* http://www.cryogenius.com/software/lfsr/ */

#include <stdint.h>
#include "lfsr.h"

/****************************************************************************
 * Galois linear feedback shift register pseudorandom number generator.
 * This LFSR cycles through 2^160-1 values before repeating.
 * Aaron Logue 2001 - To the public domain.
 *
 * Declare an external unsigned long lfsr[5] array and seed with nonzero value.
 * lfsr[0] contains most significant bits.
 *
 * The mask value below implements the following primitive polynomial:
 * X^160+X^159+X^158+X^157+X^155+X^153+X^151+X^150+X^149+X^148+X^147+X^146+
 *  X^142+X^141+X^137+X^134+X^133+X^132+X^130+
 * X^128+X^126+X^125+X^121+X^120+X^118+X^117+X^116+X^114+
 *  X^112+X^111+X^109+X^108+X^106+X^104+X^102+
 * X^95+ X^94+ X^90+ X^89+ X^88+ X^86+ X^85+ X^84+ X^83+ X^82+ X^81+
 *  X^80+ X^78+ X^76+ X^68+ X^66+
 * X^64+ X^61+ X^60+ X^59+ X^57+ X^52+ X^50+
 *  X^46+ X^45+ X^41+ X^40+ X^39+ X^38+ X^37+ X^36+ X^35+
 * X^31+ X^29+ X^27+ X^26+ X^25+ X^23+ X^20+ X^18+
 *  X^16+ X^11+ X^10+ X^8+  X^7+  X^6+  X^5+  X^3+  X+ 1
 ***************************************************************************/
#define MASK0 0xF57E313A
#define MASK1 0xB1BADAA0
#define MASK2 0x63BFA80A
#define MASK3 0x9D0A31FC
#define MASK4 0x574A86F5
static unsigned long lfsr[5] = {MASK0, MASK1, MASK2, MASK3, MASK4};
void lfsr_shift(void)
{
	int lsb;
	lsb = lfsr[4] & 1;
	lfsr[4] = ((lfsr[3] & 0x00000001) << 31) | (lfsr[4] >> 1);
	lfsr[3] = ((lfsr[2] & 0x00000001) << 31) | (lfsr[3] >> 1);
	lfsr[2] = ((lfsr[1] & 0x00000001) << 31) | (lfsr[2] >> 1);
	lfsr[1] = ((lfsr[0] & 0x00000001) << 31) | (lfsr[1] >> 1);
	lfsr[0] = lfsr[0] >> 1;
	if (lsb) {
		lfsr[4] ^= MASK4;
		lfsr[3] ^= MASK3;
		lfsr[2] ^= MASK2;
		lfsr[1] ^= MASK1;
		lfsr[0] ^= MASK0;
	}
}

uint32_t lfsr_160_u32(void)
{
	static int i;
	if (i % 5 == 0)
		lfsr_shift();
	return lfsr[i++ % 5];
}
