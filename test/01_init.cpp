#include "../src/Escort.hpp"

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "%s nvm-path\n", argv[0]);
        return 1;
    }
    escort_init(argv[1]);
    return 0;
}