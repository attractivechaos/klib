#include <stdlib.h>
#include <stddef.h>
#include "klist_tests.h"

int main(void)
{
    int n_fail = 0;

    Suite *master_suite = suite_create("master_suite");

    Suite *suites[] = {
        klist_suite(),
        NULL
    };

    SRunner *sr = srunner_create(master_suite);

    for (Suite **pp = &suites[0]; *pp != NULL; pp++) {
        srunner_add_suite(sr, *pp);
    }

    srunner_run_all(sr, CK_NORMAL);
    n_fail += srunner_ntests_failed(sr);

    srunner_free(sr);

    return (n_fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
