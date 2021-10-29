#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define MINIVIM_VER "0.1"
#define TAB_SIZE 4

#define CTRL_KEY(k) ((k) & 0x1f)
#define APBUFF_INIT {NULL, 0}

#define HL_HL_NBR (1<<0)
#define HL_HL_STR (1<<1)

/* keys code enum */
enum editor_key {
	K_BACKSPACE = 127,
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

/* highlight */
enum editor_hl {
	HL_NORMAL = 0,
	HL_COMMENT,
	HL_MLCOMMENT,
	HL_KEYWORD1,
	HL_KEYWORD2,
	HL_STRING,
	HL_NUMBER,
	HL_MATCH
};

/*** data ***/
#pragma region data

/* editor syntax data struct */
struct e_syntax {
	char *f_type;
	char **f_match;
	char **keywords;
	char *oneline_comment;
	char *ml_comment_st;
	char *ml_comment_end;
	int flags;
};

/* editor row struct */
typedef struct e_row {
	int idx;
	int sz;
	int r_sz;
	char *line;
	char *rend;
	unsigned char *hl;
	int hl_open_comment;
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
	int dirty;
	char *filename;
	char status_msg[80];
	struct e_syntax *syntax;
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

/*** filetypes ***/
#pragma region filetypes

char *C_HL_extensions[] = { ".c", ".h", ".cpp", ".hpp", ".cc", NULL};

char *C_HL_keywords[] = {
	/* C Keywords */
	"auto","break","case","continue","default","do","else","enum",
	"extern","for","goto","if","register","return","sizeof","static",
	"struct","switch","typedef","union","volatile","while","NULL",

	/* C++ Keywords */
	"alignas","alignof","and","and_eq","asm","bitand","bitor","class",
	"compl","constexpr","const_cast","deltype","delete","dynamic_cast",
	"explicit","export","false","friend","inline","mutable","namespace",
	"new","noexcept","not","not_eq","nullptr","operator","or","or_eq",
	"private","protected","public","reinterpret_cast","static_assert",
	"static_cast","template","this","thread_local","throw","true","try",
	"typeid","typename","virtual","xor","xor_eq",

	/* C types */
	"int|","long|","double|","float|","char|","unsigned|","signed|",
	"void|","short|","auto|","const|","bool|",NULL
};

struct e_syntax HLDB[] = {
	{
		"c",
		C_HL_extensions,
		C_HL_keywords,
		"//", "/*", "*/",
		HL_HL_NBR | HL_HL_STR
	},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

#pragma endregion filetypes

/*** prototypes ***/
#pragma region prototypes

static void editor_set_status_msg(const char *format, ...);
static void editor_refresh_screen();
static char *editor_prompt(char *prompt, void (*callback)(char *, int));

#pragma endregion prototypes

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

/*** syntax highlighting ***/
#pragma region hl

/* return true if char is a separator */
static int is_separator(int c) {
	return (isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL);
}

/* set row syntax */
static void editor_update_syntax(e_row *row) {
	/* realloc memory por hl array */
	row->hl = (unsigned char *)realloc(row->hl, row->r_sz);
	/* set all array to normal hl */
	memset(row->hl, HL_NORMAL, row->r_sz);

	/* not update syntax if no fily type is detected */
	if (g_e.syntax == NULL) return;

	/* keywords alias */
	char **keywords = g_e.syntax->keywords;

	/* initialise comment */
	char *olc = g_e.syntax->oneline_comment;
	char *mlcs = g_e.syntax->ml_comment_st;
	char *mlce = g_e.syntax->ml_comment_end;

	/* check if active comment */
	int olc_l = olc ? strlen(olc) : 0;
	int mlcs_l = mlcs ? strlen(mlcs) : 0;
	int mlce_l = mlce ? strlen(mlce) : 0;

	/* save if previus character was a separator (def: 1) */
	int prev_sep = 1;
	/* save if we are in a string (def: 0) */
	int in_str = 0;
	/* check if previus lines was a multiline comment with no end */
	int in_comment = (row->idx > 0 && g_e.row[row->idx - 1].hl_open_comment);

	int i = 0;
	while (i < row->r_sz) {
		/* get current char */
		char c = row->rend[i];
		/* get previus hl */
		unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

		/* hl one line comment */
		if (olc_l && !in_str && !in_comment) {
			if (!strncmp(&row->rend[i], olc, olc_l)) {
				memset(&row->hl[i], HL_COMMENT, row->r_sz - i);
				break;
			}
		}

		/* hl multi line comments */
		if (mlcs_l && mlce_l && !in_str) {
			/* if we are in a comment, comment until multicomment end */
			if (in_comment) {
				row->hl[i] = HL_MLCOMMENT;
				if (!strncmp(&row->rend[i], mlce, mlce_l)) {
					memset(&row->hl[i], HL_MLCOMMENT, mlce_l);
					i += mlce_l;
					in_comment = 0;
					prev_sep = 1;
					continue;
				} else {
					i++;
					continue;
				}
			/* else check if multicomment starts */
			} else if (!strncmp(&row->rend[i], mlcs, mlcs_l)) {
				memset(&row->hl[i], HL_MLCOMMENT, mlcs_l);
				i += mlcs_l;
				in_comment = 1;
				continue;
			}
		}

		/* hl strings */
		if (g_e.syntax->flags & HL_HL_STR) {
			/* if we already are in a str */
			if (in_str) {
				/* change hl */
				row->hl[i] = HL_STRING;
				/* if we find \ skip two chars just in case the second is a " or ' */
				if (c == '\\' && i + 1 < row->r_sz) {
					row->hl[i + 1] = HL_STRING;
					i += 2;
					continue;
				}
				/* if we find " or ' again set in_str to 0 (we are out of the str) */
				if (c == in_str) in_str = 0;
				/* set prev_sep to true and continue looping */
				i++;
				prev_sep = 1;
				continue;
			/* if we are not in a str */
			} else {
				/* if we find a start str character */
				if (c == '"' || c == '\'') {
					/* set in_str to the char (" or ') */
					in_str = c;
					/* change hl */
					row->hl[i] = HL_STRING;
					/* continue looping */
					i++;
					continue;
				}
			}
		}

		/* hl numbers */
		if (g_e.syntax->flags & HL_HL_NBR) {
			if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || (c == '.' && prev_hl == HL_NUMBER)) {
				row->hl[i] = HL_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}

		/* hl keywords */
		/* check is preceeded by separator */
		if (prev_sep) {
			int j;
			/* loop keywords and check */
			for (j = 0; keywords[j]; j++) {
				/* get len of keyword */
				int k_len = strlen(keywords[j]);
				/* get if it is a secondary keyword '|' (and remove '|' from k_len if so) */
				int kw2 = keywords[j][k_len - 1] == '|';
				if (kw2) k_len--;

				/* check if it is a keyword and ends in separator */
				if (!strncmp(&row->rend[i], keywords[j], k_len) && is_separator(row->rend[i + k_len])) {
					/* hl keyword and increase i*/
					memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, k_len);
					i += k_len;
					/* break for */
					break;
				}
			}
			/* check if for was broken (or forced) out by checking the terminating NULL value */
			if (keywords[j] != NULL) {
				prev_sep = 0;
				continue;
			}
		}

		/* set prev_sep */
		prev_sep = is_separator(c);

		i++;
	}

	/* set value of hl_open_comment to in_comment state */
	int changed = (row->hl_open_comment != in_comment);
	row->hl_open_comment = in_comment;
	/* update syntax of all lines in case multiline comment */
	if (changed && row->idx + 1 < g_e.n_rows)
		editor_update_syntax(&g_e.row[row->idx + 1]);
}

/* handle colors */
static int editor_syntax_to_color(int hl) {
	if (hl == HL_COMMENT || hl == HL_MLCOMMENT) return (36);
	if (hl == HL_KEYWORD1) return (33);
	if (hl == HL_KEYWORD2) return (32);
	if (hl == HL_STRING) return (35);
	if (hl == HL_NUMBER) return (31);
	if (hl == HL_MATCH) return (34);
	else return (37);
}

/* select syntax hl */
static void editor_select_syntax_hl() {
	/* initialise syntax to null */
	g_e.syntax = NULL;

	/* return if there is no file name yet */
	if (g_e.filename == NULL) return;

	/* get last '.' in string */
	char *ext = strrchr(g_e.filename, '.');

	/* loop editor syntax struct in the HLDB array */
	for (unsigned int i = 0; i < HLDB_ENTRIES; i++) {
		struct e_syntax *s = &HLDB[i];
		unsigned int j = 0;
		/* loop every pattern */
		while (s->f_match[j]) {
			int is_ext = (s->f_match[j][0] == '.');
			/* check if match pattern */
			if ((is_ext && ext && !strcmp(ext, s->f_match[j])) || (!is_ext && strstr(g_e.filename, s->f_match[j]))) {
				/* set syntax */
				g_e.syntax = s;

				/* update syntax */
				int f_row;
				for (f_row = 0; f_row < g_e.n_rows; f_row++) {
					editor_update_syntax(&g_e.row[f_row]);
				}

				return;
			}
			j++;
		}
	}
}

#pragma endregion hl

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

/* calculate cx */
static int editor_row_rx_to_cx(e_row *row, int rx) {
	int cx;
	int cur_rx = 0;

	/* loop row */
	for (cx = 0; cx < row->sz; cx++) {
		/* handle tab size */
		if (row->line[cx] == '\t')
			cur_rx += (TAB_SIZE - 1) - (cur_rx % TAB_SIZE);
		/* handle every other char */
		cur_rx++;

		/* return if the provided rx was out of range */
		if (cur_rx > rx)
			return (cx);
	}

	/* return cx */
	return (cx);
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

	/* update syntax */
	editor_update_syntax(row);
}

/* insert / append row */
static void editor_insert_row(int idx, char *s, size_t len) {
	/* check index is valid */
	if (idx < 0 || idx > g_e.n_rows)
		return;

	/* realloc e_row struct to allocate all rows */
	g_e.row = (e_row *)realloc(g_e.row, sizeof(e_row) * (g_e.n_rows + 1));
	if (!g_e.row)
		die("realloc");

	/* shift rows */
	memmove(&g_e.row[idx + 1], &g_e.row[idx], sizeof(e_row) * (g_e.n_rows - idx));

	/* update index of the row (displaced by insert) */
	for (int i = idx + 1; i <= g_e.n_rows; i++) g_e.row[i].idx++;

	/* set row index to index */
	g_e.row[idx].idx = idx;

	/* insert / append row */
	g_e.row[idx].sz = len;
	g_e.row[idx].line = (char *)malloc(len + 1);
	memcpy(g_e.row[idx].line, s, len);
	g_e.row[idx].line[len] = '\0';

	/* initialise render */
	g_e.row[idx].r_sz = 0;
	g_e.row[idx].rend = NULL;
	g_e.row[idx].hl = NULL;
	g_e.row[idx].hl_open_comment = 0;
	editor_update_row(&g_e.row[idx]);

	/* increase number of rows */
	g_e.n_rows++;
	/* increase dirty (we make changes) */
	g_e.dirty++;
}

/* free row */
static void editor_free_row(e_row *row) {
	free(row->rend);
	free(row->line);
	free(row->hl);
}

/* delete row */
static void editor_del_row(int idx) {
	/* check index is valid */
	if (idx < 0 || idx >= g_e.n_rows)
		return;

	/* delete row */
	editor_free_row(&g_e.row[idx]);

	/* shift rest of the rows */
	memmove(&g_e.row[idx], &g_e.row[idx + 1], sizeof(e_row) * (g_e.n_rows - idx - 1));

	/* update index of the row (displaced by delete) */
	for (int i = idx; i < g_e.n_rows - 1; i++) g_e.row[i].idx--;

	/* update number of rows */
	g_e.n_rows--;
	/* update dirty */
	g_e.dirty++;
}

/* insert char in a row */
static void editor_row_insert_char(e_row *row, int idx, int c) {
	/* check idx is valid */
	if (idx < 0 || idx > row->sz)
		idx = row->sz;

	/* realloc line so it can store one more char */
	row->line = realloc(row->line, row->sz + 2);
	/* shift line one position from where we will add the char */
	/* memmove to prevent overlap, we are in the same string */
	memmove(&row->line[idx + 1], &row->line[idx], row->sz - idx + 1);
	/* update row size */
	row->sz++;
	/* insert chart */
	row->line[idx] = c;
	/* update row */
	editor_update_row(row);
	/* increase dirty (we make changes) */
	g_e.dirty++;
}

/* append str to a row */
static void editor_row_append_str(e_row *row, char *s, size_t len) {
	/* allocate space for the append */
	row->line = realloc(row->line, row->sz + len + 1);
	/* append string */
	memcpy(&row->line[row->sz], s, len);
	/* update row size */
	row->sz += len;
	/* update row */
	editor_update_row(row);
	/* set dirty */
	g_e.dirty++;
}

/* delete char in a row */
static void editor_row_del_char(e_row *row, int idx) {
	/* check idx is valid */
	if (idx < 0 || idx >= row->sz)
		return;

	/* shift line one position left */
	memmove(&row->line[idx], &row->line[idx + 1], row->sz - idx);
	/* update row size */
	row->sz--;
	editor_update_row(row);
	g_e.dirty++;
}

#pragma endregion row_ops

/*** editor operations ***/
#pragma region editor_ops

/* insert char */
static void editor_insert_char(int c) {
	/* check if we are in the tilde line to add a new row */
	if (g_e.cy == g_e.n_rows)
		editor_insert_row(g_e.n_rows, "", 0);
	/* insert char on the row so we see it */
	editor_row_insert_char(&g_e.row[g_e.cy], g_e.cx, c);
	/* move cursor */
	g_e.cx++;
}

/* insert new line */
static void editor_insert_nl() {
	/* if we are at the start of a row just insert an empty one */
	if (g_e.cx == 0) {
		editor_insert_row(g_e.cy, "", 0);
	/* else we will need to split the line we are on into two rows */
	} else {
		e_row *row = &g_e.row[g_e.cy];
		editor_insert_row(g_e.cy + 1, &row->line[g_e.cx], row->sz - g_e.cx);
		row = &g_e.row[g_e.cy];
		row->sz = g_e.cx;
		row->line[row->sz] = '\0';
		editor_update_row(row);
	}
	/* move cursor to the new line */
	g_e.cy++;
	g_e.cx = 0;
}

/* delete char */
static void editor_del_char() {
	/* if we are past the end of the file there is nothing to delete */
	/* return immediately */
	if (g_e.cy == g_e.n_rows)
		return;
	/* return if we are at start of file */
	if (g_e.cx == 0 && g_e.cy == 0)
		return;

	/* del char on the row so we see it */
	e_row *row = &g_e.row[g_e.cy];
	if (g_e.cx > 0) {
		editor_row_del_char(row, g_e.cx - 1);
		g_e.cx--;
	} else {
		/* move cursor to end of line so it end up where the two lines will join */
		g_e.cx = g_e.row[g_e.cy - 1].sz;
		/* append row to the previus row */
		editor_row_append_str(&g_e.row[g_e.cy - 1], row->line, row->sz);
		/* delete row (now is appended to other row) */
		editor_del_row(g_e.cy);
		/* move cursor again now that row is deleted */
		g_e.cy--;
	}
}

#pragma endregion editor_ops

/*** file i/o ***/
#pragma region file_io

/* convert rows to a string with all the file */
static char *editor_rows_to_str(int *buff_l) {
	int i;
	int t_len = 0;

	/* calculate total len of the file */
	for (i = 0; i < g_e.n_rows; i++)
		t_len += g_e.row[i].sz + 1;
	*buff_l = t_len;

	/* allocate memory to store rows */
	char *buff = (char *)malloc(t_len);
	/* loop rows and copy all to the buffer */
	char *ptr = buff;
	for (i = 0; i < g_e.n_rows; i++) {
		memcpy(ptr, g_e.row[i].line, g_e.row[i].sz);
		ptr += g_e.row[i].sz;
		*ptr = '\n';
		ptr++;
	}

	return (buff);
}

/* open file in editor */
static void editor_open(const char *filename) {
	/* set filename */
	free(g_e.filename);
	g_e.filename = strdup(filename);

	/* get file type to select hl */
	editor_select_syntax_hl();

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
		editor_insert_row(g_e.n_rows, line, line_l);
	}

	/* free line and close file */
	free(line);
	fclose(fp);
	/* reset dirty */
	g_e.dirty = 0;
}

/* save file in disk */
static void editor_save() {
	/* if is a new file (with no name of course) return */
	if (g_e.filename == NULL) {
		g_e.filename = editor_prompt("Save as: %s", NULL);
		if (g_e.filename == NULL) {
			editor_set_status_msg("Save aborted");
			return;
		}
		/* update syntax file type and see if it matchs now */
		editor_select_syntax_hl();
	}

	/* get string of all the file */
	int len;
	char *buff;
	buff = editor_rows_to_str(&len);

	/* open file */
	int fd;
	fd = open(g_e.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1) {
		/* set file size to len */
		/* if is larger it will cut off any data at the end */
		/* if is shorter it will add 0 bytes at the end */
		/* this way we ensure the file haz the size of the actual content */
		if (ftruncate(fd, len) != -1) {
			/* write buff to file and close */
			if (write(fd, buff, len) == len) {
				close(fd);
				free(buff);
				/* reset dirty */
				g_e.dirty = 0;
				/* set status bar message */
				// TODO add file size in bytes and colors
				editor_set_status_msg("\"%.20s\" %dL, %dC, written", g_e.filename ? g_e.filename : "[No Name]", g_e.n_rows, 0);
				return;
			}
		}
		/* close */
		close (fd);
	}

	/* free buf */
	free(buff);
	editor_set_status_msg("Error: cant save, %s", strerror(errno));
}

#pragma endregion file_io

/*** find ***/
#pragma region find

/* editor find callback */
static void editor_find_callback(char *query, int key) {
	static int last_match = -1;
	static int dir = 1;

	/* hl match saves to know which lines needs to be restored */
	static int saved_hl_line;
	static char *saved_hl = NULL;

	/* restore if it is something */
	if (saved_hl) {
		memcpy(g_e.row[saved_hl_line].hl, saved_hl, g_e.row[saved_hl_line].r_sz);
		free(saved_hl);
		saved_hl = NULL;
	}

	/* return if is a special action key */
	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		dir = 1;
		return ;
	/* check direction */
	} else if (key == K_ARROW_RIGHT || key == K_ARROW_DOWN) {
		dir = 1;
	} else if (key == K_ARROW_LEFT || key == K_ARROW_UP) {
		dir = -1;
	} else {
		last_match = -1;
		dir = 1;
	}

