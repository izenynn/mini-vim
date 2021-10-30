#define DEFINE_I
#include <minivim.h>

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
