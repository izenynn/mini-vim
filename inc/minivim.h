#ifndef MINIVIM_H
# define MINIVIM_H

/*** special defines ***/

# define _DEFAULT_SOURCE
# define _BSD_SOURCE
# define _GNU_SOURCE

/*** includes ***/

# include <ctype.h>
# include <errno.h>
# include <fcntl.h>
# include <stdlib.h>
# include <string.h>
# include <stdio.h>
# include <stdarg.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <termios.h>
# include <unistd.h>

/*** defines ***/

# define MINIVIM_VER "0.1"

# ifndef TAB_SIZE
#  define TAB_SIZE 4
# endif

# ifndef CURSOR_HL
#  define CURSOR_HL 1
# endif

# define CTRL_KEY(k) ((k) & 0x1f)
# define APBUFF_INIT {NULL, 0}

# define NORMAL_MODE 0
# define INSERT_MODE 1

# define C_HL_EXT_SIZE 6
# define C_HL_KEYW_SIZE 82
# define HLDB_SIZE 1

# define HL_HL_NBR (1<<0)
# define HL_HL_STR (1<<1)

/*** enums ***/

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
	/* 0: normal, 1: insert */
	int	mode;
	struct e_syntax *syntax;
	struct termios org_termios;
};

/* append buff struct */
struct apbuff {
	char *buff;
	size_t len;
};

/* editor_conf global var */
extern struct editor_conf g_e;

/* init.c */
void init_editor();

/* input.c */
char *editor_prompt(char *prompt, void (*callback)(char *, int));
void editor_move_cursor(int key);
void editor_process_keypress();

/* output.c */
void editor_scroll();
void editor_draw_rows(struct apbuff *ab);
void editor_draw_status_bar(struct apbuff *ab);
void editor_draw_msg_bar(struct apbuff *ab);
void editor_refresh_screen();
void editor_set_status_msg(const char *format, ...);

/* append_buff.c */
void apbuff_append(struct apbuff *ab, const char *s, int len);
void apbuff_free(struct apbuff *ab);

/* find.c */
void editor_find_callback(char *query, int n);
void editor_find();

/* file_io.c */
char *editor_rows_to_str(int *buff_l);
void editor_open(const char *filename);
void editor_save();

/* editor_ops.c */
void editor_insert_char(int c);
void editor_insert_nl();
void editor_del_char();

/* row_ops.c */
int editor_row_cx_to_rx(e_row *row, int cx);
int editor_row_rx_to_cx(e_row *row, int rx);
void editor_update_row(e_row *row);
void editor_insert_row(int idx, char *s, size_t len);
void editor_free_row(e_row *row);
void editor_del_row(int idx);
void editor_row_insert_char(e_row *row, int idx, int c);
void editor_row_append_str(e_row *row, char *s, size_t len);
void editor_row_del_char(e_row *row, int idx);

/* syntax_hl.c */
int is_separator(int c);
void editor_update_syntax(e_row *row);
int editor_syntax_to_color(int hl);
void editor_select_syntax_hl();

/* terminal.c */
void die(const char *s);
void dis_raw_mode();
void enb_raw_mode();
int editor_read_key();
int get_cursor_pos(int *rows, int *cols);
int get_windows_size(int *rows, int *cols);

#endif