	/* check last_match */
	if (last_match == -1) dir = 1;
	int cur = last_match;
	/* loop rows and search for next match */
	int i;
	for (i = 0; i < g_e.n_rows; i++) {
		/* move cur one row (loop) */
		cur += dir;
		/* handle special cases */
		if (cur == -1) cur = g_e.n_rows -1;
		else if (cur == g_e.n_rows) cur = 0;

		/* get row */
		e_row *row = &g_e.row[cur];
		/* check for match into row */
		char *match = strstr(row->rend, query);
		if (match) {
			/* update last match */
			last_match = cur;
			/* positionate cursor y on match */
			g_e.cy = cur;
			/* positionate cursor x on start of the match */
			g_e.cx = editor_row_rx_to_cx(row, match - row->rend);
			/* positionate match line in top of screen */
			g_e.y_off = g_e.n_rows;

			/* get saved hl (so we can restore it later) */
			saved_hl_line = cur;
			saved_hl = (char *)malloc(row->r_sz);
			memcpy(saved_hl, row->hl, row->r_sz);
			/* set hl color to match */
			memset(&row->hl[match - row->rend], HL_MATCH, strlen(query));

			break;
		}
	}
}

/* find string in editor */
static void editor_find() {
	/* save cursor pos */
	int saved_cx = g_e.cx;
	int saved_cy = g_e.cy;
	int saved_x_off = g_e.x_off;
	int saved_y_off = g_e.y_off;

	/* ask for keyword and search in callback */
	char *query = editor_prompt("Search: %s", editor_find_callback);

	/* free */
	if (query) {
		free(query);
	/* if query is null is becouse we pressed ESC */
	} else {
		g_e.cx = saved_cx;
		g_e.cy = saved_cy;
		g_e.x_off = saved_x_off;
		g_e.y_off = saved_y_off;
	}
}

