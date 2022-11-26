#include "../md.h"

#include <stdio.h>
#include <stdlib.h>


void my_printf(FILE* fp) {
	char current;
	while((current = fgetc(fp)) != EOF) putchar(current);
}

int main(int argc, char *argv[]) {
    if(argv[1] == '0') {
        FILE *tsf;
        tsf = fopen(DEVICE_PATH, "w+");
        fprintf(tsf, argv[2]);
        my_printf(tsf);
        fclose(tsf);
    }
    else {
        FILE *tsf;
        tsf = fopen(DEVICE1_PATH, "w+");
        fprintf(tsf, argv[2]);
        my_printf(tsf);
        fclose(tsf);
    }
    return 0;    
}
