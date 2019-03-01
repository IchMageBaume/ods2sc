#include "xml_to_sc.h"

void walk_node(xmlNode* node, int level);
void parse_table(xmlNode* node, int y);
void cell_to_sc(xmlNode* cell, int x, int y);

/* Call libxml and pass control to recursive parser
 */
int xml_to_sc(char* xml) {
	xmlDocPtr doc;

	doc = xmlReadMemory(xml, strlen(xml), "content.xml", NULL, 0);
	if (doc == NULL) {
		return 1;
	}

	walk_node(xmlDocGetRootElement(doc), 0);

	xmlFreeDoc(doc);
	return 0;
}

/* Recursively go through xml, and pass control to table parser when a table is found
 */
void walk_node(xmlNode* node, int level) {
	for (xmlNode* cur_node = node; cur_node != NULL; cur_node = cur_node->next) {

		if (! strcmp((const char*)(cur_node->name), "table")) {
			parse_table(cur_node->children, 1); // 1 as starting y value
			break;
		}
		else {
			walk_node(cur_node->children, level + 1);
		}
	}
}

/* Recursively go through table
 *
 * y needed since it calls itself recursively
 */
void parse_table(xmlNode* node, int y) {
	
	for (xmlNode* cur_node = node; cur_node != NULL; cur_node = cur_node->next) {

		if (! strcmp((char*)(cur_node->name), "table-column")) {
			// do nothing; maybe implement style stuff here eventually
		}
		else if (! strcmp((char*)(cur_node->name), "table-row")) {

			/* there is a property called "number-rows-repeated" that is used
			 * to compress stuff. Get that and recurse thru the element multiple
			 * times, adjusting y as needed.
			 */
			int repeated_row = get_number_repeated(cur_node);

			/* If a row is empty, it would be possible to still loop through it, but
			 * some retarded documents have >100k empty rows at the end, so we can speed
			 * this up a lot if we ignore those instances
			 */
			if (node_empty(cur_node)) {
				y += repeated_row;
				continue;
			}

			// go through all the cells, as many times as told by number-rows-repeated

			for (int i = 0; i < repeated_row; i++, y++) {

				int x = 1;
				for (xmlNode* cell = cur_node->children; cell != NULL;
					cell = cell->next)
				{
					int repeated_cell = get_number_repeated(cell);

					// same as with rows here
					if (node_empty(cell)) {
						x += repeated_cell;
						continue;
					}

					for (int j = 0; j < repeated_cell; j++, x++) {
						cell_to_sc(cell, x, y);
					}
				}
			}
		}
	}
}

/* Spit out .sc source for cell
 */
void cell_to_sc(xmlNode* cell, int x, int y) {
	/* cells can have a formula and a precalculated value, so we loop through attributes,
	 * set value_attr if we find a value and output the value if we get past the
	 * loop
	 */
	xmlAttr* value_attr = NULL;

	for (xmlAttr* at = cell->properties; at != NULL; at = at->next) {

		if (! strcmp((char*)(at->name), "value-type")) {
			if (! strcmp((char*)(at->children->content), "string")) {
				char cellname[10] = { '\0' };
				get_cell_name(x, y, cellname);

				// TODO not all is this easy :c
				printf("leftstring %s = \"%s\"\n",
					cellname, cell->children->children->content);
				return;
			}
		}
		else if (! strcmp((char*)(at->name), "formula")) {
			char cellname[10] = { '\0' };
			get_cell_name(x, y, cellname);

			int max_formula_len = strlen((char*)(at->children->content));
			char formula_raw[max_formula_len + 1];
			char formula_parsed[max_formula_len + 1];

			strcpy(formula_raw, (char*)(at->children->content));
			for (int i = 0; i < max_formula_len + 1; i++) formula_parsed[i] = '\0';

			/* TODO this doesn't support the whole OpenFormula standard, though it might
			 * never do.
			 * But it'd be nice if it at least turned "SUM(...)" into "@SUM(...)"
			 */

			// remove stuff that I don't like character by character
			int formula_parsed_offset = 0;
			for (char* current_pos = formula_raw + 4;
				*current_pos != '\0'; current_pos++)
			{
				if (
					// allowed characters
					*current_pos == ' ' ||
					*current_pos == '+' ||
					*current_pos == '-' ||
					*current_pos == '*' ||
					*current_pos == '/' ||
					*current_pos == '^' ||
					*current_pos == '(' ||
					*current_pos == ')' ||
					*current_pos == ':' ||
					*current_pos == ';' ||
					(
						*current_pos >= 'a' && *current_pos <= 'z'
					) ||
					(
						*current_pos >= 'A' && *current_pos <= 'Z'
					) ||
					(
						*current_pos >= '0' && *current_pos <= '9'
					)
				)
				{
					formula_parsed[formula_parsed_offset] = *current_pos;
					formula_parsed_offset++;
				}
			}

			printf("let %s = %s\n", cellname, formula_parsed);
			return;
		}
		else if (! strcmp((char*)(at->name), "value")) {
			value_attr = at;
		}
	}

	if (value_attr != NULL) {
		char cellname[10] = { '\0' };
		get_cell_name(x, y, cellname);

		printf("let %s = %s\n", cellname, value_attr->children->content);
	}
}

