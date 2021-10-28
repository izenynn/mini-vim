#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <termios.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios g_org;

#pragma region terminal

/* die func., print error and exit */
static void die(const char *s) {
	perror(s);
	exit (EXIT_FAILURE);
}

/* atexit(), disable raw mode on terminal and restore origin attributes */
static void dis_raw_mode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_org) == -1)
		die("tcsetattr");
}

/* Enable raw mode on terminal */
static void enb_raw_mode() {
	struct termios raw;

	/* Save terminal's original attributes */
	if (tcgetattr(STDIN_FILENO, &g_org) == -1)
		die("tcgetattr");
	atexit(dis_raw_mode);

	/* Get terminal's attributes */
	tcgetattr(STDIN_FILENO, &raw);
	/* Bitwise terminal's attribute flags */
	/* Old flags
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cflag |= (CS8);
	*/
	raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cflag &= ~(CSIZE | PARENB);
	raw.c_cflag |= ~(CS8);
	/* Timeout */
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	/* Apply changes to terminal */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");
}

#pragma endregion terminal

#pragma region init

/* main */
int main() {
	char c;

	/* Change terminal to raw mode */
	enb_raw_mode();

	/* Read input until it finds a 'q' */
	while (1) {
		c = '\0';
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
			die("read");
		if (iscntrl(c))
			printf("%d\r\n", c);
		else
			printf("%d ('%c')\r\n", c, c);
		if (c == CTRL_KEY('q'))
			break;
	}

	return (EXIT_SUCCESS);
}

#pragma endregion init