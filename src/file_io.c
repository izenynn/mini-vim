#include <minivim.h>

/* convert rows to a string with all the file */
char *editor_rows_to_str(int *buff_l) {
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
void editor_open(const char *filename) {
	/* set filename */
	free(g_e.filename);
	g_e.filename = strdup(filename);

	/* get file type to select hl */
	editor_select_syntax_hl();

	/* open file */
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		fp = fopen(filename, "w");
		if (!fp) {
			die("fopen");
		}
	}

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
void editor_save() {
	/* if is a new file (with no name of course) return */
	if (g_e.filename == NULL) {
		editor_set_status_msg("\x1b[41mERROR: no file name\x1b[m");
		return;
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
				editor_set_status_msg("\"%.20s\" %dL, written", g_e.filename ? g_e.filename : "[No Name]", g_e.n_rows);
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
