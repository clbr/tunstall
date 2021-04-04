#include <limits.h>
#include <lrtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XXH_INLINE_ALL
#include "xxhash.h"

using namespace std;

static void die(const char why[]) {
	printf("%s\n", why);
	exit(1);
}

#define MAXSIZE 32768

class piecer {
public:
	piecer(const u8 *mem_): mem(mem_) {
		clear();
	}

	void add(const u16 addr, const u8 len) {
		u16 i;
		const u16 curmax = num[len - 2];
		const u8 hashed = hash(addr, len);

		for (i = hashmap[hashed]; i < MAXSIZE; i = hashnext[i]) {
			if (!memcmp(&mem[addr], &mem[entries[len - 2][i].addr], len)) {
				// Found it, just add one
				entries[len - 2][i].num++;

				if (entries[len - 2][i].num > largestnum[len - 2]) {
					largestnum[len - 2] = entries[len - 2][i].num;
					largestpos[len - 2] = i;
				}
				return;
			}
		}

		// Didn't find it, add a new one, clearing it first
		entries[len - 2][curmax].addr = addr;
		entries[len - 2][curmax].num = 1;

		hashnext[curmax] = hashmap[hashed];
		hashmap[hashed] = curmax;

		if (!largestnum[len - 2]) {
			largestnum[len - 2] = 1;
			largestpos[len - 2] = curmax;
		}

		num[len - 2]++;
	}

	u32 size(const u8 len) const {
		return num[len - 2];
	}

	const u8 *best(const u8 len, u16 &outbestnum) const {
		outbestnum = largestnum[len - 2];

/*		u16 i, opts = 0;
		const u16 curmax = num[len - 2];
		for (i = 0; i < curmax; i++) {
			if (entries[len - 2][i].num != outbestnum)
				continue;
			opts++;
		}
		if (opts > 1) printf("%u options\n", opts);*/

		return &mem[entries[len - 2][largestpos[len - 2]].addr];
	}

	void clearhash() {
		memset(hashmap, 0xff, sizeof(hashmap));
		memset(hashnext, 0xff, sizeof(hashnext));
	}

	void clear() {
		memset(num, 0, sizeof(num));
		memset(largestpos, 0, sizeof(largestpos));
		memset(largestnum, 0, sizeof(largestnum));

		clearhash();
	}

private:
	const u8 *mem;

	struct entry {
		u16 addr;
		u16 num;
	};

	entry entries[128 + 1 - 2][MAXSIZE];
	u16 num[128 + 1 - 2];

	u16 largestpos[128 + 1 - 2], largestnum[128 + 1 - 2];

	// Hashes are only used while adding, during one len
	u16 hashnext[MAXSIZE];
	u16 hashmap[64];

	u8 hash(const u16 addr, const u8 len) const {
		return XXH3_64bits(&mem[addr], len) & 63;
	}
};

struct entry {
	char data[128];
	u8 len;
};

static int lencmp(const void *ap, const void *bp) {
	const entry * const a = (entry *) ap;
	const entry * const b = (entry *) bp;

	if (a->len > b->len)
		return -1;
	if (a->len < b->len)
		return 1;
	return 0;
}

