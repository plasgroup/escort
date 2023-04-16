FILES    = main.cpp list.cpp list_single.cpp list_multi.cpp list_sort.cpp
INCLUDES = test.hpp list.hpp

main: $(FILES) $(INCLUDES)
	g++ -o main $(FILES) -lescort -L../bin -lpthread -DKILL_MODE
clean:
	rm -f main
