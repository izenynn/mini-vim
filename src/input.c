#include <minivim.h>

/* editor prompt */
char *editor_prompt(char *prompt, void (*callback)(char *, int)) {
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
void editor_move_cursor(int key) {
	e_row *row = (g_e.cy >= g_e.n_rows) ? NULL : &g_e.row[g_e.cy];
	int right_off = g_e.mode == NORMAL_MODE ? 1 : 0;

	if (key == K_ARROW_UP || key == 'k') {
		if (g_e.cy != 0) {
			g_e.cy--;
		}
	}
	else if (key == K_ARROW_DOWN || key == 'j') {
		if (g_e.cy < g_e.n_rows) {
			g_e.cy++;
		}
	}
	else if (key == K_ARROW_LEFT || key == 'h') {
		if (g_e.cx != 0){
			g_e.cx--;
		}
	}
	else if (key == K_ARROW_RIGHT || key == 'l') {
		if (row && g_e.cx < row->sz - right_off) {
			g_e.cx++;
		}
	}

	row = (g_e.cy >= g_e.n_rows) ? NULL : &g_e.row[g_e.cy];
	int row_l = row ? row->sz : 0;
	if (g_e.cx > row_l) {
		g_e.cx = row_l;
	}
}

/* process key press */
void editor_process_keypress() {
	int key;

	key = editor_read_key();

	/* normal mode */
	if (g_e.mode == NORMAL_MODE) {
		/* move keys */
		if (key == 'k' || key == 'j' || key == 'h' || key == 'l'
			|| key == K_ARROW_UP || key == K_ARROW_DOWN || key == K_ARROW_LEFT || key == K_ARROW_RIGHT) {
			editor_move_cursor(key);
		/* insert keys */
		} else if (key == 'i' || key == 'a' || key == 'o' || key == 'O') {
			/* change to insert mode */
			g_e.mode = INSERT_MODE;
			/* move cursor depending on key */
			if (key == 'a') {
				editor_move_cursor(K_ARROW_RIGHT);
			} else if (key == 'o' || key == 'O') {
				if (key == 'O') editor_move_cursor(K_ARROW_UP);
				/* move cursor to end of line */
				if (g_e.cy < g_e.n_rows)
					g_e.cx = g_e.row[g_e.cy].sz;
				/* insert nl from there */
				editor_insert_nl();
			}
			editor_set_status_msg("-- INSERT --");
		/* home key */
		} else if (key == '^' || key == K_HOME) {
			g_e.cx = 0;
		/* end key */
		} else if (key == '$' || key == K_END) {
			if (g_e.cy < g_e.n_rows && g_e.row[g_e.cy].sz > 0)
				g_e.cx = g_e.row[g_e.cy].sz - 1;
		/* page up and page down */
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
		}
	/* insert mode */
	} else if (g_e.mode == INSERT_MODE) {
		/* new line */
		if (key == '\r') {
			editor_insert_nl();
		/* Ctrl-Q (currently quit the program) */
		} else if (key == CTRL_KEY('q')) {
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
		/* change to normal mode */
		} else if (key == '\x1b') {
			g_e.mode = NORMAL_MODE;
			editor_move_cursor(K_ARROW_LEFT);
			editor_set_status_msg("");
		/* default case (rest of chars) */
		/* dont do nothing for Ctrl-L (default for refresh screen) */
		/* we already handle the refresh ourselves */
		} else if (key != CTRL_KEY('l')) {
			editor_insert_char(key);
		}
	}
}
