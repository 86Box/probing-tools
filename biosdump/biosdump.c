#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
#ifdef STANDARD_MAIN
main(int argc, char *argv[])
#else
main(void)
#endif
{
    FILE *f;
    char *p;
    uint32_t size, ret;

    printf("Dumping low BIOS range (000C0000-000FFFFF)... ");
    p = (char *) 0x000c0000;
    size = 0x00040000;
    f = fopen("000c0000.dmp", "wb");
    if (f == NULL) {
        printf("FAILURE\n");
        return 1;
    }
    ret = fwrite(p, 1, size, f);
    if (ret != size) {
        printf("FAILURE\n");
        fclose(f);
        return 2;
    }
    fclose(f);
    printf("OK\n\n");

    printf("Dumping mid BIOS range (00FE0000-00FFFFFF)... ");
    p = (char *) 0x00fe0000;
    size = 0x00020000;
    f = fopen("00fe0000.dmp", "wb");
    if (f == NULL) {
        printf("FAILURE\n");
        return 3;
    }
    ret = fwrite(p, 1, size, f);
    if (ret != size) {
        printf("FAILURE\n");
        fclose(f);
        return 4;
    }
    fclose(f);
    printf("OK\n\n");

    printf("Dumping high BIOS range (FFF80000-FFFFFFFF)... ");
    p = (char *) 0xfff80000;
    size = 0x00080000;
    f = fopen("fff80000.dmp", "wb");
    if (f == NULL) {
        printf("FAILURE\n");
        return 5;
    }
    ret = fwrite(p, 1, size, f);
    if (ret != size) {
        printf("FAILURE\n");
        fclose(f);
        return 6;
    }
    fclose(f);
    printf("OK\n");

    return 0;
}