#pragma endregion find

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
		/* draw actual row */
		} else {
			/* get row len */
			int len = g_e.row[f_row].r_sz - g_e.x_off;
			if (len < 0) len = 0;
			if (len > g_e.scrn_cols) len = g_e.scrn_cols;
			/* get string to draw */
			char *c = &g_e.row[f_row].rend[g_e.x_off];
			/* get hl string */
			unsigned char *hl = &g_e.row[f_row].hl[g_e.x_off];
			/* for optimization record current color so we not do extra writes */
			int cur_color = -1;
			/* loop row */
			int i;
			for (i = 0; i < len; i++) {
				/* handle cursor */
				if (y == g_e.cy - g_e.y_off && i == g_e.rx - g_e.x_off) {
					apbuff_append(ab, "\x1b[m", 4);
					apbuff_append(ab, "\x1b[7m", 4);
					apbuff_append(ab, &c[i], 1);
					apbuff_append(ab, "\x1b[m", 4);
					/* print escape secuence for cur color so we not cut hl */
					if (cur_color != -1) {
						char buff[16];
						int c_len = snprintf(buff, sizeof(buff), "\x1b[%dm", cur_color);
						apbuff_append(ab, buff, c_len);
					}
				/* handle no print chars */
				} else if (iscntrl(c[i])) {
					char sym = (c[i] <= 26) ? '@' + c[i] : '?';
					apbuff_append(ab, "\x1b[7m", 4);
					apbuff_append(ab, &sym, 1);
					apbuff_append(ab, "\x1b[m", 3);
					/* print escape secuence for cur color so we not cut hl */
					if (cur_color != -1) {
						char buff[16];
						int c_len = snprintf(buff, sizeof(buff), "\x1b[%dm", cur_color);
						apbuff_append(ab, buff, c_len);
					}
				/* if normal color (white) */
				} else if (hl[i] == HL_NORMAL) {
					if (cur_color != -1) {
						/* set color */
						cur_color = -1;
						/* append color */
						apbuff_append(ab, "\x1b[39m", 5);
					}
					/* append char */
					apbuff_append(ab, &c[i], 1);
				/* handle syntax hl */
				} else {
					/* get color */
					int color = editor_syntax_to_color(hl[i]);
					if (color != cur_color) {
						/* set color */
						cur_color = color;
						/* get color escape char len */
						char buff[16];
						int c_len = snprintf(buff, sizeof(buff), "\x1b[%dm", color);
						/* append color */
						apbuff_append(ab, buff, c_len);
					}
					/* append char */
					apbuff_append(ab, &c[i], 1);
				}
			}
			/* handle cursor on empty lines */
			if (y == g_e.cy - g_e.y_off && g_e.row[f_row].sz == 0) {
				apbuff_append(ab, "\x1b[m", 4);
				apbuff_append(ab, "\x1b[7m", 4);
				apbuff_append(ab, " ", 1);
				apbuff_append(ab, "\x1b[m", 4);
			}
			/* reset to normal color */
			apbuff_append(ab, "\x1b[39m", 5);
		}

		/* erase part of the line to the right of the cursor */
		apbuff_append(ab, "\x1b[K", 3);
		/* dont write a new line on the last row */
		apbuff_append(ab, "\r\n", 2);
	}
}

