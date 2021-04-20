/*  A fast brute-force Tunstall implementation
    Copyright (C) 2021 Lauri Kasanen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <limits.h>
#include <lrtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tunstall.h"

#define MAXSIZE 32768

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

	if (len > MAXSIZE)
		die("Can't compress over 32k");

	u8 mem[MAXSIZE], compressed[MAXSIZE], testbuf[MAXSIZE];

	if (fread(mem, len, 1, f) != 1)
		die("Read");

	fclose(f);

	printf("Read %u bytes\n\n", len);

	u16 complen = tunstall_comp(mem, compressed, len);
	if (complen == USHRT_MAX)
		die("Failed to compress");

	// Attempt decompression to check it
	tunstall_decomp(compressed, testbuf, len);

	if (!memcmp(mem, testbuf, len)) {
		puts("Decompression ok");
	} else {
		puts("FAIL");
		abort();
	}

	// Output
	char outname[PATH_MAX];
	if (argc == 3) {
		strcpy(outname, argv[2]);
	} else {
		strcpy(outname, argv[1]);
		strcat(outname, ".tunstall");
	}

	printf("\nSaving compressed file to %s\n", outname);
	f = fopen(outname, "w");
	if (!f) die("Can't open for writing");

	fwrite(compressed, complen, 1, f);
	const u32 wrote = ftell(f);

	fclose(f);

	printf("Wrote %u bytes, %.2f%%\n", wrote, wrote * 100.0f / len);

	return 0;
}
