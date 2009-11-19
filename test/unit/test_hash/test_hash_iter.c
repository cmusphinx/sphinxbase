/**
 * @file test_hash_iter.c Test hash table iterators
 * @author David Huggins-Daines <dhuggins@cs.cmu.edu>
 */

#include "hash_table.h"
#include "test_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	hash_table_t *h;
	hash_iter_t *itor;

	/* Test insertion */
	TEST_ASSERT(h = hash_table_new(42, FALSE));
	TEST_EQUAL((void*)0xdeadbeef, hash_table_enter(h, "foo", (void*)0xdeadbeef));
	TEST_EQUAL((void*)0xdeadbeef, hash_table_enter(h, "foo", (void*)0xd0d0feed));
	TEST_EQUAL((void*)0xcafec0de, hash_table_enter(h, "bar", (void*)0xcafec0de));
	TEST_EQUAL((void*)0xeeefeeef, hash_table_enter(h, "baz", (void*)0xeeefeeef));
	TEST_EQUAL((void*)0xbabababa, hash_table_enter(h, "quux", (void*)0xbabababa));

	/* Now test iterators. */
	for (itor = hash_table_iter(h); itor; itor = hash_table_iter_next(itor)) {
		printf("%s %p\n", itor->ent->key, itor->ent->val);
		if (0 == strcmp(itor->ent->key, "foo")) {
			TEST_EQUAL(itor->ent->val, (void*)0xdeadbeef);
		}
		else if (0 == strcmp(itor->ent->key, "bar")) {
			TEST_EQUAL(itor->ent->val, (void*)0xcafec0de);
		}
		else if (0 == strcmp(itor->ent->key, "baz")) {
			TEST_EQUAL(itor->ent->val, (void*)0xeeefeeef);
		}
		else if (0 == strcmp(itor->ent->key, "quux")) {
			TEST_EQUAL(itor->ent->val, (void*)0xbabababa);
		}
	}

	return 0;
}