/* draw status bar */
static void editor_draw_status_bar(struct apbuff *ab) {
	/* invert draw color */
	apbuff_append(ab, "\x1b[7m", 4);

	/* draw file name */
	char status[80];
	char r_status[80];
	/* get filename and file lines */
	/* TODO write file size in chars like sublivim */
	int len = snprintf(status, sizeof(status), "%.20s %s",
		g_e.filename ? g_e.filename : "[No Name]",
		g_e.dirty ? "[+] " : "");
	/* get status bar end string data */
	int r_len;
	// TODO print botton right correctly (like sublivim)
	/* get status bar end string */
	r_len = snprintf(r_status, sizeof(r_status), "%s | %d/%d",
		g_e.syntax ? g_e.syntax->f_type : "no ft",
		g_e.cy + 1,g_e.n_rows);
	/* append file name and file lines */
	if (len > g_e.scrn_cols) len = g_e.scrn_cols;
	apbuff_append(ab, status, len);

	/* draw status bar */
	while (len < g_e.scrn_cols) {
		if (g_e.scrn_cols - len == r_len) {
			apbuff_append(ab, r_status, r_len);
			break;
		} else {
			apbuff_append(ab, " ", 1);
			len++;
		}
	}

	/* reset draw color */
	apbuff_append(ab, "\x1b[m", 3);
	/* add new line for message bar */
	apbuff_append(ab, "\r\n", 2);
}

