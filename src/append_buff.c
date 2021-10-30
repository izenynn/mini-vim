#include <minivim.h>

/* append new string to struct apbuff */
void apbuff_append(struct apbuff *ab, const char *s, int len) {
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
void apbuff_free(struct apbuff *ab) {
	free(ab->buff);
}
