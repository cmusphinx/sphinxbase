/* ====================================================================
 * Copyright (c) 1996-2004 Carnegie Mellon University.  All rights 
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
#include <stdio.h>
#include <stdlib.h>
#if !defined(WIN32)
#include <unistd.h>
#include <sys/file.h>
#if !defined(O_BINARY)
#define O_BINARY 0
#endif
#endif
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#if defined(WIN32)
#include <io.h>
#include <errno.h>
#endif

#include "fe.h"
#include "cmd_ln.h"
#include "err.h"
#include "ckd_alloc.h"
#include "byteorder.h"

#include "wave2feat.h"
#include "cmd_ln_defn.h"

struct globals_s {
	param_t params;
	int32 nskip;
	int32 runlen;
	char *wavfile;
	char *cepfile;
	char *ctlfile;
	char *wavdir;
	char *cepdir;
	char *wavext;
	char *cepext;
	int32 input_format;
	int32 is_batch;
	int32 is_single;
	int32 blocksize;
	int32 machine_endian;
	int32 input_endian;
	int32 output_endian;
	int32 nchans;
	int32 whichchan;
	int32 idct;
};
typedef struct globals_s globals_t;

globals_t *fe_parse_options(int argc, char **argv);
int32 fe_convert_files(globals_t *P);
int32 fe_build_filenames(globals_t *P, char *fileroot, char **infilename, char **outfilename);
char *fe_copystr(char *dest_str, char *src_str);
int32 fe_count_frames(fe_t *FE, int32 nsamps, int32 count_partial_frames);
int32 fe_readspch(globals_t *P, char *infile, int16 **spdata, int32 *splen);
int32 fe_writefeat(fe_t *FE, char *outfile, int32 nframes, mfcc_t **feat);
int32 fe_free_param(globals_t *P);
int32 fe_openfiles(globals_t *P, fe_t *FE, char *infile, int32 *fp_in, int32 *nsamps, 
		   int32 *nframes, int32 *nblocks, char *outfile, int32 *fp_out);
int32 fe_readblock_spch(globals_t *P, int32 fp, int32 nsamps, int16 *buf);
int32 fe_writeblock_feat(globals_t *P, fe_t *FE, int32 fp, int32 nframes, mfcc_t **feat);
int32 fe_closefiles(int32 fp_in, int32 fp_out);
int32 fe_convert_logspec(globals_t *P, fe_t *FE, char *infile, char *outfile);

/*       
         7-Feb-00 M. Seltzer - wrapper created for new front end -
         does blockstyle processing if necessary. If input stream is
         greater than DEFAULT_BLOCKSIZE samples (currently 200000)
         then it will read and write in DEFAULT_BLOCKSIZE chunks. 
         
         Had to change fe_process_utt(). Now the 2d feature array
         is allocated internally to that function rather than
         externally in the wrapper. 
         
         Added usage display with -help switch for help

         14-Feb-00 M. Seltzer - added NIST header parsing for 
         big endian/little endian parsing. kind of a hack.

         changed -wav switch to -nist to avoid future confusion with
         MS wav files
         
         added -mach_endian switch to specify machine's byte format
*/

int32
main(int32 argc, char **argv)
{
	globals_t *P;

	P = fe_parse_options(argc, argv);
	if (fe_convert_files(P) != FE_SUCCESS) {
		E_FATAL("error converting files...exiting\n");
	}
	free(P);
	return (0);
}


