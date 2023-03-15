FILES    = main.cpp list.cpp list_single.cpp list_multi.cpp list_sort.cpp
INCLUDES = test.hpp list.hpp

main: $(FILES) $(INCLUDES)
	g++ -o main $(FILES) -ljemalloc -L../src/jemalloc/lib -lescort -L../bin -lpthread
clean:
	rm -f main
