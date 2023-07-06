#include <unistd.h>
#include "../src/Escort.hpp"

constexpr int ROOT_ID_ARRAY = 0;

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "%s nvm-path\n", argv[0]);
        return 1;
    }
    escort_init(argv[1]);

    escort_thread_init();

    escort_begin_op();

    int** a = *PNEW(int(*[10]));
    escort_set_root(ROOT_ID_ARRAY, a);

    for (int i = 0; i < 10; i++) {
        int* p = PNEW(int);
        printf("p = %p\n", p);
        a[i] = p;
    }
    escort_write_region(a, sizeof(int*[10]));

    escort_end_op();
    for (int j = 0; j < 1000000; j++) {
        escort_begin_op();

        for (int i = 0; i < 10; i++) {
            *a[i] = i;
            escort_write_region(a[i], sizeof(int));
        }

        escort_end_op();
    }

    escort_thread_finalize();

    escort_finalize();

    return 0;
}