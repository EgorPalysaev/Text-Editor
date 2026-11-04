CC = gcc
CFALGS = -Wextra -Wall -Werror -std=23
CLIBS = -lm

edit: text-editor.c
	$(CC) $(CFLAGS) $< -o $@ 
clean:
	rm -f edit

