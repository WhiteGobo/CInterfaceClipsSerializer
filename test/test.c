#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <regex.h>

#include "CInterfaceClipsSerializer.h"
#include <NQuadsRDF.h>

static char *premisefile = NULL;
static char *conclusionfile = NULL;

static int parse_args(int argc, char *argv[]);
static char* serialize_premise();
static bool check_regex(const char*);
static char* load_into_memory(const char* filepath);

int main(int argc, char *argv[]){
	int err;
	char* output;
	err = parse_args(argc, argv);
	if (err != 0){
		exit(EXIT_FAILURE);
	}

	output = serialize_premise();
	if(output == NULL){
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "output was:\n%s\n", output);
	if(!check_regex(output)){
		free(output);
		exit(EXIT_FAILURE);
	}
	free(output);
	exit(EXIT_SUCCESS);
}

static bool check_regex(const char* x){
	int err;
	char* regstring = load_into_memory(conclusionfile);
	fprintf(stderr, "Use regex:\n%s\n\n", regstring);
	regex_t reg;
	err = regcomp(&reg, regstring, REG_EXTENDED);
	free(regstring);
	if (err != 0){
		fprintf(stderr, "Failed to compile regex: %s\n", regstring);
		return false;
	}
	err = regexec(&reg, x, 0, NULL, 0);
	regfree(&reg);
	if (err != 0){
		return false;
	}
	return true;
}


static char* serialize_premise(){
	int err;
	char* ret;
	ClipsSerializer* serializer = ClipsSerializer_new();
	if(serializer == NULL){
		fprintf(stderr, "ERROR ClipsSerializer_new failed\n");
		return NULL;
	}
	err = nquads_parse_file(premisefile,
			(TripleHandler*) ClipsSerializer_add, serializer);
	ret = ClipsSerializer_finish(serializer);
	if(err != 0){
		fprintf(stderr, "ERROR during parsing of %s\n", conclusionfile);
		return NULL;
	}
	if(ret == NULL){
		fprintf(stderr, "ClipsSerializer_finish failed to "
				"return output\n");
	}
	return ret;
}

static struct option parse_options[] = {
	{"premise", required_argument, NULL, 'i'},
	{"conclusion", required_argument, NULL, 'o'},
        {NULL, 0, NULL, 0}
};

static int parse_args(int argc, char *argv[]){
	int err = 0;
	int c = 0;
	int option_index;
	while(c != -1){
		c = getopt_long(argc, argv, "",
				parse_options, &option_index);
		switch(c){
			case -1: //end of arguments
				break;
			case 'i':
				premisefile = optarg;
				break;
			case 'o':
				conclusionfile = optarg;
				break;
			default:
				fprintf(stderr, "unrecognized argument\n");
				err = 1;
				break;
		}
	}
	if (premisefile == NULL){
		fprintf(stderr, "no premisepath given\n");
		err = 1;
	}
	if (conclusionfile == NULL){
		fprintf(stderr, "no conclusionpath given\n");
		err = 1;
	}
	return err;
}


static char* load_into_memory(const char* filepath){
        char *ret;
        long fsize;
        FILE *f = fopen(filepath, "rb");
	fprintf(stderr, "brubru %s\n", filepath);
        if (f == NULL) return NULL;
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        rewind(f);
        //fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

        ret = malloc(fsize + 1);
        fread(ret, fsize, 1, f);
        ret[fsize] = 0;
        fclose(f);
        return ret;
}