int32
fe_convert_files(globals_t * P)
{

	fe_t *FE;
	char *infile, *outfile, fileroot[MAXCHARS];
	FILE *ctlfile;
	int16 *spdata = NULL;
	int32 splen =
	    0, total_samps, frames_proc, nframes, nblocks, last_frame;
	int32 fp_in, fp_out, last_blocksize = 0, curr_block, total_frames;
	mfcc_t **cep = NULL, **last_frame_cep;
	int32 return_value;
	int32 warn_zero_energy = OFF;
	int32 process_utt_return_value;

	if ((FE = fe_init(&P->params)) == NULL) {
		E_ERROR("memory alloc failed...exiting\n");
		return (FE_MEM_ALLOC_ERROR);
	}

	if (P->is_batch) {
		int32 nskip = P->nskip;
		int32 runlen = P->runlen;

		if ((ctlfile = fopen(P->ctlfile, "r")) == NULL) {
			E_ERROR("Unable to open control file %s\n",
				P->ctlfile);
			return (FE_CONTROL_FILE_ERROR);
		}
		while (fscanf(ctlfile, "%s", fileroot) != EOF) {
			if (nskip > 0) {
				--nskip;
				continue;
			}
			if (runlen > 0) {
				--runlen;
			}
			else if (runlen == 0) {
				break;
			}

			fe_build_filenames(P, fileroot, &infile, &outfile);

			if (P->params.verbose)
				E_INFO("%s\n", infile);

			if (P->idct) {
				/* Special case for doing logspec to MFCC. */
				return_value = fe_convert_logspec(P, FE, infile, outfile);
				if (return_value != FE_SUCCESS)
					return return_value;
				continue;
			}
			return_value =
			    fe_openfiles(P, FE, infile, &fp_in,
					 &total_samps, &nframes, &nblocks,
					 outfile, &fp_out);
			if (return_value != FE_SUCCESS) {
				return (return_value);
			}

			warn_zero_energy = OFF;

			if (nblocks * P->blocksize >= total_samps)
				last_blocksize =
				    total_samps - (nblocks -
						   1) * P->blocksize;

			if (!fe_start_utt(FE)) {
				curr_block = 1;
				total_frames = frames_proc = 0;
				/*execute this loop only if there is more than 1 block to
				   be processed */
				while (curr_block < nblocks) {
					splen = P->blocksize;
					if ((spdata =
					     (int16 *) calloc(splen,
							      sizeof
							      (int16))) ==
					    NULL) {
						E_ERROR
						    ("Unable to allocate memory block of %d shorts for input speech\n",
						     splen);
						return
						    (FE_MEM_ALLOC_ERROR);
					}
					if (fe_readblock_spch
					    (P, fp_in, splen,
					     spdata) != splen) {
						E_ERROR
						    ("error reading speech data\n");
						return
						    (FE_INPUT_FILE_READ_ERROR);
					}
					process_utt_return_value =
					    fe_process_utt(FE, spdata,
							   splen, &cep,
							   &frames_proc);
					if (process_utt_return_value !=
					    FE_SUCCESS) {
						if (FE_ZERO_ENERGY_ERROR ==
						    process_utt_return_value)
						{
							warn_zero_energy =
							    ON;
						}
						else {
							return
							    (process_utt_return_value);
						}
					}
					if (frames_proc > 0)
						fe_writeblock_feat(P, FE,
								   fp_out,
								   frames_proc,
								   cep);
					if (cep != NULL) {
						ckd_free_2d((void **) cep);
						cep = NULL;
					}
					curr_block++;
					total_frames += frames_proc;
					if (spdata != NULL) {
						free(spdata);
						spdata = NULL;
					}
				}
				/* process last (or only) block */
				if (spdata != NULL) {
					free(spdata);
					spdata = NULL;
				}
				splen = last_blocksize;

				if ((spdata =
				     (int16 *) calloc(splen,
						      sizeof(int16))) ==
				    NULL) {
					E_ERROR
					    ("Unable to allocate memory block of %d shorts for input speech\n",
					     splen);
					return (FE_MEM_ALLOC_ERROR);
				}

				if (fe_readblock_spch
				    (P, fp_in, splen, spdata) != splen) {
					E_ERROR
					    ("error reading speech data\n");
					return (FE_INPUT_FILE_READ_ERROR);
				}

				process_utt_return_value =
				    fe_process_utt(FE, spdata, splen, &cep,
						   &frames_proc);
				if (process_utt_return_value != FE_SUCCESS) {
					if (FE_ZERO_ENERGY_ERROR ==
					    process_utt_return_value) {
						warn_zero_energy = ON;
					}
					else {
						return
						    (process_utt_return_value);
					}
				}
				if (frames_proc > 0)
					fe_writeblock_feat(P, FE, fp_out,
							   frames_proc,
							   cep);
				if (cep != NULL) {
					ckd_free_2d((void **) cep);
					cep = NULL;
				}
				curr_block++;
				if (P->params.logspec != ON)
					last_frame_cep =
					    (mfcc_t **) ckd_calloc_2d(1,
								      FE->
								      NUM_CEPSTRA,
								      sizeof
								      (float32));
				else
					last_frame_cep =
					    (mfcc_t **) ckd_calloc_2d(1,
								      FE->
								      MEL_FB->
								      num_filters,
								      sizeof
								      (float32));
				process_utt_return_value =
				    fe_end_utt(FE, last_frame_cep[0],
					       &last_frame);
				if (FE_ZERO_ENERGY_ERROR ==
				    process_utt_return_value) {
					warn_zero_energy = ON;
				}
				else {
					assert(process_utt_return_value ==
					       FE_SUCCESS);
				}
				if (last_frame > 0) {
					fe_writeblock_feat(P, FE, fp_out,
							   last_frame,
							   last_frame_cep);
					frames_proc++;
				}
				total_frames += frames_proc;

				fe_closefiles(fp_in, fp_out);
				if (spdata != NULL) {
					free(spdata);
					spdata = NULL;
				}
				if (last_frame_cep != NULL) {
					ckd_free_2d((void **)
						    last_frame_cep);
					last_frame_cep = NULL;
				}
				if (ON == warn_zero_energy) {
					E_WARN
					    ("File %s has some frames with zero energy. Consider using dither\n",
					     infile);
				}
			}
			else {
				E_ERROR("fe_start_utt() failed\n");
				return (FE_START_ERROR);
			}
		}
		fe_close(FE);
	}

	else if (P->is_single) {

		fe_build_filenames(P, fileroot, &infile, &outfile);
		if (P->params.verbose)
			printf("%s\n", infile);

		if (P->idct)
			/* Special case for doing logspec to MFCC. */
			return fe_convert_logspec(P, FE, infile, outfile);

		return_value =
		    fe_openfiles(P, FE, infile, &fp_in, &total_samps,
				 &nframes, &nblocks, outfile, &fp_out);
		if (return_value != FE_SUCCESS) {
			return (return_value);
		}

		warn_zero_energy = OFF;

		if (nblocks * P->blocksize >= total_samps)
			last_blocksize =
			    total_samps - (nblocks - 1) * P->blocksize;

		if (!fe_start_utt(FE)) {
			curr_block = 1;
			total_frames = frames_proc = 0;
			/*execute this loop only if there are more than 1 block to
			   be processed */
			while (curr_block < nblocks) {
				splen = P->blocksize;
				if ((spdata =
				     (int16 *) calloc(splen,
						      sizeof(int16))) ==
				    NULL) {
					E_ERROR
					    ("Unable to allocate memory block of %d shorts for input speech\n",
					     splen);
					return (FE_MEM_ALLOC_ERROR);
				}
				if (fe_readblock_spch
				    (P, fp_in, splen, spdata) != splen) {
					E_ERROR
					    ("Error reading speech data\n");
					return (FE_INPUT_FILE_READ_ERROR);
				}
				process_utt_return_value =
				    fe_process_utt(FE, spdata, splen, &cep,
						   &frames_proc);
				if (FE_ZERO_ENERGY_ERROR ==
				    process_utt_return_value) {
					warn_zero_energy = ON;
				}
				else {
					assert(process_utt_return_value ==
					       FE_SUCCESS);
				}
				if (frames_proc > 0)
					fe_writeblock_feat(P, FE, fp_out,
							   frames_proc,
							   cep);
				if (cep != NULL) {
					ckd_free_2d((void **) cep);
					cep = NULL;
				}
				curr_block++;
				total_frames += frames_proc;
				if (spdata != NULL) {
					free(spdata);
					spdata = NULL;
				}
			}
			/* process last (or only) block */
			if (spdata != NULL) {
				free(spdata);
				spdata = NULL;
			}
			splen = last_blocksize;
			if ((spdata =
			     (int16 *) calloc(splen,
					      sizeof(int16))) == NULL) {
				E_ERROR
				    ("Unable to allocate memory block of %d shorts for input speech\n",
				     splen);
				return (FE_MEM_ALLOC_ERROR);
			}
			if (fe_readblock_spch(P, fp_in, splen, spdata) !=
			    splen) {
				E_ERROR("Error reading speech data\n");
				return (FE_INPUT_FILE_READ_ERROR);
			}
			process_utt_return_value =
			    fe_process_utt(FE, spdata, splen, &cep,
					   &frames_proc);
			if (FE_ZERO_ENERGY_ERROR ==
			    process_utt_return_value) {
				warn_zero_energy = ON;
			}
			else {
				assert(process_utt_return_value ==
				       FE_SUCCESS);
			}
			if (frames_proc > 0)
				fe_writeblock_feat(P, FE, fp_out,
						   frames_proc, cep);
			if (cep != NULL) {
				ckd_free_2d((void **) cep);
				cep = NULL;
			}

			curr_block++;
			if (P->params.logspec != ON)
				last_frame_cep =
				    (mfcc_t **) ckd_calloc_2d(1,
							      FE->
							      NUM_CEPSTRA,
							      sizeof
							      (float32));
			else
				last_frame_cep =
				    (mfcc_t **) ckd_calloc_2d(1,
							      FE->MEL_FB->
							      num_filters,
							      sizeof
							      (float32));
			process_utt_return_value =
			    fe_end_utt(FE, last_frame_cep[0], &last_frame);
			if (FE_ZERO_ENERGY_ERROR ==
			    process_utt_return_value) {
				warn_zero_energy = ON;
			}
			else {
				assert(process_utt_return_value ==
				       FE_SUCCESS);
			}
			if (last_frame > 0) {
				fe_writeblock_feat(P, FE, fp_out,
						   last_frame,
						   last_frame_cep);
				frames_proc++;
			}
			total_frames += frames_proc;

			fe_closefiles(fp_in, fp_out);
			if (cep != NULL) {
				free(spdata);
				spdata = NULL;
			}
			if (last_frame_cep != NULL) {
				ckd_free_2d((void **) last_frame_cep);
				last_frame_cep = NULL;
			}
		}
		else {
			E_ERROR("fe_start_utt() failed\n");
			return (FE_START_ERROR);
		}

		fe_close(FE);
		if (ON == warn_zero_energy) {
			E_WARN
			    ("File %s has some frames with zero energy. Consider using dither\n",
			     infile);
		}
	}
	else {
		E_ERROR("Unknown mode - single or batch?\n");
		return (FE_UNKNOWN_SINGLE_OR_BATCH);

	}

	return (FE_SUCCESS);

}

