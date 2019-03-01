#include "read_ods.h"

/* Read content.xml from an .ods file
 */
char* ods_get_xml(char* filename, char** xml_out, size_t* xml_len) {
	struct zip * ods_file;
    struct zip_file * table_xml_file;
    struct zip_stat table_xml_stats;
	int err = 0;
	char* entry_name = "content.xml";

	if ((ods_file = zip_open(filename, 0, &err)) == NULL) {
		return "Couldn't open ods file";
	}

	if (zip_stat(ods_file, entry_name, ZIP_FL_UNCHANGED, &table_xml_stats) == -1) {
		return "Couldn't stat the ods's content.xml";
	}
	if (! (table_xml_stats.valid & ZIP_STAT_SIZE)) {
		return "Couldn't get the ods's content.xml's size";
	}

	if ((table_xml_file = zip_fopen(ods_file, entry_name, ZIP_FL_UNCHANGED)) == NULL) {
		return "Couldn't open the ods's content.xml";
	}

	if ((*xml_out = (char*)malloc(table_xml_stats.size + 1)) == NULL) {
		zip_fclose(table_xml_file);
		zip_close(ods_file);
		return "Out of memory!";
	}
	(*xml_out)[table_xml_stats.size] = '\0';

	if (zip_fread(table_xml_file, *xml_out, table_xml_stats.size)
		< table_xml_stats.size)
	{
		zip_fclose(table_xml_file);
		zip_close(ods_file);
		return "Couldn't read all of content.xml";
	}

	zip_fclose(table_xml_file);
	zip_close(ods_file);
	return NULL;
}

