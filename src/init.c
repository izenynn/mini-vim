#include <minivim.h>

extern struct editor_conf g_e;

/* initialise editor */
void init_editor() {
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
	g_e.mode = NORMAL_MODE;
	g_e.syntax = NULL;

	/* get window size */
	if (get_windows_size(&g_e.scrn_rows, &g_e.scrn_cols) == -1)
		die("get_window_size");
	
	/* remove rows, status bar and msg bar */
	g_e.scrn_rows -= 2;
}
