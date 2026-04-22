CC = clang
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -g
LDFLAGS =

all: test_maylloc

test_maylloc: test_maylloc.o maylloc.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c maylloc.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: test_maylloc
	./test_maylloc

clean:
	rm -f *.o test_maylloc

.PHONY: all test clean
