CC = clang
CFLAGS = -Wall -Werror -Wextra -Wpedantic

all: 
	$(CC) $(CFLAGS) main.c -o markov

check: 
	valgrind --leak-check=full ./markov

clean:
	rm markov