int main(int argc, char **argv) {

	if (argc < 2) die("Need file");

	FILE *f = fopen(argv[1], "r");
	if (!f) die("Can't open");

	fseek(f, 0, SEEK_END);
	const u32 len = ftell(f);
	rewind(f);

	if (len > MAXSIZE)
		die("Can't compress over 32k");

	u8 * const mem = (u8 *) malloc(len);

	if (fread(mem, len, 1, f) != 1)
		die("Read");

	fclose(f);

	printf("Read %u bytes\n\n", len);

	// Analyze
	piecer *pc = new piecer(mem);
	u32 maxes[129];
	u8 lastlevel = 128;

	u8 bytes[256] = { 0 };

	u32 i;
	for (i = 0; i < len; i++) {
		bytes[mem[i]] = 1;
	}

	entry entries[256];

	u32 used = 0;
	for (i = 0; i < 256; i++) {
		if (bytes[i]) {
			entries[used].data[0] = i;
			entries[used].len = 1;
			used++;
		}
	}
	printf("%u values used\n", used);

	for (i = 2; i <= 128; i++) {
		u32 p;
		for (p = 0; p < len; p++) {
			if (p + i >= len)
				break;
			pc->add(p, i);
		}
		pc->clearhash();

		printf("Length %u had %u unique matches\n", i, pc->size(i));

		u16 max = 0;
		pc->best(i, max);
		printf("Largest amount was %u\n", max);
		maxes[i] = max;

		if (max == 1) {
			lastlevel = i;
			break;
		}
	}

	puts("");

	u32 bestamount = 0;
	u8 best = 0;

	for (i = 2; i <= lastlevel; i++) {
		const u32 mul = i * maxes[i];
		if (mul > bestamount) {
			best = i;
			bestamount = mul;
		}
	}

	printf("Best amount was %u, at %u\n", bestamount, best);

	u32 numentries = used + 1;

	u16 unused;
	const u8 *ptr = pc->best(best, unused);
	memcpy(entries[used].data, ptr, best);
	entries[used].len = best;

	printf("Contents: ");
	for (i = 0; i < best; i++) {
		printf("%u,", ptr[i]);
	}
	puts("");

	// Erase them from the memmap
	u8 * const memmap = (u8 *) calloc(1, len);
	u32 erased = 0;

	for (i = 0; i < len - best; i++) {
		if (!memcmp(&entries[used].data[0], &mem[i], best)) {
			memset(&memmap[i], 1, best);
			i += best - 1;
			erased += best;
		}
	}

	// We have our first entry. Time to iterate
	while (numentries < 256) {
		printf("Iterating. %u entries found, %u/%u bytes\n",
			numentries, erased, len);

		memset(maxes, 0, 129);
		pc->clear();

		for (i = 2; i <= lastlevel; i++) {
			u32 p;
			for (p = 0; p < len; p++) {
				if (p + i >= len)
					break;

				if (u8 *ptr = (u8 *) memrchr(&memmap[p], 1, i)) {
					const u32 dist = ptr - &memmap[p];
					if (dist)
						p += dist - 1;
					continue;
				}

				pc->add(p, i);
			}
			pc->clearhash();

			u16 max = 0;
			pc->best(i, max);
			printf("Length %u had %u unique matches, max %u\n", i,
				pc->size(i), max);

			maxes[i] = max;

			if (max == 1) {
				lastlevel = i;
				break;
			} else if (!max) {
				lastlevel = i - 1;
				break;
			}
		}

		bestamount = 0;
		best = 0;

		for (i = 2; i <= lastlevel; i++) {
			const u32 mul = i * maxes[i];
			if (mul > bestamount) {
				best = i;
				bestamount = mul;
			}
		}
		printf("Best amount was %u, at %u\n", bestamount, best);

		if (!best) // Found nothing
			break;

		// Found a new best
		ptr = pc->best(best, unused);
		memcpy(entries[numentries].data, ptr, best);
		entries[numentries].len = best;
		numentries++;

		printf("Contents: ");
		for (i = 0; i < best; i++) {
			printf("%u,", ptr[i]);
		}
		puts("");

		// Erase from map
		for (i = 0; i < len - best; i++) {

			if (u8 *ptr = (u8 *) memrchr(&memmap[i], 1, best)) {
				const u32 dist = ptr - &memmap[i];
				if (dist)
					i += dist - 1;
				continue;
			}

			if (!memcmp(&entries[numentries - 1].data[0], &mem[i], best)) {
				memset(&memmap[i], 1, best);
				i += best - 1;
				erased += best;
			}
		}

		// End conditions
		// If longest open streak is 1
		if (lastlevel == 1) {
			puts("No more runs to find");
			break;
		}

		// If dict length exceeds 512
		u32 dlen = 0;
		for (i = 0; i < numentries; i++)
			dlen += entries[i].len;
		if (dlen >= 512) {
			puts("Dictionary length capped");
			break;
		}
	}

	puts("");

	u32 dlen = 0;
	for (i = 0; i < numentries; i++)
		dlen += entries[i].len;
	printf("Dictionary has %u entries, %u total length.\n", numentries, dlen);

	qsort(entries, numentries, sizeof(entry), lencmp);

	u32 k;
	for (i = 0; i < numentries; i++) {
		printf("Entry %u: len %u: ", i, entries[i].len);
		for (k = 0; k < entries[i].len; k++)
			printf("%u,", entries[i].data[k]);
		puts("");
	}

	// Finally, calculate compressed size
	u8 * const compressed = (u8 *) calloc(1, len);
	u8 * const testbuf = (u8 *) calloc(1, len);
	u8 *out = compressed;
	u32 compsize = 0;
	for (i = 0; i < len;) {
		for (k = 0; k < numentries; k++) {
			if (i + entries[k].len > len)
				continue;
			if (!memcmp(&mem[i], &entries[k].data[0], entries[k].len)) {
				compsize++;
				*out++ = k;
				i += entries[k].len;
				goto next;
			}
		}

		printf("Not found at %u, val %u\n", i, mem[i]);
		abort();

		next:;
	}
	if (i != len)
		abort();

	printf("Compressed stream size %u\n", compsize);

	// Attempt decompression to check it
	u8 *in = compressed;
	out = testbuf;

	while (out - testbuf < len) {
		const u8 i = *in++;
		const u16 dlen = entries[i].len;
		memcpy(out, &entries[i].data[0], dlen);
		out += dlen;
	}

	if (out != testbuf + len)
		abort();

	if (!memcmp(mem, testbuf, len))
		puts("Decompression ok");
	else
		puts("FAIL");

	free(testbuf);
	free(compressed);
	free(mem);
	free(memmap);
	return 0;
}