void
fe_validate_parameters(globals_t * P)
{

	if ((P->is_batch) && (P->is_single)) {
		E_FATAL
		    ("You cannot define an input file and a control file\n");
	}

	if (P->wavfile == NULL && P->wavdir == NULL) {
		E_FATAL("No input file or file directory given\n");
	}

	if (P->cepfile == NULL && P->cepdir == NULL) {
		E_FATAL("No cepstra file or file directory given\n");
	}

	if (P->ctlfile == NULL && P->cepfile == NULL && P->wavfile == NULL) {
		E_FATAL("No control file given\n");
	}

	if (P->nchans > 1) {
		E_INFO("Files have %d channels of data\n", P->nchans);
		E_INFO("Will extract features for channel %d\n",
		       P->whichchan);
	}

	if (P->whichchan > P->nchans) {
		E_FATAL("You cannot select channel %d out of %d\n",
			P->whichchan, P->nchans);
	}

	if ((P->params.UPPER_FILT_FREQ * 2) > P->params.SAMPLING_RATE) {
		E_WARN("Upper frequency higher than Nyquist frequency\n");
	}

	if (P->params.doublebw == ON) {
		E_INFO("Will use double bandwidth filters\n");
	}

}


globals_t *
fe_parse_options(int32 argc, char **argv)
{
	globals_t *P;
	int32 format;
	char *endian;

	cmd_ln_parse(defn, argc, argv);

	if ((P = (globals_t *) calloc(1, sizeof(globals_t))) == NULL) {
		E_FATAL
		    ("memory alloc failed in fe_parse_options()\n...exiting\n");
	}

	fe_init_params(&P->params);
	P->nskip = P->runlen = -1;

	P->wavfile = cmd_ln_str("-i");
	if (P->wavfile != NULL) {
		P->is_single = ON;
	}

	P->cepfile = cmd_ln_str("-o");

	P->ctlfile = cmd_ln_str("-c");
	if (P->ctlfile != NULL) {
		char *nskip;
		char *runlen;

		P->is_batch = ON;

		nskip = cmd_ln_str("-nskip");
		runlen = cmd_ln_str("-runlen");
		if (nskip != NULL) {
			P->nskip = atoi(nskip);
		}
		if (runlen != NULL) {
			P->runlen = atoi(runlen);
		}
	}

	P->wavdir = cmd_ln_str("-di");
	P->cepdir = cmd_ln_str("-do");
	P->wavext = cmd_ln_str("-ei");
	P->cepext = cmd_ln_str("-eo");
	format = cmd_ln_int32("-raw");
	if (format) {
		P->input_format = RAW;
	}
	format = cmd_ln_int32("-nist");
	if (format) {
		P->input_format = NIST;
	}
	format = cmd_ln_int32("-mswav");
	if (format) {
		P->input_format = MSWAV;
	}

	P->nchans = cmd_ln_int32("-nchans");
	P->whichchan = cmd_ln_int32("-whichchan");
	P->params.PRE_EMPHASIS_ALPHA = cmd_ln_float32("-alpha");
	P->params.SAMPLING_RATE = cmd_ln_float32("-srate");
	P->params.WINDOW_LENGTH = cmd_ln_float32("-wlen");
	P->params.FRAME_RATE = cmd_ln_int32("-frate");
	if (!strcmp(cmd_ln_str("-feat"), "sphinx")) {
		P->params.FB_TYPE = MEL_SCALE;
		P->output_endian = BIG;
	}
	else {
		E_ERROR
		    ("MEL_SCALE IS CURRENTLY THE ONLY IMPLEMENTATION\n\n");
		E_FATAL("Make sure you specify '-feat sphinx'\n");
	}
	P->params.NUM_FILTERS = cmd_ln_int32("-nfilt");
	P->params.NUM_CEPSTRA = cmd_ln_int32("-ncep");
	P->params.LOWER_FILT_FREQ = cmd_ln_float32("-lowerf");
	P->params.UPPER_FILT_FREQ = cmd_ln_float32("-upperf");

	P->params.warp_type = cmd_ln_str("-warp_type");
	P->params.warp_params = cmd_ln_str("-warp_params");

	P->params.FFT_SIZE = cmd_ln_int32("-nfft");
	if (cmd_ln_int32("-doublebw")) {
		P->params.doublebw = ON;
	}
	else {
		P->params.doublebw = OFF;
	}
	P->blocksize = cmd_ln_int32("-blocksize");
	P->params.verbose = cmd_ln_int32("-verbose");
	endian = cmd_ln_str("-mach_endian");
	if (!strcmp("big", endian)) {
		P->machine_endian = BIG;
	}
	else {
		if (!strcmp("little", endian)) {
			P->machine_endian = LITTLE;
		}
		else {
			E_FATAL("Machine must be big or little Endian\n");
		}
	}
	endian = cmd_ln_str("-input_endian");
	if (!strcmp("big", endian)) {
		P->input_endian = BIG;
	}
	else {
		if (!strcmp("little", endian)) {
			P->input_endian = LITTLE;
		}
		else {
			E_FATAL("Input must be big or little Endian\n");
		}
	}
	P->params.dither = cmd_ln_int32("-dither");
	P->params.seed = cmd_ln_int32("-seed");
	P->params.logspec = cmd_ln_int32("-logspec");
	P->idct = cmd_ln_int32("-idct");

	fe_validate_parameters(P);

	return (P);

}


