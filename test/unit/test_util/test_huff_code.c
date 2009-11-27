/**
 * @file test_huff_code.c Test Huffman coding
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "huff_code.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int32 const ivalues[10] = {
	1, 2, 3, 5, 7, 11, 13, 17, 19, 23
};
int32 const frequencies[10] = {
	42, 4, 5, 6, 225, 15001, 3, 2, 87, 1003
};
char * const svalues[10] = {
	"foo", "bar", "baz", "quux", "argh",
	"hurf", "burf", "blatz", "unf", "woof"
};

void
test_intcode(huff_code_t *hc)
{
	FILE *fh;
	int i;

	fh = fopen("hufftest.out", "wb");
	huff_code_attach(hc, fh, "wb");
	for (i = 0; i < 10; ++i) {
		huff_code_encode_int(hc, ivalues[i], NULL);
	}
	huff_code_detach(hc);
	fclose(fh);
	fh = fopen("hufftest.out", "rb");
	huff_code_attach(hc, fh, "rb");
	for (i = 0; i < 10; ++i) {
		int val = huff_code_decode_int(hc, NULL, 0);
		printf("%d ", val);
		TEST_EQUAL(val, ivalues[i]);
	}
	printf("\n");
	huff_code_detach(hc);
	fclose(fh);
}

void
test_strcode(huff_code_t *hc)
{
	FILE *fh;
	int i;

	fh = fopen("hufftest.out", "wb");
	huff_code_attach(hc, fh, "wb");
	for (i = 0; i < 10; ++i) {
		huff_code_encode_str(hc, svalues[i], NULL);
	}
	huff_code_detach(hc);
	fclose(fh);
	fh = fopen("hufftest.out", "rb");
	huff_code_attach(hc, fh, "rb");
	for (i = 0; i < 10; ++i) {
		char const *val = huff_code_decode_str(hc, NULL, 0);
		printf("%s ", val);
		TEST_EQUAL(0, strcmp(val, svalues[i]));
	}
	printf("\n");
	huff_code_detach(hc);
	fclose(fh);
}

int
main(int argc, char *argv[])
{
	huff_code_t *hc;
	FILE *fh;

	hc = huff_code_build_int(ivalues, frequencies, 10);
	huff_code_dump(hc, stdout);
	test_intcode(hc);
	fh = fopen("huffcode.out", "wb");
	huff_code_write(hc, fh);
	fclose(fh);
	huff_code_free(hc);

	fh = fopen("huffcode.out", "rb");
	hc = huff_code_read(fh);
	fclose(fh);
	test_intcode(hc);
	huff_code_free(hc);

	hc = huff_code_build_str(svalues, frequencies, 10);
	huff_code_dump(hc, stdout);
	test_strcode(hc);
	fh = fopen("huffcode.out", "wb");
	huff_code_write(hc, fh);
	fclose(fh);
	huff_code_free(hc);

	fh = fopen("huffcode.out", "rb");
	hc = huff_code_read(fh);
	fclose(fh);
	test_strcode(hc);
	huff_code_free(hc);

	return 0;
}

