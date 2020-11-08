#include <lrtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <vector>

using namespace std;

static void die(const char why[]) {
	printf("%s\n", why);
	exit(1);
}

struct entry {
	vector<u8> data;
};

static int lencmp(const void *ap, const void *bp) {
	const entry * const a = (entry *) ap;
	const entry * const b = (entry *) bp;

	if (a->data.size() > b->data.size())
		return -1;
	if (a->data.size() < b->data.size())
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

	u8 * const mem = (u8 *) malloc(len);

	if (fread(mem, len, 1, f) != 1)
		die("Read");

	fclose(f);

	printf("Read %u bytes\n\n", len);

	// Analyze
	map<vector<u8>, u32> stuff[129];
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
			entries[used].data.push_back(i);
			used++;
		}
	}
	printf("%u values used\n", used);

	for (i = 2; i <= 128; i++) {
		u32 p;
		u32 max = 0;
		for (p = 0; p < len; p++) {
			if (p + i >= len)
				break;
			vector<u8> vec;
			u8 k;
			for (k = 0; k < i; k++)
				vec.push_back(mem[p + k]);

			const u32 one = ++stuff[i][vec];
			if (one > max)
				max = one;
		}

		printf("Length %u had %lu unique matches\n", i, stuff[i].size());

/*		u32 max = 0;
		const map<vector<u8>, u32> &cur = stuff[i];
		const u32 num = cur.size();
		for (p = 0; p < num; p++) {
			map<vector<u8>, u32>::const_iterator it;
			for (it = cur.begin(); it != cur.end(); it++) {
				if (it->second > max)
					max = it->second;
			}
		}*/

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

	map<vector<u8>, u32>::const_iterator it;
	for (it = stuff[best].begin(); it != stuff[best].end(); it++) {
		if (it->second != maxes[best])
			continue;

		entries[used].data = it->first;

		printf("Contents: ");
		for (i = 0; i < it->first.size(); i++) {
			printf("%u,", it->first[i]);
		}
		puts("");
		break;
	}

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

		for (i = 2; i <= lastlevel; i++) {
			u32 p;
			u32 max = 0;
			stuff[i].clear();
			for (p = 0; p < len; p++) {
				if (p + i >= len)
					break;

				if (u8 *ptr = (u8 *) memrchr(&memmap[p], 1, i)) {
					const u32 dist = ptr - &memmap[p];
					if (dist)
						p += dist - 1;
					continue;
				}

				vector<u8> vec;
				u8 k;
				for (k = 0; k < i; k++)
					vec.push_back(mem[p + k]);

				const u32 one = ++stuff[i][vec];
				if (one > max)
					max = one;
			}
			printf("Length %u had %lu unique matches, max %u\n", i,
				stuff[i].size(), max);

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

		// Found a new best
		for (it = stuff[best].begin(); it != stuff[best].end(); it++) {
			if (it->second != maxes[best])
				continue;

			entries[numentries++].data = it->first;

			printf("Contents: ");
			for (i = 0; i < it->first.size(); i++) {
				printf("%u,", it->first[i]);
			}
			puts("");
			break;
		}

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
			dlen += entries[i].data.size();
		if (dlen >= 512) {
			puts("Dictionary length capped");
			break;
		}
	}

	puts("");

	u32 dlen = 0;
	for (i = 0; i < numentries; i++)
		dlen += entries[i].data.size();
	printf("Dictionary has %u entries, %u total length.\n", numentries, dlen);

	qsort(entries, numentries, sizeof(entry), lencmp);

	u32 k;
	for (i = 0; i < numentries; i++) {
		printf("Entry %u: len %lu\n\t", i, entries[i].data.size());
		for (k = 0; k < entries[i].data.size(); k++)
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
			if (i + entries[k].data.size() > len)
				continue;
			if (!memcmp(&mem[i], &entries[k].data[0], entries[k].data.size())) {
				compsize++;
				*out++ = k;
				i += entries[k].data.size();
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
		const u16 dlen = entries[i].data.size();
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
