#include <lrtypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <vector>

using namespace std;

static void die(const char why[]) {
	printf("%s\n", why);
	exit(1);
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

	u8 i;
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

		if (max == 1)
			break;
	}

	free(mem);
	return 0;
}
