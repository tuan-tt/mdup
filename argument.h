#ifndef _ARGUMENT_H_
#define _ARGUMENT_H_

#include "attr.h"

#define VERSION			"1.0"

struct prog_args {
	int n_thread;
	bool is_remove;
	bool is_barcode;
	bool is_write;
	char *in_bam;
	char *reference;
	char *out_dir;
};

void print_usage();

void get_args(int argc, char *argv[]);

#endif /* _ARGUMENT_H_ */
