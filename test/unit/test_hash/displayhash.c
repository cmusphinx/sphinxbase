/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hash_table.h>
#include <err.h>

/* Insert -hmmdump, -lm, -svq4svq, -beam, -lminmemory into a hash and display it. */
int
main(int argc, char **argv)
{
    hash_table_t *ht;
    ht = hash_new(75, 0);

    if (hash_enter(ht, "-hmmdump", 1) != 1) {
        E_FATAL("Insertion of -hmmdump failed\n");
    }

    if (hash_enter(ht, "-svq4svq", 1) != 1) {
        E_FATAL("Insertion of -svq4svq failed\n");
    }

    if (hash_enter(ht, "-lm", 1) != 1) {
        E_FATAL("Insertion of -lm failed\n");
    }

    if (hash_enter(ht, "-beam", 1) != 1) {
        E_FATAL("Insertion of -beam failed\n");
    }

    if (hash_enter(ht, "-lminmemory", 1) != 1) {
        E_FATAL("Insertion of -lminmemory failed\n");
    }

    hash_display(ht, 1);

    hash_free(ht);
    ht = NULL;
    return 0;
}


#if 0
E_INFO("Hash table in the command line\n");
hash_display(ht, 1);

E_INFO("After deletion of -lm\n");
hash_delete(ht, "-lm");
hash_display(ht, 1);

E_INFO("After deletion of -lm\n");

hash_delete(ht, "-lm");
hash_display(ht, 1);

E_INFO("After deletion of -svq4svq\n");
hash_delete(ht, "-svq4svq");
hash_display(ht, 1);

E_INFO("After deletion of -beam\n");
hash_delete(ht, "-beam");
hash_display(ht, 1);
#endif
