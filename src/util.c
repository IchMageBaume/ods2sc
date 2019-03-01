#include "util.h"

/* like strlen(char*), but backwards and returns the pointer instead of the length
 */
char* strstart(char* end) {
	while(end[-1] != '\0') end--;

	return end;
}

/* Generate the letter part of a cell name, e.g. the "HG" of HG32 into buf.
 * Note that x is zero-based in this case, which it isn't for the rest of the code.
 *
 * It was easier to code it to grow the name in to buf backwards, so you need to give it
 * a pointer of the end of a sufficiently long buffer and then seek backwards to find the
 * point where the letters start.
 */
void cell_letter_grow_backwards(int x, char* buf) {
	*buf = (x % 26) + 'A';
	if (x > 26) cell_letter_grow_backwards(x / 26 - 1, buf - 1);
}

/* Put the name of a cell, e.g. JG3042 into buf
 */
void get_cell_name(int x, int y, char* buf) {
	char letterbuf[10] = { '\0' };
	cell_letter_grow_backwards(x - 1, letterbuf + 9);
	char* letters = strstart(letterbuf + 9);

	sprintf(buf, "%s%d", letters, y);
}

/* Get how many times a row/cell/column is repeated, which is used in the ods format for
 * compression
 */
int get_number_repeated(xmlNode* node) {
	int repeated = 1;

	for (xmlAttr* at = node->properties; at != NULL; at = at->next) {
		if (at->children != NULL) {
			if (! strcmp((char*)(at->name), "number-rows-repeated") ||
				! strcmp((char*)(at->name), "number-columns-repeated"))
			{
				repeated = atoi((char*)(at->children->content));
			}
		}
	}

	return repeated;
}

int node_empty(xmlNode* node) {

	for (xmlNode* cur_node = node; cur_node; cur_node = cur_node->next) {
		if (cur_node->content != NULL) {
			if (cur_node->content[0] != '\0') {
				return 0;
			}
		}

		if (! node_empty(cur_node->children)) return 0;
	}

	return 1;
}
