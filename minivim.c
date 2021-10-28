#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define MINIVIM_VER "0.1"
#define TAB_SIZE 4

#define CTRL_KEY(k) ((k) & 0x1f)
#define APBUFF_INIT {NULL, 0}

/*** data ***/
#pragma region data

/* keys code enum */
enum editor_key {
	K_ARROW_UP = 1000,
	K_ARROW_DOWN,
	K_ARROW_LEFT,
	K_ARROW_RIGHT,
	K_DEL,
	K_HOME,
	K_END,
	K_PAGE_UP,
	K_PAGE_DOWN
};

/* editor row struct */
typedef struct e_row {
	int sz;
	int r_sz;
	char *line;
	char *rend;
} e_row;

/* editor config struct */
struct editor_conf {
	int cx, cy;
	int rx;
	int y_off;
	int x_off;
	int scrn_rows;
	int scrn_cols;
	int n_rows;
	e_row *row;
	struct termios org_termios;
};

/* append buff struct */
struct apbuff {
	char *buff;
	size_t len;
};

/* editor_conf global var */
struct editor_conf g_e;

#pragma endregion data

/*** terminal ***/
#pragma region terminal

/* die func., refresh screen, print error and exit */
static void die(const char *s) {
	/* clear screen and move cursor */
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	exit (EXIT_FAILURE);
}

/* atexit(), end ncurses, disable raw mode on terminal and restore origin attributes */
static void dis_raw_mode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_e.org_termios) == -1)
		die("tcsetattr");
}

