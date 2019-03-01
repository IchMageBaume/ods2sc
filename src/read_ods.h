#ifndef _READ_ODS_INCLUDED_
#define _READ_ODS_INCLUDED_

#include <stdlib.h>
#include <zip.h>

char* ods_get_xml(char* filename, char** xml_out, size_t* xml_len);

#endif // _READ_ODS_INCLUDED_
