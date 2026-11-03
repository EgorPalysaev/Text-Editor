/*** includes ***/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>


/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)


/*** data ***/

struct editorConfig {
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
    __newTermios.c_cc[VTIME] = 10;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &__newTermios) == -1) die("enableRawMode tcsetattr");
}

int getWindowSize(int *tty_rows, int *tty_columns) {
    struct winsize __user_tty;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &__user_tty) == -1 || __user_tty.ws_col == 0) {
	return -1;
    } else {
	*tty_rows = ws_row;
	*tty_columns = ws_col;
	return 0;
    }
}


/*** input ***/

char inputReadKey() {
    signed char __readValue = 0;
    char __symbol = 0;
    while ((__readValue = read(STDIN_FILENO, &__symbol, 1)) != 1) {
	if (__readValue == -1 && errno != EAGAIN) die ("read");
    }
    return __symbol;
}


/*** output ***/

void editorDrawRows() {
    for (int y = 0; y < __UserConfig.tty_rows; y++) {
	write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScrean() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** mapping ***/

void editorProcessKeypress() {
    char __symbol = inputReadKey();

    switch (__symbol) {
	case CTRL_KEY('q'):
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	exit(0);
	break;
    }
}


/*** init ***/

void initEditor() {
    if (getWindowSize(&__UserConfig.tty_rows, &__UserConfig.tty_columns) == -1) die("initEditor_getWindowSize");
}

int main(int argc, char **argv){

    enableRawMode();
    initEditor();

    while(1) {
	editorRefreshScrean();
	editorProcessKeypress();
    }
    return 0;
}
