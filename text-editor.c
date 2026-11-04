/*** includes ***/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <string.h>


/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define TEXT_EDITOR_VERSION "v.0.0.1"


/*** declarations ***/

int inputReadKey();

/*** data ***/

struct editorConfig {
    int cx, cy;
    int editorRows;
    int editorCols;
    struct termios __originalTermios;
} __UserConfig;


/*** terminal ***/

void die(const char *string) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(string);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &__UserConfig.__originalTermios) == -1) die("disableRawMode tcsetattr");
}
void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &__UserConfig.__originalTermios) == -1) die("enableRawMode tcgetattr");

    atexit(disableRawMode);

    struct termios __newTermios = __UserConfig.__originalTermios;
    __newTermios.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    __newTermios.c_oflag &= ~(OPOST);
    __newTermios.c_cflag |= (CS8);
    __newTermios.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    __newTermios.c_cc[VMIN] = 0;
    __newTermios.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &__newTermios) == -1) die("enableRawMode tcsetattr");
}

int getCursorPosition(int *editorRows, int *editorCols) {
    char buf[32];
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    for (unsigned char i = 0; i < sizeof(buf) - 1; i++) {
	if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
	if (buf[i] == 'R'){
	    buf[i] = '\0';
	    break;
	}
    }
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", editorRows, editorCols) != 2) return -1;

    return 0;
}

int getWindowSize(int *editorRows, int *editorCols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
	if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
	return getCursorPosition(editorRows, editorCols);
    } else {
	*editorRows = ws.ws_row;
	*editorCols = ws.ws_col;
	return 0;
    }
}


/*** appending buffer ***/

struct abuf {
    char *buf;
    size_t len;
};

#define ABUF_INIT { NULL, 0 }

void abAppend(struct abuf *abuf, char *new_s, size_t new_len) {
   char *new_buf = (char *)realloc(abuf->buf, abuf->len + new_len);

    if (new_buf != NULL) {
	memcpy(&new_buf[abuf->len], new_s, new_len);
	abuf->buf = new_buf;
	abuf->len += new_len;
    } else {
	free(abuf->buf);
	return;
    }
}

void abFree(struct abuf *abuf) {
    free(abuf->buf);
}


/*** input ***/

int inputReadKey() {
    signed char __readValue = 0;
    int __symbol = 0;
    while ((__readValue = read(STDIN_FILENO, &__symbol, 1)) != 1) {
	if (__readValue == -1 && errno != EAGAIN) die ("read");
    }
    return __symbol;
}

void editorMoveCursor(int key) {
    switch (key) {
	case CTRL_KEY('h'): 
	    if (__UserConfig.cx != 1) __UserConfig.cx--;
	    break;
	case CTRL_KEY('j'):
	    if (__UserConfig.cy != __UserConfig.editorRows) __UserConfig.cy++;
	    break;
	case CTRL_KEY('k'):
	    if (__UserConfig.cy != 1) __UserConfig.cy--;
	    break;
	case CTRL_KEY('l'):
	    if (__UserConfig.cx != __UserConfig.editorCols) __UserConfig.cx++;
	    break;
	case CTRL_KEY('u'): 
	    __UserConfig.cy = 1;
	    break;
	case CTRL_KEY('d'):
	    __UserConfig.cy = __UserConfig.editorRows;
	    break;
	case CTRL_KEY('f'):
	    __UserConfig.cx = 1;
	    break;
	case CTRL_KEY('g'):
	    __UserConfig.cx = __UserConfig.editorCols;
	    break;
    }
}


/*** output ***/

void editorDrawRows(struct abuf *abuf) {
    for (int y = 0; y < __UserConfig.editorRows; y++) {
	if (y == __UserConfig.editorRows / 3) {
	    char welcome[64];
	    int welcomelen = snprintf(welcome, sizeof welcome, "Text Editor %s", TEXT_EDITOR_VERSION); 
	    if (welcomelen > __UserConfig.editorCols) welcomelen = 0;
	    int padding = (__UserConfig.editorCols - welcomelen) / 2;
	    if (padding) {
		abAppend(abuf, "~", 1);
		padding--;
	    }
	    while (padding--) abAppend(abuf, " ", 1);
	    abAppend(abuf, welcome, welcomelen);
	} else {
	    abAppend(abuf, "~", 1);
	}
	if (y < __UserConfig.editorRows - 1) {
	    abAppend(abuf, "\r\n", 2);
	}
    }
}

void editorRefreshScrean() {
    struct abuf abuf = ABUF_INIT;

    abAppend(&abuf, "\x1b[?25l", 6);
    abAppend(&abuf, "\x1b[2J", 4);
    abAppend(&abuf, "\x1b[H", 3);

    editorDrawRows(&abuf);

    char buf_cursor[32];
    snprintf(buf_cursor, sizeof buf_cursor, "\x1b[%d;%dH", __UserConfig.cy, __UserConfig.cx);
    abAppend(&abuf, buf_cursor, strlen(buf_cursor));
    abAppend(&abuf, "\x1b[?25h", 6);
    write(STDOUT_FILENO, abuf.buf, abuf.len); 
    abFree(&abuf);
}

/*** mapping ***/

void editorProcessKeypress() {
    int __symbol = inputReadKey();

    switch (__symbol) {
	case CTRL_KEY('q'):
	    write(STDOUT_FILENO, "\x1b[2J", 4);
	    write(STDOUT_FILENO, "\x1b[H", 3);
	    exit(0);
	    break;
	case CTRL_KEY('u'): case CTRL_KEY('d'): case CTRL_KEY('f'): case CTRL_KEY('g'):
	case CTRL_KEY('h'): case CTRL_KEY('j'): case CTRL_KEY('k'): case CTRL_KEY('l'): editorMoveCursor(__symbol); break;
    }
}


/*** file i/o ***/

void openFile(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");
    fclose(fp);
}


/*** init ***/

void initEditor() {
    __UserConfig.cx = 1;
    __UserConfig.cy = 1;
    if (getWindowSize(&__UserConfig.editorRows, &__UserConfig.editorCols) == -1) die("initEditor_getWindowSize");
}

int main(int argc, char **argv){

    if (argc != 2) {
	printf("Input: ./edit <filename>\n");
	exit(1);
    }
    enableRawMode();
    initEditor();
    openFile(argv[1]);

    while(1) {
	editorRefreshScrean();
	editorProcessKeypress();
    }
    return 0;
}
