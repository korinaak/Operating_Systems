CC = gcc
CFLAGS = -Wall
OBJ = main.o child.o
TARGET = gates child

all: $(TARGET)

gates: main.o
	$(CC) $(CFLAGS) main.o -o gates

child: child.o
	$(CC) $(CFLAGS) child.o -o child

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

child.o: child.c
	$(CC) $(CFLAGS) -c child.c

clean:
	rm -f $(OBJ) $(TARGET)
