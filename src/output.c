#include <minivim.h>

/* check if we can scroll */
void editor_scroll() {
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
void editor_draw_rows(struct apbuff *ab) {
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
					apbuff_append(ab, "\x1b[34m~\x1b[m", 9);
					padding--;
				}
				while (padding--)
					apbuff_append(ab, " ", 1);
				apbuff_append(ab, welcome, welcome_l);
			/* there is a document but no more rows to print */
			} else {
				/* if nothing to draw */
				apbuff_append(ab, "\x1b[34m~\x1b[m", 9);
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
				if (CURSOR_HL && g_e.mode == NORMAL_MODE && y == g_e.cy - g_e.y_off && i == g_e.rx - g_e.x_off) {
					/* change color on cursor position only */
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
			if (CURSOR_HL && g_e.mode == NORMAL_MODE && y == g_e.cy - g_e.y_off && g_e.row[f_row].sz == 0) {
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
void editor_draw_status_bar(struct apbuff *ab) {
	/* invert draw color */
	apbuff_append(ab, "\x1b[7m", 4);

	/* draw file name */
	char status[80];
	char r_status[80];
	/* get filename and file lines */
	int len = snprintf(status, sizeof(status), "%.20s %s",
		g_e.filename ? g_e.filename : "[No Name]",
		g_e.dirty ? "[+] " : "");
	/* get status bar end string data */
	int r_len;
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
void editor_draw_msg_bar(struct apbuff *ab) {
	/* */
	apbuff_append(ab, "\x1b[K", 3);

	/* */
	int msg_l = strlen(g_e.status_msg);
	if (msg_l > g_e.scrn_cols) msg_l = g_e.scrn_cols;
	/* apped to buff */
	apbuff_append(ab, g_e.status_msg, msg_l);
}

/* refresh editor screen */
void editor_refresh_screen() {
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
void editor_set_status_msg(const char *format, ...) {
	va_list args;
	va_start(args, format);
	vsnprintf(g_e.status_msg, sizeof(g_e.status_msg), format, args);
	va_end(args);
}
