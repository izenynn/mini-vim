#include <minivim.h>

/* die func., refresh screen, print error and exit */
void die(const char *s) {
	/* clear screen and move cursor */
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	/* restore terminal */
	dis_raw_mode();

	/* print error and exit */
	perror(s);
	exit (EXIT_FAILURE);
}

/* atexit(), end ncurses, disable raw mode on terminal and restore origin attributes */
void dis_raw_mode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_e.org_termios) == -1)
		die("tcsetattr");
}

/* enable raw mode on terminal */
void enb_raw_mode() {
	struct termios raw;

	/* save terminal's original attributes */
	if (tcgetattr(STDIN_FILENO, &g_e.org_termios) == -1)
		die("tcgetattr");
	atexit(dis_raw_mode);

	/* get terminal's attributes */
	raw = g_e.org_termios;
	/* terminal flags */
	raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cflag &= ~(CSIZE | PARENB);
	raw.c_cflag |= ~(CS8);
	/* timeout */
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;
	/* apply changes to terminal */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");
}

/* read key */
int editor_read_key() {
	int nread;
	char c;

	/* read 1 byte */
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}

	/* keep reading if escape char is read */
	if (c == '\x1b') {
		char seq[3];

		/* read the rest of the sequence */
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return ('\x1b');
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return ('\x1b');

		/* get arrows */
		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return ('\x1b');
				if (seq[2] == '~') {
					if (seq[1] == '1') return (K_HOME);
					if (seq[1] == '3') return (K_DEL);
					if (seq[1] == '4') return (K_END);
					if (seq[1] == '5') return (K_PAGE_UP);
					if (seq[1] == '6') return (K_PAGE_DOWN);
					if (seq[1] == '7') return (K_HOME);
					if (seq[1] == '8') return (K_END);
				}
			}
			if (seq[1] == 'A') return (K_ARROW_UP);
			if (seq[1] == 'B') return (K_ARROW_DOWN);
			if (seq[1] == 'C') return (K_ARROW_RIGHT);
			if (seq[1] == 'D') return (K_ARROW_LEFT);
			if (seq[1] == 'H') return (K_HOME);
			if (seq[1] == 'F') return (K_END);
		} else if (seq[0] == '0') {
			if (seq[1] == 'H') return (K_HOME);
			if (seq[1] == 'F') return (K_END);
		}

		return ('\x1b');
	}

	return (c);
}

/* get cursor pos on terminal */
int get_cursor_pos(int *rows, int *cols) {
	char buff[32];
	unsigned int i;

	/* n command can be used to query the terminal for status info */
	/* the argument "6" ask for cursor position */
	/* this way we can read the reply from the standard input */
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		return (-1);

	/* read standard input into a buffer until 'R' is found */
	/* buff will be something like "<esc>30;120" (30 is the number of rows, and 120 of cols) */
	i = -1;
	while (++i < sizeof(buff) - 1) {
		if (read(STDIN_FILENO, &buff[i], 1) != 1)
			break;
		if (buff[i] == 'R')
			break;
	}
	buff[i] = '\0';

	/* make sure buff has an escape sequence */
	if (buff[0] != '\x1b' || buff[1] != '[')
		return (-1);
	/* parse buff into two integers: rows and cols */
	if (sscanf(&buff[2], "%d;%d", rows, cols) != 2)
		return (-1);

	return (0);
}

/* get terminal window size */
int get_windows_size(int *rows, int *cols) {
	/* <sys/ioctl.h> struct */
	struct winsize ws;

	/* get rows and cols using ioctl() (input output control) */
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		/* fallback method if ioctl fails */
		/* move cursor to bottom right corner */
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return (-1);
		/* return the fallback method */
		return (get_cursor_pos(rows, cols));
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return (0);
	}
}
