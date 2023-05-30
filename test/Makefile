FILES    = main.cpp list.cpp list_single.cpp list_multi.cpp list_sort.cpp
INCLUDES = test.hpp list.hpp


LDFLAGS = -L../jemalloc/lib -L../bin -L../ralloc/test -lescortdbg -lralloc -lpthread

CFLAGS = -O0 -g

TEST_PROGS = 01_init

all: $(TEST_PROGS)

$(TEST_PROGS):%:%.o
	g++ -o $@ $< $(LDFLAGS)

%.o:%.cpp
	g++ -o $@ -c $(CFLAGS) $<

main: $(FILES) $(INCLUDES)
	g++ -o main $(FILES) -lescort -L../bin -lpthread -DKILL_MODE
clean:
	rm -f main *.o
