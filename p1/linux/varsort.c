#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sort.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

void usage() {
    fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
    exit(1);
}

// compare function for qsort
int compare(const void *a, const void *b) {
    const rec_dataptr_t *f1 = a;
    const rec_dataptr_t *f2 = b;
    if ( f1->key < f2->key ) {
        return -1;
    } else if ( f1->key == f2->key ) {
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char *argv[]) {
    // Input and output files initialisation
    char *inputfile   = NULL;
    char *outputfile   = NULL;

    // No argument error
    if (argc != 5)
      usage();

    // Parsing the input arguments
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "i:o:")) != -1) {
        switch (c) {
        case 'i':
            inputfile     = strdup(optarg);
            break;
        case 'o':
            outputfile     = strdup(optarg);
            break;
        default:
            usage();
        }
    }

    // checking whether the input arguments have been parsed properly
    if ( inputfile == NULL || outputfile == NULL ) {
        usage();
    }

    // Opening the input file
    int ifd = open(inputfile, O_RDONLY);
    if (ifd < 0) {
            fprintf(stderr, "Error: Cannot open file %s\n", inputfile);
            exit(1);
    }

    // Counting number of records in the file
    int nrecords;
    int rc;
    rc = read(ifd, &nrecords, sizeof(nrecords));
    if (rc != sizeof(nrecords)) {
            perror("read");
            exit(1);
    }
    // Reading all the data from file
    rec_dataptr_t* array;
    array = (rec_dataptr_t*)malloc(nrecords*sizeof(rec_dataptr_t));
    if (array == NULL) {
      fprintf(stderr, "malloc memory allocation failed\n");
      exit(1);
    }
    int i = 0;
    while (i < nrecords) {
      // Reading the fixed portion of the data
      rc = read(ifd, &array[i].key, sizeof(unsigned int));
      if (rc != sizeof(unsigned int)) {
        perror("read");
        exit(1);
      }
      rc = read(ifd, &array[i].data_ints, sizeof(unsigned int));
      if (rc != sizeof(unsigned int)) {
        perror("read");
        exit(1);
      }

      // Read the variable portion of the record
      int ptr_size = array[i].data_ints*sizeof(unsigned int);
      array[i].data_ptr = (unsigned int*)malloc(ptr_size);
      if (array[i].data_ptr == NULL) {
        fprintf(stderr, "malloc memory allocation failed\n");
        exit(1);
      }
      rc = read(ifd, array[i].data_ptr, ptr_size);
      if (rc !=  array[i].data_ints * sizeof(unsigned int)) {
        perror("read");
        exit(1);
      }
      i++;
    }


    // Sorting
    qsort(array, nrecords, sizeof(rec_dataptr_t), compare);

    // Open and create output file
    int ofd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
    if (ofd < 0) {
      fprintf(stderr, "Error: Cannot open file %s\n", outputfile);
      exit(1);
    }

    // Output the number of keys as a header for this file
    int oc;
    oc = write(ofd, &nrecords, sizeof(nrecords));
    if (oc != sizeof(nrecords)) {
      perror("write");
      exit(1);
    }

    // Output the fixed and variable part of the data
    i = 0;
    while (i < nrecords) {
      int data_size = array[i].data_ints*sizeof(unsigned int);
      oc = write(ofd, &array[i].key, sizeof(array[i].key));
      if (oc != sizeof(array[i].key)) {
        perror("write");
        exit(1);
      }
      oc = write(ofd, &array[i].data_ints, sizeof(array[i].data_ints));
      if (oc != sizeof(array[i].data_ints)) {
        perror("write");
        exit(1);
      }
      oc = write(ofd, array[i].data_ptr, data_size);
      free(array[i].data_ptr);
      if (oc != data_size) {
        perror("write");
        exit(1);
      }
      i++;
    }

    // Freeing the allocated memory
    free(array);
    free(inputfile);
    free(outputfile);

    // Closing the opened files
    close(ifd);
    close(ofd);

    return 0;
}