int32
fe_build_filenames(globals_t * P, char *fileroot, char **infilename,
		   char **outfilename)
{
	char cbuf[MAXCHARS];
	char chanlabel[MAXCHARS];

	if (P->nchans > 1)
		sprintf(chanlabel, ".ch%d", P->whichchan);

	if (P->is_batch) {
		sprintf(cbuf, "%s", "");
		strcat(cbuf, P->wavdir);
		strcat(cbuf, "/");
		strcat(cbuf, fileroot);
		strcat(cbuf, ".");
		strcat(cbuf, P->wavext);
		if (infilename != NULL) {
			*infilename = fe_copystr(*infilename, cbuf);
		}

		sprintf(cbuf, "%s", "");
		strcat(cbuf, P->cepdir);
		strcat(cbuf, "/");
		strcat(cbuf, fileroot);
		if (P->nchans > 1)
			strcat(cbuf, chanlabel);
		strcat(cbuf, ".");
		strcat(cbuf, P->cepext);
		if (outfilename != NULL) {
			*outfilename = fe_copystr(*outfilename, cbuf);
		}
	}
	else if (P->is_single) {
		sprintf(cbuf, "%s", "");
		strcat(cbuf, P->wavfile);
		if (infilename != NULL) {
			*infilename = fe_copystr(*infilename, cbuf);
		}

		sprintf(cbuf, "%s", "");
		strcat(cbuf, P->cepfile);
		if (outfilename != NULL) {
			*outfilename = fe_copystr(*outfilename, cbuf);
		}
	}
	else {
		E_FATAL("Unspecified Batch or Single Mode\n");
	}

	return 0;
}


