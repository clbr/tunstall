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
	u8 ones;

	while (1) {
		const u8 num = *in++;
		const u8 len = *in++;

		memset(&lens[cur], len, num);
		cur += num;

		// 1 is omitted if obvious
		if (len == 2) {
			ones = *in++;
			memset(&lens[cur], 1, ones);

			nonones = numentries - ones;
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

	// Ones as a bitmap
	const u8 bytebase = nonones / 8;
	const u8 numbytes = ones / 8 + (ones % 8 ? 1 : 0);
	u16 dictpos = nononecum;
	for (i = 0; i < numbytes; i++) {
		const u8 byte = *in++;
		if (!byte)
			continue;
		u8 bit;
		for (bit = 0; bit < 8; bit++) {
			if (byte & (1 << bit)) {
				dict[dictpos++] = (i + bytebase) * 8 + bit;
			}
		}
	}

	// Unpack stream
	do {
		cur = *in++;
		const u8 len = lens[cur];
		memcpy(out, &dict[starts[cur]], len);
		out += len;
	} while (out - origout < outlen);

	if (out - origout != outlen) abort();
}
