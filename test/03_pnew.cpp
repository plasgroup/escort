#include <unistd.h>
#include "../src/Escort.hpp"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "%s nvm-path\n", argv[0]);
        return 1;
    }
    escort_init(argv[1]);

    escort_thread_init();

    for (int i = 0; i < 10; i++) {
        int* p = PNEW(int);
        printf("p = %p\n", p);
    }

    escort_thread_finalize();

    escort_finalize();

    return 0;
}