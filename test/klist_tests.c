#include "klist_tests.h"

/*
 * Definitions
 */
#define __int_free(x)
KLIST_INIT(32, int, __int_free)

/*
 * Tests
 */
START_TEST (init)
{
    klist_t(32) *list = kl_init(32);
    ck_assert_ptr_ne(list, 0);
    kl_destroy(32, list);
}
END_TEST

static klist_t(32) *g_list;

void empty_setup(void)
{
    g_list = kl_init(32);
}

void empty_teardown(void)
{
    kl_destroy(32, g_list);
    g_list = NULL;
}

START_TEST (empty_size)
{
    ck_assert_int_eq(kl_length(g_list), 0);
}
END_TEST

START_TEST (empty_shift)
{
    int d = 42;
    int result = kl_shift(32, g_list, &d);
    ck_assert_int_eq(result, -1);
    ck_assert_int_eq(kl_length(g_list), 0);
    ck_assert_int_eq(d, 42);
}
END_TEST

START_TEST (empty_unshift)
{
    *kl_unshiftp(32, g_list) = 22;
    ck_assert_int_eq(kl_length(g_list), 1);

    int val = kl_val(kl_begin(g_list));
    ck_assert_int_eq(val, 22);
}
END_TEST

START_TEST (empty_push)
{
    *kl_pushp(32, g_list) = 22;
    ck_assert_int_eq(kl_length(g_list), 1);

    int val = kl_val(kl_begin(g_list));
    ck_assert_int_eq(val, 22);
}
END_TEST

START_TEST (empty_begin_end)
{
    ck_assert_ptr_eq(kl_begin(g_list), kl_end(g_list));
}
END_TEST

Suite* klist_suite(void)
{
    Suite *s = suite_create("klist");

    TCase *tc_core = tcase_create("init");
    tcase_add_test(tc_core, init);
    suite_add_tcase(s, tc_core);

    TCase *tc_empty = tcase_create("empty");
    tcase_add_checked_fixture(tc_empty, empty_setup, empty_teardown);
    tcase_add_test(tc_empty, empty_size);
    tcase_add_test(tc_empty, empty_shift);
    tcase_add_test(tc_empty, empty_unshift);
    tcase_add_test(tc_empty, empty_push);
    tcase_add_test(tc_empty, empty_begin_end);
    suite_add_tcase(s, tc_empty);

    return s;
}