#include "xml_to_sc.h"

void walk_node(xmlNode* node, int level);
void parse_table(xmlNode* node, int y);
void cell_to_sc(xmlNode* cell, int x, int y);

char* function_translations[] = {
	/* Having the '@' and '(' here makes other code parts easier and faster
	 */
	// TODO you know what, I think we need a reusable FOSS OpenFormula parser
	"sqrt", "@sqrt("
	"ln", "@ln(",
	"rounddown", "@floor(",
	"roundup", "@ceil(",
	"round", "@round("
	"abs", "@abs(",
	"power", "@pow(",
	"pi", "@pi", // does this even work?
	"radians", "@rtd(",
	"degrees", "@dtr(",
	"sin", "@sin(",
	"cos", "@cos(",
	"tan", "@tan(",
	"asin", "@asin(",
	"acos", "@acos(",
	"atan", "@atan(",
	"atan2", "@atan2(",



	"sum", "@sum(",
	NULL
};

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
// TODO gosh this is messy
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
			memset(formula_parsed, '\0', formula_len + 200);

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
				else if (*current_pos == ';') {
					formula_parsed[formula_parsed_offset] = ',';
					formula_parsed_offset++;
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
					/* if copied_until isn't current_pos here, the name of a function is
					 * between it and current_pos
					 */

					char function_lowercase[current_pos - copied_until + 1];
					function_lowercase[current_pos - copied_until] = '\0';
					for (int i = 0; i < current_pos - copied_until; i++) {
						char tmp = copied_until[i];

						/* big ascii characters come before the small ones,
						 * so this checks if tmp is big or small
						 */
						function_lowercase[i] = tmp <= 'Z'? tmp + ('a' - 'A') : tmp;
					}

					int found_translation = 0; // no double break, so we need this flag
					for (int table_offset = 0;
						function_translations[table_offset] != NULL; table_offset += 2)
					{
						if (! strcmp(function_translations[table_offset],
							function_lowercase))
						{
							found_translation = 1;

							strcat(formula_parsed + formula_parsed_offset,
								function_translations[table_offset + 1]);
							formula_parsed_offset +=
								strlen(function_translations[table_offset + 1]);

							break;
						}
					}

					if (! found_translation) {
						// just put an '@' in front and hope it works, lol
						formula_parsed[formula_parsed_offset] = '@';
						formula_parsed_offset++;
						memcpy(formula_parsed + formula_parsed_offset, copied_until,
							current_pos - copied_until + 1);
						formula_parsed_offset += current_pos - copied_until + 1;
						copied_until = current_pos + 1;
					}
				}
				else {
					/* we got some unwanted character here; just ignore it and set
					 * copied_until since these unwanted characters shouldn't appear in
					 * function names
					 */
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

