#include <stdio.h>
#include <string.h>
#include <math.h>

#include "feat.h"
#include "ckd_alloc.h"

const float32 data[6][13] = {
	{ 15.114, -1.424, -0.953, 0.186, -0.656, -0.226, -0.105, -0.412, -0.024, -0.091, -0.124, -0.158, -0.197},
	{ 14.729, -1.313, -0.892, 0.140, -0.676, -0.089, -0.313, -0.422, -0.058, -0.101, -0.100, -0.128, -0.123},
	{ 14.502, -1.351, -1.028, -0.189, -0.718, -0.139, -0.121, -0.365, -0.139, -0.154, 0.041, 0.009, -0.073},
	{ 14.557, -1.676, -0.864, 0.118, -0.445, -0.168, -0.069, -0.503, -0.013, 0.007, -0.056, -0.075, -0.237},
	{ 14.665, -1.498, -0.582, 0.209, -0.487, -0.247, -0.142, -0.439, 0.059, -0.058, -0.265, -0.109, -0.196},
	{ 15.025, -1.199, -0.607, 0.235, -0.499, -0.080, -0.062, -0.554, -0.209, -0.124, -0.445, -0.352, -0.400},
};

int
main(int argc, char *argv[])
{
	feat_t *fcb;
	float32 **in_feats, ***out_feats;
	int32 i, j;

	/* Test "raw" features without concatenation */
	fcb = feat_init(strdup("13"), "none", "no", "none", 1);

	in_feats = (float32 **)ckd_alloc_2d_ptr(6, 13, data, sizeof(float32));
	out_feats = (float32 ***)ckd_calloc_3d(6, 1, 13, sizeof(float32));
	feat_s2mfc2feat_block(fcb, in_feats, 6, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < 13; ++j) {
			printf("%.3f ", out_feats[i][0][j]);
		}
		printf("\n");
	}
	feat_free(fcb);

	/* Test "raw" features with concatenation */
	fcb = feat_init(strdup("13:1"), "none", "no", "none", 1);

	in_feats = (float32 **)ckd_alloc_2d_ptr(6, 13, data, sizeof(float32));
	out_feats = (float32 ***)ckd_calloc_3d(8, 1, 39, sizeof(float32));
	feat_s2mfc2feat_block(fcb, in_feats, 6, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < 39; ++j) {
			printf("%.3f ", out_feats[i][0][j]);
		}
		printf("\n");
	}
	feat_free(fcb);

	/* Test 1s_c_d_dd features */
	fcb = feat_init(strdup("1s_c_d_dd"), "none", "no", "none", 1);
	feat_s2mfc2feat_block(fcb, in_feats, 6, 1, 1, out_feats);

	for (i = 0; i < 6; ++i) {
		for (j = 0; j < 39; ++j) {
			printf("%.3f ", out_feats[i][0][j]);
		}
		printf("\n");
	}

	/* Verify that the deltas are correct. */
	for (i = 2; i < 4; ++i) {
		for (j = 0; j < 13; ++j) {
			if (fabs(out_feats[i][0][13+j] - 
				 (out_feats[i+2][0][j]
				  - out_feats[i-2][0][j])) > 0.01) {
				printf("Delta mismatch in [%d][%d]\n", i, j);
				return 1;
			}
		}
	}
	feat_free(fcb);

	return 0;
}
