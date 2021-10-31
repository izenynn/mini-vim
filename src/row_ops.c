#include <minivim.h>

/* calculate rx */
int editor_row_cx_to_rx(e_row *row, int cx) {
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
int editor_row_rx_to_cx(e_row *row, int rx) {
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
void editor_update_row(e_row *row) {
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
		} else if (row->line[i] == ' ') {
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
void editor_insert_row(int idx, char *s, size_t len) {
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
void editor_free_row(e_row *row) {
	free(row->rend);
	free(row->line);
	free(row->hl);
}

/* delete row */
void editor_del_row(int idx) {
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
void editor_row_insert_char(e_row *row, int idx, int c) {
	/* check idx is valid */
	if (idx < 0 || idx > row->sz)
		idx = row->sz;

	/* realloc line so it can store one more char */
	row->line = (char *)realloc(row->line, row->sz + 2);
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
void editor_row_append_str(e_row *row, char *s, size_t len) {
	/* allocate space for the append */
	row->line = (char *)realloc(row->line, row->sz + len + 1);
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
void editor_row_del_char(e_row *row, int idx) {
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
