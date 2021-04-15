#include <limits.h>
#include <lrtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tunstall.h"

void tunstall_decomp(const u8 *in, u8 *out, const u16 outlen) {

	const u8 * const origout = out;
	u16 numentries = *in++;
	if (!numentries)
		numentries = 256;

	u8 dict[512];
	u8 lens[256];
	u16 starts[256];
	u8 cur = 0;
	u16 i;
	u8 nonones;

	while (1) {
		const u8 num = *in++;
		const u8 len = *in++;

		memset(&lens[cur], len, num);
		cur += num;

		// 1 is omitted if obvious
		if (len == 2) {
			const u8 onenum = *in++;
			memset(&lens[cur], 1, onenum);

			nonones = numentries - onenum;
			break;
		}
	}

	// Fill in starts array
	u16 cum = 0, nononecum = 0;
	for (i = 0; i < numentries; i++) {
		starts[i] = cum;
		cum += lens[i];
		if (i < nonones)
			nononecum += lens[i];
	}

	// Read dict
	memcpy(dict, in, nononecum);
	in += nononecum;
}