char *
fe_copystr(char *dest_str, char *src_str)
{
	int i, src_len, len;
	char *s;

	src_len = strlen(src_str);
	len = src_len;
	s = (char *) malloc(len + 1);
	for (i = 0; i < src_len; i++)
		*(s + i) = *(src_str + i);
	*(s + src_len) = NULL_CHAR;

	return (s);
}

int32
fe_count_frames(fe_t * FE, int32 nsamps, int32 count_partial_frames)
{
	int32 frame_start, frame_count = 0;

	assert(FE->FRAME_SIZE != 0);
	for (frame_start = 0; frame_start + FE->FRAME_SIZE <= nsamps;
	     frame_start += FE->FRAME_SHIFT)
		frame_count++;

	/* dhuggins@cs, 2006-04-25: Update this to match the updated
	 * partial frame condition in fe_process_utt(). */
	if (count_partial_frames) {
		if (frame_count * FE->FRAME_SHIFT < nsamps)
			frame_count++;
	}

	return (frame_count);
}

int32
fe_openfiles(globals_t * P, fe_t * FE, char *infile, int32 * fp_in,
	     int32 * nsamps, int32 * nframes, int32 * nblocks,
	     char *outfile, int32 * fp_out)
{
	struct stat filestats;
	int fp = 0, len = 0, outlen, numframes, numblocks;
	FILE *fp2;
	char line[MAXCHARS];
	int got_it = 0;


	/* Note: this is kind of a hack to read the byte format from the
	   NIST header */
	if (P->input_format == NIST) {
		if ((fp2 = fopen(infile, "rb")) == NULL) {
			E_ERROR_SYSTEM("Cannot read %s", infile);
			return (FE_INPUT_FILE_READ_ERROR);
		}
		*line = 0;
		got_it = 0;
		while (strcmp(line, "end_head") && !got_it) {
			fscanf(fp2, "%s", line);
			if (!strcmp(line, "sample_byte_format")) {
				fscanf(fp2, "%s", line);
				if (!strcmp(line, "-s2")) {
					fscanf(fp2, "%s", line);
					if (!strcmp(line, "01")) {
						P->input_endian = LITTLE;
						got_it = 1;
					}
					else if (!strcmp(line, "10")) {
						P->input_endian = BIG;
						got_it = 1;
					}
					else
						E_ERROR
						    ("Unknown/unsupported byte order\n");
				}
				else
					E_ERROR
					    ("Error determining byte format\n");
			}
		}
		if (!got_it) {
			E_WARN
			    ("Can't find byte format in header, setting to machine's endian\n");
			P->input_endian = P->machine_endian;
		}
		fclose(fp2);
	}
	else if (P->input_format == RAW) {
		/*
		   P->input_endian = P->machine_endian;
		 */
	}
	else if (P->input_format == MSWAV) {
		P->input_endian = LITTLE;	// Default for MS WAV riff files
	}


	if ((fp = open(infile, O_RDONLY | O_BINARY, 0644)) < 0) {
		fprintf(stderr, "Cannot open %s\n", infile);
		return (FE_INPUT_FILE_OPEN_ERROR);
	}
	else {
		if (fstat(fp, &filestats) != 0)
			printf("fstat failed\n");

		if (P->input_format == NIST) {
			short *hdr_buf;

			len =
			    (filestats.st_size -
			     HEADER_BYTES) / sizeof(short);
			/* eat header */
			hdr_buf =
			    (short *) calloc(HEADER_BYTES / sizeof(short),
					     sizeof(short));
			if (read(fp, hdr_buf, HEADER_BYTES) !=
			    HEADER_BYTES) {
				E_ERROR("Cannot read %s\n", infile);
				return (FE_INPUT_FILE_READ_ERROR);
			}
			free(hdr_buf);
		}
		else if (P->input_format == RAW) {
			len = filestats.st_size / sizeof(int16);
		}
		else if (P->input_format == MSWAV) {
			/* Read the header */
			MSWAV_hdr *hdr_buf;
			if ((hdr_buf =
			     (MSWAV_hdr *) calloc(1,
						  sizeof(MSWAV_hdr))) ==
			    NULL) {
				E_ERROR
				    ("Cannot allocate for input file header\n");
				return (FE_INPUT_FILE_READ_ERROR);
			}
			if (read(fp, hdr_buf, sizeof(MSWAV_hdr)) !=
			    sizeof(MSWAV_hdr)) {
				E_ERROR
				    ("Cannot allocate for input file header\n");
				return (FE_INPUT_FILE_READ_ERROR);
			}
			/* Check header */
			if (strncmp(hdr_buf->rifftag, "RIFF", 4) != 0 ||
			    strncmp(hdr_buf->wavefmttag, "WAVEfmt",
				    7) != 0) {
				E_ERROR("Error in mswav file header\n");
				return (FE_INPUT_FILE_READ_ERROR);
			}
			if (strncmp(hdr_buf->datatag, "data", 4) != 0) {
				/* In this case, there are other "chunks" before the
				 * data chunk, which we can ignore. We have to find the
				 * start of the data chunk, which begins with the string
				 * "data".
				 */
				int16 found = OFF;
				char readChar;
				char *dataString = "data";
				int16 charPointer = 0;
				printf("LENGTH: %d\n", (int)strlen(dataString));
				while (found != ON) {
					if (read
					    (fp, &readChar,
					     sizeof(char)) !=
					    sizeof(char)) {
						E_ERROR
						    ("Failed reading wav file.\n");
						return
						    (FE_INPUT_FILE_READ_ERROR);
					}
					if (readChar ==
					    dataString[charPointer]) {
						charPointer++;
					}
					if (charPointer ==
					    (int) strlen(dataString)) {
						found = ON;
						strcpy(hdr_buf->datatag,
						       dataString);
						if (read
						    (fp,
						     &(hdr_buf->
						       datalength),
						     sizeof(int32)) !=
						    sizeof(int32)) {
							E_ERROR
							    ("Failed reading wav file.\n");
							return
							    (FE_INPUT_FILE_READ_ERROR);
						}
					}
				}
			}
			if (P->input_endian != P->machine_endian) {	// If machine is Big Endian
				hdr_buf->datalength =
				    SWAP_INT32(&(hdr_buf->datalength));
				hdr_buf->data_format =
				    SWAP_INT16(&(hdr_buf->data_format));
				hdr_buf->numchannels =
				    SWAP_INT16(&(hdr_buf->numchannels));
				hdr_buf->BitsPerSample =
				    SWAP_INT16(&(hdr_buf->BitsPerSample));
				hdr_buf->SamplingFreq =
				    SWAP_INT32(&(hdr_buf->SamplingFreq));
				hdr_buf->BytesPerSec =
				    SWAP_INT32(&(hdr_buf->BytesPerSec));
			}
			/* Check Format */
			if (hdr_buf->data_format != 1
			    || hdr_buf->BitsPerSample != 16) {
				E_ERROR
				    ("MS WAV file not in 16-bit PCM format\n");
				return (FE_INPUT_FILE_READ_ERROR);
			}
			len = hdr_buf->datalength / sizeof(short);
			P->nchans = hdr_buf->numchannels;
			/* DEBUG: Dump Info */
			if (P->params.verbose) {
				E_INFO("Reading MS Wav file %s:\n",
				       infile);
				E_INFO
				    ("\t16 bit PCM data, %d channels %d samples\n",
				     P->nchans, len);
				E_INFO("\tSampled at %d\n",
				       hdr_buf->SamplingFreq);
			}
			free(hdr_buf);
		}
		else {
			E_ERROR("Unknown input file format\n");
			return (FE_INPUT_FILE_OPEN_ERROR);
		}
	}


	len = len / P->nchans;
	*nsamps = len;
	*fp_in = fp;

	numblocks = (int) ((float) len / (float) P->blocksize);
	if (numblocks * P->blocksize < len)
		numblocks++;

	*nblocks = numblocks;

	if ((fp =
	     open(outfile, O_CREAT | O_WRONLY | O_TRUNC | O_BINARY,
		  0644)) < 0) {
		E_ERROR("Unable to open %s for writing features\n",
			outfile);
		return (FE_OUTPUT_FILE_OPEN_ERROR);
	}
	else {
		/* compute number of frames and write cepfile header */
		numframes = fe_count_frames(FE, len, COUNT_PARTIAL);
		if (P->params.logspec != ON)
			outlen = numframes * FE->NUM_CEPSTRA;
		else
			outlen = numframes * FE->MEL_FB->num_filters;
		if (P->output_endian != P->machine_endian)
			SWAP_INT32(&outlen);
		if (write(fp, &outlen, 4) != 4) {
			E_ERROR("Data write error on %s\n", outfile);
			close(fp);
			return (FE_OUTPUT_FILE_WRITE_ERROR);
		}
		if (P->output_endian != P->machine_endian)
			SWAP_INT32(&outlen);
	}

	*nframes = numframes;
	*fp_out = fp;

	return 0;
}

