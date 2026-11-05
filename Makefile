CC = gcc
CFALGS = -Wextra -Wall -Werror -pedantic -std=23

edit: text-editor.c
	$(CC) $(CFLAGS) $< -o $@ 
clean:
	rm -f edit

