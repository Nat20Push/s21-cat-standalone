CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -g

s21_cat: s21_cat.c
	$(CC) $(CFLAGS) s21_cat.c -o s21_cat

.PHONY: run test clean rebuild
run: s21_cat
	./s21_cat

test: s21_cat
	./test_cat.sh

clean:
	rm -f s21_cat

rebuild: clean s21_cat
