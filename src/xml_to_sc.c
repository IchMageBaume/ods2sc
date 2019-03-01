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

			/* formula_raw has + 4 offset since it starts with something like "of:="
			 * as far as I've seen
			 *
			 * formula_parsed has + 200 size since it can be longer than the original 
			 * formula, though usually it is not
			 */
			int formula_len = strlen((char*)(at->children->content));
			char* formula_raw = (char*)(at->children->content + 4);
			char formula_parsed[formula_len + 200];

			for (int i = 0; i < formula_len + 1; i++) formula_parsed[i] = '\0';

			/* remove stuff that I don't like character by character
			 *
			 * copied_until points to the next character that hasn't been copied yet;
			 * we don't copy character by character so we can add the '@' to function
			 * names when we find out that we've been looping over one
			 */
			int formula_parsed_offset = 0;
			char* copied_until = formula_raw;
			for (char* current_pos = formula_raw; *current_pos != '\0'; current_pos++) {
				if (
					*current_pos == ' ' ||
                    *current_pos == '+' ||
                    *current_pos == '-' ||
                    *current_pos == '*' ||
                    *current_pos == '/' ||
                    *current_pos == '^' ||
					*current_pos == ':' ||
					*current_pos == ';' ||
					*current_pos == ')' ||
					*current_pos == '%' ||
					(
						*current_pos >= '0' && *current_pos <= '9'
					)
				)
				{
					// copy over stuff so far; nothing out of the ordinary here
					memcpy(formula_parsed + formula_parsed_offset, copied_until,
						current_pos - copied_until + 1);
					formula_parsed_offset += current_pos - copied_until + 1;
					copied_until = current_pos + 1;
				}
				else if (
					(
						*current_pos >= 'a' && *current_pos <= 'z'
					) ||
					(
						*current_pos >= 'A' && *current_pos <= 'Z'
					)
				)
				{
					// don't copy yet, we might be in the name of a function
				}
				else if (*current_pos == '(') {
					/* if copied_until isn't right here, the name of a function is
					 * between it and current_pos
					 */

					// copy an '@' to formula_parsed since thats how sc functions start
					// TODO this is not enough; I'll need a real function translation
					// table for this. but at least sum() -> @sum() works c:
					formula_parsed[formula_parsed_offset] = '@';
					formula_parsed_offset++;
					memcpy(formula_parsed + formula_parsed_offset, copied_until,
						current_pos - copied_until + 1);
					formula_parsed_offset += current_pos - copied_until + 1;
					copied_until = current_pos + 1;
				}
				else {
					// we got some unwanted character here; just discard it
					copied_until = current_pos + 1;
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

