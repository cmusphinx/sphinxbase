/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include <stdio.h>
#include <string.h>

#include "strfuncs.h"
#include "ckd_alloc.h"

int
main(int argc, char *argv[])
{
    if (argc < 2)
        return 1;

    if (!strcmp(argv[1], "string_join")) {
        char *foo = string_join("bar", "baz", "quux", NULL);
        if (strcmp(foo, "barbazquux") != 0) {
            printf("%s != barbazquux\n", foo);
            return 1;
        }
        foo = string_join("hello", NULL);
        if (strcmp(foo, "hello") != 0) {
            printf("%s != hello\n", foo);
            return 1;
        }
        return 0;
    }
    else if (!strcmp(argv[1], "str2words")) {
        char *line = strdup("    foo bar baz argh");
        char **words;
        int n;

        n = str2words(line, NULL, 0);
        if (n != 4) {
            printf("%d != 4\n", n);
            return 1;
        }
        words = ckd_calloc(n, sizeof(*words));
        n = str2words(line, words, n);
        if (n != 4) {
            printf("%d != 4\n", n);
            return 1;
        }
        if (strcmp(words[0], "foo") != 0
            || strcmp(words[1], "bar") != 0
            || strcmp(words[2], "baz") != 0
            || strcmp(words[3], "argh") != 0) {
            printf("%s, %s, %s, %s != foo, bar, baz, argh\n",
                   words[0], words[1], words[2], words[3]);
            return 1;
        }
        return 0;
    }
    else if (!strcmp(argv[1], "nextword")) {
        char *line = strdup(" \tfoo bar\nbaz argh");
        char *word;
        const char *delim = " \t\n";
        char delimfound;
        int n;

        n = nextword(line, delim, &word, &delimfound);
        if (strcmp(word, "foo") != 0) {
            printf("%s != foo\n", word);
            return 1;
        }
        if (delimfound != ' ') {
            printf("didn't find ' '\n");
            return 1;
        }
        word[n] = delimfound;
        line = word + n;
        n = nextword(line, delim, &word, &delimfound);
        if (strcmp(word, "bar") != 0) {
            printf("%s != bar\n", word);
            return 1;
        }
        if (delimfound != '\n') {
            printf("didn't find '\\n'\n");
            return 1;
        }
        word[n] = delimfound;
        line = word + n;
        n = nextword(line, delim, &word, &delimfound);
        if (strcmp(word, "baz") != 0) {
            printf("%s != baz\n", word);
            return 1;
        }
        if (delimfound != ' ') {
            printf("didn't find ' '\n");
            return 1;
        }
        word[n] = delimfound;
        line = word + n;
        n = nextword(line, delim, &word, &delimfound);
        if (strcmp(word, "argh") != 0) {
            printf("%s != argh\n", word);
            return 1;
        }
        if (delimfound != '\0') {
            printf("didn't find NUL\n");
            return 1;
        }

        return 0;
    }
    return 0;
}
