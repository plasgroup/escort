#include <unistd.h>
#include "../src/Escort.hpp"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "%s nvm-path\n", argv[0]);
        return 1;
    }
    escort_init(argv[1]);

    escort_thread_init();

    sleep(3);

    escort_thread_finalize();

    escort_finalize();

    return 0;
}