int32
fe_readblock_spch(globals_t * P, int32 fp, int32 nsamps, int16 * buf)
{
	int32 bytes_read, cum_bytes_read, nreadbytes, actsamps, offset, i,
	    j, k;
	int16 *tmpbuf;
	int32 nchans, whichchan;

	nchans = P->nchans;
	whichchan = P->whichchan;

	if (nchans == 1) {
		if (P->input_format == RAW || P->input_format == NIST
		    || P->input_format == MSWAV) {
			nreadbytes = nsamps * sizeof(int16);
			if ((bytes_read =
			     read(fp, buf, nreadbytes)) != nreadbytes) {
				E_ERROR("error reading block\n");
				return (0);
			}
		}
		else {
			E_ERROR("unknown input file format\n");
			return (0);
		}
		cum_bytes_read = bytes_read;
	}
	else if (nchans > 1) {

		if (nsamps < P->blocksize) {
			actsamps = nsamps * nchans;
			tmpbuf =
			    (int16 *) calloc(nsamps * nchans,
					     sizeof(int16));
			cum_bytes_read = 0;
			if (P->input_format == RAW
			    || P->input_format == NIST) {

				k = 0;
				nreadbytes = actsamps * sizeof(int16);

				if ((bytes_read =
				     read(fp, tmpbuf,
					  nreadbytes)) != nreadbytes) {
					E_ERROR
					    ("error reading block (got %d not %d)\n",
					     bytes_read, nreadbytes);
					return (0);
				}

				for (j = whichchan - 1; j < actsamps;
				     j = j + nchans) {
					buf[k] = tmpbuf[j];
					k++;
				}
				cum_bytes_read += bytes_read / nchans;
			}
			else {
				E_ERROR("unknown input file format\n");
				return (0);
			}
			free(tmpbuf);
		}
		else {
			tmpbuf = (int16 *) calloc(nsamps, sizeof(int16));
			actsamps = nsamps / nchans;
			cum_bytes_read = 0;

			if (actsamps * nchans != nsamps) {
				E_WARN
				    ("Blocksize %d is not an integer multiple of Number of channels %d\n",
				     nsamps, nchans);
			}

			if (P->input_format == RAW
			    || P->input_format == NIST) {
				for (i = 0; i < nchans; i++) {

					offset = i * actsamps;
					k = 0;
					nreadbytes =
					    nsamps * sizeof(int16);

					if ((bytes_read =
					     read(fp, tmpbuf,
						  nreadbytes)) !=
					    nreadbytes) {
						E_ERROR
						    ("error reading block (got %d not %d)\n",
						     bytes_read,
						     nreadbytes);
						return (0);
					}

					for (j = whichchan - 1; j < nsamps;
					     j = j + nchans) {
						buf[offset + k] =
						    tmpbuf[j];
						k++;
					}
					cum_bytes_read +=
					    bytes_read / nchans;
				}
			}
			else {
				E_ERROR("unknown input file format\n");
				return (0);
			}
			free(tmpbuf);
		}
	}

	else {
		E_ERROR("unknown number of channels!\n");
		return (0);
	}

	if (P->input_endian != P->machine_endian) {
		for (i = 0; i < nsamps; i++)
			SWAP_INT16(&buf[i]);
	}

	return (cum_bytes_read / sizeof(int16));

}

