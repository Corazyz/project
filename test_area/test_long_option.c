#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

static struct option long_options[] = {
    {"version",      no_argument,       0,   'V'},
    {"output",       required_argument, 0,   'o'},
    {0,              0,                 0,    0}
};

void print_version(void) {
    printf("This is version 1.0 of the program.\n");
}
void usage(const char *progname) {
    fprintf(stderr, "Usage: %s [options]\n", progname);
    fprintf(stderr, "Options: \n");
    fprintf(stderr, " -V, --version    Show version information\n");
    fprintf(stderr, " -o FILE, --output-FILE\n");
    fprintf(stderr, "                      Specify the output file\n");
}

int main(int argc, char *argv[]){
    int c, opt_idx = 0;
    char *output_file = NULL;

    while(1) {
        c = getopt_long(argc, argv, "Vo:", long_options, &opt_idx);
        if (c == -1) {
            printf("c = -1\n");
            break;
        }
        switch (c)
        {
        case 'V':
            print_version();
            break;
        case 'o':
            output_file = optarg;
            printf("Output file set to %s\n", output_file);
            break;
        case '?':
            usage(argv[0]);
            exit(EXIT_FAILURE);
        default:
            abort();
        }
    }
    printf("optint = %d\n", optind);
    if (optind < argc) {
        printf("Non-option arguments: ");
        while(optind < argc) {
            printf(" %s", argv[optind++]);
        }
        printf("\n");
    }
    return 0;
}