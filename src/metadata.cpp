#include <assert.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "metadata.hpp"

int Escort::PersistentMetaData::open(const char* filename)
{
    assert(map_addr == 0);
    fd = ::open(filename, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    struct stat64 st;
    fstat64(fd, &st);
    map_size = st.st_size;
    void* map_ptr = mmap(NULL, map_size, PROT_WRITE | PROT_READ, MAP_SHARED_VALIDATE | MAP_SYNC, fd, 0);
    if (map_ptr == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    map_addr = reinterpret_cast<uintptr_t>(map_ptr);
    printf("PMETA %p\n", (void*) map_addr);
    return 0;
}

void Escort::PersistentMetaData::close() {
    assert(map_addr != 0);
    ::close(fd);
    fd = -1;
    map_addr = 0;
    map_size = 0;
}

void Escort::PersistentMetaData::init(void* dram_base) {
    set_dram_base(dram_base, false);
    set_epoch(-1, false);
    set_magic(MAGIC, false);
    _mm_sfence();
}