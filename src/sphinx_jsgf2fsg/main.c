/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 2007 Carnegie Mellon University.  All rights
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

#include <hash_table.h>
#include <fsg_model.h>
#include <jsgf.h>
#include <err.h>

#include <string.h>
#include <strfuncs.h>

static fsg_model_t *
get_fsg(jsgf_t *grammar, const char *name)
{
    jsgf_rule_iter_t *itor;
    logmath_t *lmath = logmath_init(1.0001, 0, 0);
    fsg_model_t *fsg = NULL;

    for (itor = jsgf_rule_iter(grammar); itor;
         itor = jsgf_rule_iter_next(itor)) {
        jsgf_rule_t *rule = jsgf_rule_iter_rule(itor);
        char const *rule_name = jsgf_rule_name(rule);

        if ((name == NULL && jsgf_rule_public(rule))
            || (name && strlen(rule_name)-2 == strlen(name) &&
                0 == strncmp(rule_name + 1, name, strlen(rule_name) - 2))) {
            fsg = jsgf_build_fsg_raw(grammar, rule, logmath_retain(lmath), 1.0);
            jsgf_rule_iter_free(itor);
            break;
        }
    }

    logmath_free(lmath);
    return fsg;
}

int
main(int argc, char *argv[])
{
    jsgf_t *jsgf;
    fsg_model_t *fsg;
    int fsm = 0;
    char *outfile = NULL;
    char *symfile = NULL;

    if (argc > 1 && 0 == strcmp(argv[1], "-fsm")) {
        fsm = 1;
        ++argv;
    }

    jsgf = jsgf_parse_file(argc > 1 ? argv[1] : NULL, NULL);
    if (jsgf == NULL) {
        return 1;
    }
    fsg = get_fsg(jsgf, argc > 2 ? argv[2] : NULL);

    if (argc > 3)
        outfile = argv[3];
    if (argc > 4)
        symfile = argv[4];

    if (fsm) {
        if (outfile)
            fsg_model_writefile_fsm(fsg, outfile);
        else
            fsg_model_write_fsm(fsg, stdout);
        if (symfile)
            fsg_model_writefile_symtab(fsg, symfile);
    }
    else {
        if (outfile)
            fsg_model_writefile(fsg, outfile);
        else
            fsg_model_write(fsg, stdout);
    }
    fsg_model_free(fsg);
    jsgf_grammar_free(jsgf);

    return 0;
}


/** Silvio Moioli: Windows CE/Mobile entry point added. */
#if defined(_WIN32_WCE)
#pragma comment(linker,"/entry:mainWCRTStartup")
#include <windows.h>

//Windows Mobile has the Unicode main only
int wmain(int32 argc, wchar_t *wargv[]) {
    char** argv;
    size_t wlen;
    size_t len;
    int i;

    argv = malloc(argc*sizeof(char*));
    for (i=0; i<argc; i++){
        wlen = lstrlenW(wargv[i]);
        len = wcstombs(NULL, wargv[i], wlen);
        argv[i] = malloc(len+1);
        wcstombs(argv[i], wargv[i], wlen);
    }

    //assuming ASCII parameters
    return main(argc, argv);
}
#endif
