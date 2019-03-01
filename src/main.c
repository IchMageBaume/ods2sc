#include <stdio.h>

#include "read_ods.h"
#include "xml_to_sc.h"

int main(int argc, char* argv[]) {
	char* ods_name = argc >= 2? argv[1] : "test.ods";
	char* table_content_xml;
	size_t xml_len;
	char* err;
	int ret;
	
	err = ods_get_xml(ods_name, &table_content_xml, &xml_len);
	if (err != NULL) {
		fprintf(stderr, "Error reading %s: %s\n", ods_name, err);
		return 1;
	}

	ret = xml_to_sc(table_content_xml);
	if (ret != 0) {
		// TODO make better error messages. (Or any at all lul)
		fprintf(stderr, "xml_to_sc returned %d\n", ret);
		return ret;
	}

	return 0;
}

