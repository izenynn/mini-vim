#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <termios.h>

struct termios g_org;

static void dis_raw_mode() {
	/* atexit() func., retore terminal's original attributes */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_org);
}

static void enb_raw_mode() {
	struct termios raw;

	/* Save terminal's original attributes */
	tcgetattr(STDIN_FILENO, &g_org);
	atexit(dis_raw_mode);

	/* Get terminal's attributes */
	tcgetattr(STDIN_FILENO, &raw);
	/* Bitwise flags to disable unwanted behaviurs */
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* Apply changes to terminal */
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
	char c;

	/* Change terminal to raw mode */
	enb_raw_mode();

	/* Read input until it finds a 'q' */
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		if (iscntrl(c)) printf("%d\n", c);
		else printf("%d ('%c')\n", c, c);
	}

	return (0);
}