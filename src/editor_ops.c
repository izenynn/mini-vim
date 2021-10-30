#include <minivim.h>

/* insert char */
void editor_insert_char(int c) {
	/* check if we are in the tilde line to add a new row */
	if (g_e.cy == g_e.n_rows)
		editor_insert_row(g_e.n_rows, "", 0);
	/* insert char on the row so we see it */
	editor_row_insert_char(&g_e.row[g_e.cy], g_e.cx, c);
	/* move cursor */
	g_e.cx++;
}

/* insert new line */
void editor_insert_nl() {
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
void editor_del_char() {
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
