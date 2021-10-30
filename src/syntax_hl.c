#include <minivim.h>

/*** filetypes ***/

char *C_HL_extensions[C_HL_EXT_SIZE] = { ".c", ".h", ".cpp", ".hpp", ".cc", NULL};

char *C_HL_keywords[C_HL_KEYW_SIZE] = {
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

struct e_syntax HLDB[HLDB_SIZE] = {
	{
		"c",
		C_HL_extensions,
		C_HL_keywords,
		"//", "/*", "*/",
		HL_HL_NBR | HL_HL_STR
	},
};

# define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/* return true if char is a separator */
int is_separator(int c) {
	return (isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL);
}

/* set row syntax */
void editor_update_syntax(e_row *row) {
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
int editor_syntax_to_color(int hl) {
	if (hl == HL_COMMENT || hl == HL_MLCOMMENT) return (36);
	if (hl == HL_KEYWORD1) return (33);
	if (hl == HL_KEYWORD2) return (32);
	if (hl == HL_STRING) return (35);
	if (hl == HL_NUMBER) return (31);
	if (hl == HL_MATCH) return (34);
	else return (37);
}

/* select syntax hl */
void editor_select_syntax_hl() {
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