/* enable raw mode on terminal */
static void enb_raw_mode() {
	struct termios raw;

	/* save terminal's original attributes */
	if (tcgetattr(STDIN_FILENO, &g_e.org_termios) == -1)
		die("tcgetattr");
	atexit(dis_raw_mode);

	/* get terminal's attributes */
	raw = g_e.org_termios;
	/* bitwise terminal's attribute flags */
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
static int editor_read_key() {
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

static int get_cursor_pos(int *rows, int *cols) {
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
static int get_windows_size(int *rows, int *cols) {
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

#pragma endregion terminal

/*** row operations ***/
#pragma region row_ops

/* calculate rx */
static int editor_row_cx_to_rx(e_row *row, int cx) {
	int rx = 0;
	int i;

	/* iterate line and calculate len with tabs included */
	for (i = 0; i < cx; i++) {
		if (row->line[i] == '\t')
			rx += (TAB_SIZE - 1) - (rx % TAB_SIZE);
		rx++;
	}

	return (rx);
}

/* update row */
static void editor_update_row(e_row *row) {
	int tabs = 0;
	int i;

	/* count tabs */
	for (i = 0; i < row->sz; i++) {
		if (row->line[i] == '\t') tabs++;
	}
	/* free old rend and allocate the new one */
	free(row->rend);
	row->rend = (char *)malloc(row->sz + tabs * (TAB_SIZE - 1) + 1);

	/* copy line chars to rend and handle tabs */
	int idx = 0;
	for (i = 0; i < row->sz; i++) {
		if (row->line[i] == '\t') {
			row->rend[idx++] = ' ';
			while (idx % TAB_SIZE)
				row->rend[idx++] = ' ';
		} else {
			row->rend[idx++] = row->line[i];
		}
	}
	/* set '\0' at end of string and set render size */
	row->rend[idx] = '\0';
	row->r_sz = idx;
}

/* append row */
static void editor_append_row(char *s, size_t len) {
	/* realloc e_row struct to allocate all rows */
	g_e.row = (e_row *)realloc(g_e.row, sizeof(e_row) * (g_e.n_rows + 1));
	if (!g_e.row)
		die("realloc");

	/* append row */
	int y = g_e.n_rows;
	g_e.row[y].sz = len;
	g_e.row[y].line = (char *)malloc(len + 1);
	memcpy(g_e.row[y].line, s, len);
	g_e.row[y].line[len] = '\0';

	/* initialise render */
	g_e.row[y].r_sz = 0;
	g_e.row[y].rend = NULL;
	editor_update_row(&g_e.row[y]);

	/* increase number of rows */
	g_e.n_rows++;
}

#pragma endregion row_ops

/*** file i/o ***/
#pragma region file_io

static void editor_open(const char *filename) {
	/* open file */
	FILE *fp = fopen(filename, "r");
	if (!fp)
		die("fopen");

	/* read file */
	char *line = NULL;
	size_t linecap = 0;
	ssize_t line_l;
	/* read lines */
	while ((line_l = getline(&line, &linecap, fp)) != -1) {
		/* remove new line characters from the line (already added between rows) */
		while (line_l > 0 && (line[line_l - 1] == '\r' || line[line_l - 1] == '\n'))
			line_l--;
		/* append row */
		editor_append_row(line, line_l);
	}

	/* free line and close file */
	free(line);
	fclose(fp);
}

#pragma endregion file_io

/*** append buffer ***/
#pragma region apbuff

/* append new string to struct apbuff */
static void apbuff_append(struct apbuff *ab, const char *s, int len) {
	char *new;

	/* realloc out to make sure it can store the string we will append */
	new = (char *)realloc(ab->buff, ab->len + len);
	if (!new)
		return;
	/* append string s to the new one */
	memcpy(&new[ab->len], s, len);
	/* change ab->buff to the new string and update len */
	ab->buff = new;
	ab->len += len;
}

/* free an apbuff struct */
static void apbuff_free(struct apbuff *ab) {
	free(ab->buff);
}

#pragma endregion apbuff

/*** output ***/
#pragma region output

/* check if we can scroll */
static void editor_scroll() {
	/* get rx */
	g_e.rx = 0;
	if (g_e.cy < g_e.n_rows)
		g_e.rx = editor_row_cx_to_rx(&g_e.row[g_e.cy], g_e.cx);

	/* handle vertical scroll */
	if (g_e.cy < g_e.y_off)
		g_e.y_off = g_e.cy;
	if (g_e.cy >= g_e.y_off + g_e.scrn_rows)
		g_e.y_off = g_e.cy - g_e.scrn_rows + 1;
	/* handle horizontal scroll */
	if (g_e.rx < g_e.x_off)
		g_e.x_off = g_e.rx;
	if (g_e.rx >= g_e.x_off + g_e.scrn_cols)
		g_e.x_off = g_e.rx - g_e.scrn_cols + 1;
}

/* draw rows on editor */
static void editor_draw_rows(struct apbuff *ab) {
	int y;

	/* iterate rows and draw lines */
	for (y = 0; y < g_e.scrn_rows; y++) {
		/* get file row where we want to start */
		int f_row = y + g_e.y_off;
		/* if no rows to print */
		if (f_row >= g_e.n_rows) {
			/* if total rows == 0 means no argument, so print welcome msg */
			if (g_e.n_rows == 0 && y == g_e.scrn_rows / 3) {
				/* welcome message */
				char welcome[32];
				int welcome_l = snprintf(welcome, sizeof(welcome), "minivim - ver %s", MINIVIM_VER);
				if (welcome_l > g_e.scrn_cols)
					welcome_l = g_e.scrn_cols;
				int padding = (g_e.scrn_cols - welcome_l) / 2;
				if (padding) {
					apbuff_append(ab, "~", 1);
					padding--;
				}
				while (padding--)
					apbuff_append(ab, " ", 1);
				apbuff_append(ab, welcome, welcome_l);
			/* there is a document but no more rows to print */
			} else {
				/* if nothing to draw */
				apbuff_append(ab, "~", 1);
			}
		/* rows to print */
		} else {
			int len = g_e.row[f_row].r_sz - g_e.x_off;
			if (len < 0) len = 0;
			if (len > g_e.scrn_cols) len = g_e.scrn_cols;
			apbuff_append(ab, &g_e.row[f_row].rend[g_e.x_off], len);
		}

		/* erase part of the line to the right of the cursor */
		apbuff_append(ab, "\x1b[K", 3);
		/* dont write a new line on the last row */
		if (y < g_e.scrn_rows - 1)
			apbuff_append(ab, "\r\n", 2);
	}
}

/* refresh editor screen */
static void editor_refresh_screen() {
	/* handle vertical scroll */
	editor_scroll();

	/* initialise struct */
	struct apbuff ab = APBUFF_INIT;

	/* hide cursor when drawing on screen */
	apbuff_append(&ab, "\x1b[?25l", 6);
	/* move cursor */
	apbuff_append(&ab, "\x1b[H", 3);

	/* draw rows */
	editor_draw_rows(&ab);

	/* get cursor pos and move cursor */
	char buff[32];
	snprintf(buff, sizeof(buff), "\x1b[%d;%dH", (g_e.cy - g_e.y_off) + 1, (g_e.rx - g_e.x_off) + 1);
	apbuff_append(&ab, buff, strlen(buff));

	/* show cursor again (finish drawing) */
	apbuff_append(&ab, "\x1b[?25h", 6);

	/* write buffer on screen */
	write(STDOUT_FILENO, ab.buff, ab.len);

	/* free buffer */
	apbuff_free(&ab);
}

#pragma endregion output

/*** input ***/
#pragma region input

/* process movement keys */
static void editor_move_cursor(int key) {
	e_row *row = (g_e.cy >= g_e.n_rows) ? NULL : &g_e.row[g_e.cy];

	if (key == K_ARROW_UP) {
		if (g_e.cy != 0) g_e.cy--;
	}
	else if (key == K_ARROW_DOWN) {
		if (g_e.cy < g_e.n_rows) g_e.cy++;
	}
	else if (key == K_ARROW_LEFT) {
		if (g_e.cx != 0) g_e.cx--;
	}
	else if (key == K_ARROW_RIGHT) {
		if (row && g_e.cx < row->sz)
			g_e.cx++;
	}

	row = (g_e.cy >= g_e.n_rows) ? NULL : &g_e.row[g_e.cy];
	int row_l = row ? row->sz : 0;
	if (g_e.cx > row_l)
		g_e.cx = row_l;
}

/* process key press */
static void editor_process_keypress() {
	int key;

	key = editor_read_key();

	if (key == CTRL_KEY('q')) {
		/* clear screen and move cursor before exit */
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(EXIT_SUCCESS);
	} else if (key == K_HOME) {
		g_e.cx = 0;
	} else if (key == K_END) {
		if (g_e.cy < g_e.n_rows)
			g_e.cx = g_e.row[g_e.cy].sz;
	} else if (key == K_PAGE_UP || key == K_PAGE_DOWN) {
		/* positionate cursor before moving a page */
		if (key == K_PAGE_UP) {
			g_e.cy = g_e.y_off;
		} else {
			g_e.cy = g_e.y_off + g_e.scrn_rows - 1;
			if (g_e.cy > g_e.n_rows) g_e.cy = g_e.n_rows;
		}
		/* move cursor a page */
		int cnt = g_e.scrn_rows;
		while (cnt--)
			editor_move_cursor(key == K_PAGE_UP ? K_ARROW_UP : K_ARROW_DOWN);
	} else if (key == K_ARROW_UP || key == K_ARROW_DOWN || key == K_ARROW_LEFT || key == K_ARROW_RIGHT) {
		editor_move_cursor(key);
	}
}

#pragma endregion input

/*** init ***/
#pragma region init

/* initialise editor */
static void init_editor() {
	/* initisalise vars */
	g_e.cx = 0;
	g_e.cy = 0;
	g_e.rx = 0;
	g_e.y_off = 0;
	g_e.x_off = 0;
	g_e.n_rows = 0;
	g_e.row = NULL;

	/* get window size */
	if (get_windows_size(&g_e.scrn_rows, &g_e.scrn_cols) == -1)
		die("get_window_size");
}

/* main */
int main(int argc, char *argv[]) {
	/* change terminal to raw mode */
	enb_raw_mode();

	/* initialise editor */
	init_editor();

	/* open file in editor */
	if (argc >= 2)
		editor_open(argv[1]);

	/* program loop */
	while (1) {
		editor_refresh_screen();
		editor_process_keypress();
	}

	return (EXIT_SUCCESS);
}

#pragma endregion init
