CFLAGS  = -Wall -Werror -pedantic -fsanitize=address,undefined
SRC     = main.c ../workq.c ../net.c ../chat.c
CC      = gcc

all: $(SRC)
	$(CC) $(CFLAGS) $^

fast:
	$(CC) -Wall -Werror -pedantic $(SRC)
