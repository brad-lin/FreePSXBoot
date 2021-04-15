#include <stdio.h>

int main(int argc, char ** argv) {
    if (argc != 4) {
        printf("usage: %s input output label\n", argv[0]);
        return -1;
    }

    FILE * in = fopen(argv[1], "rb");
    if (!in) {
        printf("Unable to open input file %s\n", argv[1]);
        return -1;
    }

    FILE * out = fopen(argv[2], "wb");
    if (!out) {
        printf("Unable to open output file %s\n", argv[2]);
        fclose(in);
        return -1;
    }

    fseek(in, 0, SEEK_END);
    int size = ftell(in);
    fseek(in, 0, SEEK_SET);

    fprintf(out, "static unsigned char %s[%i] = {\n    ", argv[3], size);

    for (unsigned i = 0; i < size; i++) {
        unsigned c = fgetc(in);
        fprintf(out, "0x%02x, ", c);
        if (((i + 1) % 16) == 0) {
            fprintf(out, "\n");
            if (i != (size - 1)) fprintf(out, "    ");
        }
    }
    if ((size % 16) != 0) {
        fprintf(out, "\n");
    }
    fprintf(out, "};\n");

    fclose(in);
    fclose(out);
    return 0;
}