/* draw message bar */
static void editor_draw_msg_bar(struct apbuff *ab) {
	/* */
	apbuff_append(ab, "\x1b[K", 3);

	/* */
	int msg_l = strlen(g_e.status_msg);
	if (msg_l > g_e.scrn_cols) msg_l = g_e.scrn_cols;
	/* apped to buff */
	apbuff_append(ab, g_e.status_msg, msg_l);
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
	/* draw status bar and msg bar*/
	editor_draw_status_bar(&ab);
	editor_draw_msg_bar(&ab);

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

/* set status message */
static void editor_set_status_msg(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(g_e.status_msg, sizeof(g_e.status_msg), format, args);
	va_end(args);
}

#pragma endregion output

/*** input ***/
#pragma region input

/* editor prompt */
static char *editor_prompt(char *prompt, void (*callback)(char *, int)) {
	size_t buff_l = 0;
	size_t buff_sz = 128;
	char *buff = (char *)malloc(buff_sz);
	if (!buff)
		die ("malloc");

	/* initialise buff */
	buff[0] = '\0';

	while (1) {
		/* set status message */
		editor_set_status_msg(prompt, buff);
		/* refesh screen */
		editor_refresh_screen();

		/* get key input */
		int key = editor_read_key();
		/* handle delete keys */
		if (key == K_DEL || key == CTRL_KEY('h') || key == K_BACKSPACE) {
			if (buff_l != 0)
				buff[--buff_l] = '\0';
		/* quit prompt if ESC is pressed */
		} else if (key == '\x1b') {
			editor_set_status_msg("");
			/* callback */
			if (callback) callback(buff, key);
			free(buff);
			return (NULL);
		}
		/* is new line (enter) detect, return */
		if (key == '\r') {
			if (buff_l != 0) {
				editor_set_status_msg("");
				/* callback */
				if (callback) callback(buff, key);
				return (buff);
			}
		/* any other keys */
		} else if (!iscntrl(key) && key < 128) {
			/* if buffer does not have enought space duplicate it */
			if (buff_l == buff_sz - 1) {
				buff_sz *= 2;
				buff = (char *)realloc(buff, buff_sz);
			}
			/* register key into buffer */
			buff[buff_l++] = key;
			buff[buff_l] = '\0';
		}

		/* callback */
		if (callback) callback(buff, key);
	}
}

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
		if (g_e.cx != 0){
			g_e.cx--;
		} else if (g_e.cy > 0) {
			g_e.cy--;
			g_e.cx = g_e.row[g_e.cy].sz;
		}
	}
	else if (key == K_ARROW_RIGHT) {
		if (row && g_e.cx < row->sz) {
			g_e.cx++;
		} else if (row && g_e.cx == row->sz) {
			g_e.cy++;
			g_e.cx = 0;
		}
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

	/* new line */
	if (key == '\r') {
		editor_insert_nl();
	/* Ctrl-Q (currently quit the program) */
	} else if (key == CTRL_KEY('q')) {
		//TODO free all
		/* free all */
		//free(g_e.filename);
		//free(g_e.row[?].line)
		//free(g_e.row[?].rend)
		//free(g_e.row);

		/* clear screen and move cursor before exit */
		write(STDOUT_FILENO, "\x1b[2J", 4);
		write(STDOUT_FILENO, "\x1b[H", 3);
		exit(EXIT_SUCCESS);
	/* Ctrl-S (save key) */
	} else if (key == CTRL_KEY('s')) {
		editor_save();
	/* home key */
	} else if (key == K_HOME) {
		g_e.cx = 0;
	/* end key */
	} else if (key == K_END) {
		if (g_e.cy < g_e.n_rows)
			g_e.cx = g_e.row[g_e.cy].sz;
	/* Ctrl-F (search) */
	} else if (key == CTRL_KEY('f')) {
		editor_find();
	/* delete keys */
	} else if (key == K_BACKSPACE || key == CTRL_KEY('h') || key == K_DEL) {
		if (key == K_DEL)
			editor_move_cursor(K_ARROW_RIGHT);
		editor_del_char();
	/* page up and page down keys */
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
	/* arrow keys move the cursor */
	} else if (key == K_ARROW_UP || key == K_ARROW_DOWN || key == K_ARROW_LEFT || key == K_ARROW_RIGHT) {
		editor_move_cursor(key);
	/* dont do nothing with ESC and Ctrl-L (refresh screen, we already refresh every key press) */
	} else if (key == CTRL_KEY('l') || key == '\x1b') {
		;
	/* default case (rest of chars) */
	} else {
		editor_insert_char(key);
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
	g_e.dirty = 0;
	g_e.filename = NULL;
	g_e.status_msg[0] = '\0';
	g_e.syntax = NULL;

	/* get window size */
	if (get_windows_size(&g_e.scrn_rows, &g_e.scrn_cols) == -1)
		die("get_window_size");
	
	/* remove rows, status bar and msg bar */
	g_e.scrn_rows -= 2;
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
	
	/* set editor status msg empty at start */
	editor_set_status_msg("");

	/* program loop */
	while (1) {
		editor_refresh_screen();
		editor_process_keypress();
	}

	return (EXIT_SUCCESS);
}

#pragma endregion init
