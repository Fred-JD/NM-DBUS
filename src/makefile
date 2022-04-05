CFLAGS = -O
CC = g++
SRC = main.cpp nm_class.cpp
OBJ = $(SRC:.cpp = .o)

NumTest: $(OBJ)
	$(CC) $(CFLAGS) -o output $(OBJ) `pkg-config --cflags --libs libnm`
clean:
	rm -f core *.o
