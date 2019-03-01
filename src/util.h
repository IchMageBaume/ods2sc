#ifndef _UTIL_INCLUDED_
#define _UTIL_INCLUDED_

#include <string.h>
#include <libxml/parser.h>

void get_cell_name(int x, int y, char* buf);
int get_number_repeated(xmlNode* node);
int node_empty(xmlNode* node);

#endif