int32
fe_writeblock_feat(globals_t * P, fe_t * FE, int32 fp, int32 nframes,
		   mfcc_t ** feat)
{

	int32 i, length, nwritebytes;
	float32 **ffeat;

	if (P->params.logspec == ON)
		length = nframes * FE->MEL_FB->num_filters;
	else
		length = nframes * FE->NUM_CEPSTRA;

	ffeat = (float32 **) feat;
	fe_mfcc_to_float(FE, feat, ffeat, nframes);
	if (P->output_endian != P->machine_endian) {
		for (i = 0; i < length; ++i)
			SWAP_FLOAT32(ffeat[0] + i);
	}

	nwritebytes = length * sizeof(float32);
	if (write(fp, ffeat[0], nwritebytes) != nwritebytes) {
		close(fp);
		E_FATAL("Error writing block of features\n");
	}

	return (length);
}


int32
fe_closefiles(int32 fp_in, int32 fp_out)
{
	close(fp_in);
	close(fp_out);
	return 0;
}

int32
fe_convert_logspec(globals_t *P, fe_t *FE, char *infile, char *outfile)
{
	FILE *ifh, *ofh;
	int32 ifsize, ofsize;
	float32 *logspec;

	if ((ifh = fopen(infile, "rb")) == NULL) {
		E_ERROR_SYSTEM("Cannot read %s", infile);
		return (FE_INPUT_FILE_READ_ERROR);
	}
	if ((ofh = fopen(outfile, "wb")) == NULL) {
		E_ERROR_SYSTEM("Unable to open %s for writing features",
			       outfile);
		return (FE_OUTPUT_FILE_OPEN_ERROR);
	}

	fread(&ifsize, 4, 1, ifh);
	ofsize = ifsize / FE->MEL_FB->num_filters * FE->NUM_CEPSTRA;
	fwrite(&ofsize, 4, 1, ofh);
	logspec = ckd_calloc(FE->MEL_FB->num_filters, sizeof(*logspec));

	while (fread(logspec, 4, FE->MEL_FB->num_filters, ifh) == FE->MEL_FB->num_filters) {
		fe_logspec_to_mfcc(FE, logspec, logspec);
		if (fwrite(logspec, 4, FE->NUM_CEPSTRA, ofh) < FE->NUM_CEPSTRA) {
			E_ERROR_SYSTEM("Failed to write %d cepstra to %s",
				       FE->NUM_CEPSTRA, outfile);
			free(logspec);
			return (FE_OUTPUT_FILE_WRITE_ERROR);
		}
	}
	if (!feof(ifh)) {
		E_ERROR("Short read in input file %s\n", infile);
		free(logspec);
		return (FE_INPUT_FILE_READ_ERROR);
	}
	fclose(ifh);
	fclose(ofh);

	return FE_SUCCESS;
}

