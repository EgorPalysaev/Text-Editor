CC = gcc
CFALGS = -Wextra -Wall -Werror -std=99
CLIBS = -lm

edit: text-editor.c
	$(CC) $(CFLAGS) $< -o $@ 
clean:
	rm -f edit

