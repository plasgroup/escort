FILES    = main.cpp list.cpp list_single.cpp list_multi.cpp list_sort.cpp
INCLUDES = test.hpp list.hpp


LDFLAGS = -L../jemalloc/lib -L../bin -L../ralloc/test -lescortdbg -lralloc -lpthread

CFLAGS = -O0 -g

TEST_PROGS = 01_init 02_threadinit 03_pnew 04_transaction 05_checkpoint 06_multithread

all: $(TEST_PROGS)

$(TEST_PROGS):%:%.o
	g++ -o $@ $< $(LDFLAGS)

%.o:%.cpp
	g++ -o $@ -c $(CFLAGS) $<

main: $(FILES) $(INCLUDES)
	g++ -o main $(FILES) -lescort -L../bin -lpthread -DKILL_MODE
clean:
	rm -f main *.o $(TEST_PROGS)