/*
 * Log record.  Maintained by RCS.
 *
 * $Log: wave2feat.c,v $
 * Revision 1.35  2006/02/25 00:53:48  egouvea
 * Added the flag "-seed". If dither is being used and the seed is less
 * than zero, the random number generator is initialized with time(). If
 * it is at least zero, it's initialized with the provided seed. This way
 * we have the benefit of having dither, and the benefit of being
 * repeatable.
 *
 * This is consistent with what sphinx3 does. Well, almost. The random
 * number generator is still what the compiler provides.
 *
 * Also, moved fe_init_params to fe_interface.c, so one can initialize a
 * variable of type param_t with meaningful values.
 *
 * Revision 1.34  2006/02/20 23:55:51  egouvea
 * Moved fe_dither() to the "library" side rather than the app side, so
 * the function can be code when using the front end as a library.
 *
 * Revision 1.33  2006/02/17 00:31:34  egouvea
 * Removed switch -melwarp. Changed the default for window length to
 * 0.025625 from 0.256 (so that a window at 16kHz sampling rate has
 * exactly 410 samples). Cleaned up include's. Replaced some E_FATAL()
 * with E_WARN() and return.
 *
 * Revision 1.32  2006/02/16 20:11:20  egouvea
 * Fixed the code that prints a warning if any zero-energy frames are
 * found, and recommending the user to add dither. Previously, it would
 * only report the zero energy frames if they happened in the last
 * utterance. Now, it reports for each utterance.
 *
 * Revision 1.31  2006/02/16 00:18:26  egouvea
 * Implemented flexible warping function. The user can specify at run
 * time which of several shapes they want to use. Currently implemented
 * are an affine function (y = ax + b), an inverse linear (y = a/x) and a
 * piecewise linear (y = ax, up to a frequency F, and then it "breaks" so
 * Nyquist frequency matches in both scales.
 *
 * Added two switches, -warp_type and -warp_params. The first specifies
 * the type, which valid values:
 *
 * -inverse or inverse_linear
 * -linear or affine
 * -piecewise or piecewise_linear
 *
 * The inverse_linear is the same as implemented by EHT. The -mel_warp
 * switch was kept for compatibility (maybe remove it in the
 * future?). The code is compatible with EHT's changes: cepstra created
 * from code after his changes should be the same as now. Scripts that
 * worked with his changes should work now without changes. Tested a few
 * cases, same results.
 *
 */
