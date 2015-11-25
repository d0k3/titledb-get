#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE	(16 * 1024 * 1024)
#define FORCE_PARTITION 0

void showhelp_exit() {
	printf(" usage: importfix [import.db]\n\n");
	exit(0);
}

int main( int argc, char** argv )
{
	unsigned char* titledb;
    int fsize;
    
	
	printf("\nimportfix by d0k3\n");
	printf("---------------------\n\n");
	
	if(argc != 2) showhelp_exit();

    titledb = (unsigned char*) malloc(BUFFER_SIZE);
	if(titledb == NULL) {
		printf("out of memory");
		return 0;
	}
    
	FILE* fp = fopen(argv[1], "rb");
	if(fp == NULL) {
		printf("open %s failed!\n\n", argv[1]);
		return 0;
	}
    fsize = fread(titledb, 1, BUFFER_SIZE, fp);
    fclose(fp);
    
    // check active partition
    #ifndef FORCE_PARTITION
    int ap = *((int*) (titledb + 0x130));
    if ((ap < 0) || (ap > 1)) {
        printf("%08X: active partition invalid! (%i)\n", 0x130, ap);
        return 0;
    }
    #else
    int ap = FORCE_PARTITION;
    #endif
    printf("%08X: active partition: %s\n", 0x130, (ap) ? "secondary" : "primary");
    
    unsigned char* difi = titledb + *((int*) (titledb + ((ap) ? 0x110 : 0x108)));
    if (memcmp(difi, "DIFI", 4) != 0) {
        printf("%08X: 'DIFI' fail!\n\n", difi - titledb);
        return 0;
    }
    printf("%08X: 'DIFI' offset\n", difi - titledb);
    int offset_ivfc_lvl4 = *((int*) (difi + 0x9C));
    int offset_ivfc_part = *((int*) (difi + 0xF4));
    int offset_file_base = *((int*) (titledb + 0x120));
    int size_ivfc_part = *((int*) (difi + 0xFC));
    int difi_flags = *((int*) (difi + 0x38));
    
    unsigned char* nandtdb = titledb + offset_ivfc_lvl4 + offset_ivfc_part + offset_file_base + ((difi_flags) ? 0 : size_ivfc_part);
    if (memcmp(nandtdb, "NANDIDB", 7) != 0) {
        printf("%08X: 'NAND' fail!\n\n", nandtdb - titledb);
        return 0;
    }
    printf("%08X: 'NANDIDB' offset\n", nandtdb - titledb);
    
    unsigned char* bdri = nandtdb + 0x80;
    //int bdrisize = *((int*) (bdri + 0x10)) / *((int*) (bdri + 0x18));
    if (memcmp(bdri, "BDRI", 4) != 0) {
        printf("%08X: 'BDRI' fail!\n\n", bdri - titledb);
        return 0;
    }
    printf("%08X: 'BDRI' offset\n", bdri - titledb);
    
    unsigned char* tentbl = bdri + *((int*) (bdri + 0x58));
    int nten = *((int*) (tentbl + 0x2C));
    int ntenmax = *((int*) (tentbl + 0x80));
    if (tentbl - titledb > fsize) {
        printf("%08X: title entry table fail!\n\n", tentbl - titledb);
        return 0;
    }
    printf("%08X: entry table (%08X%08X) -> %i/%i entries\n", tentbl - titledb, *((unsigned int*) tentbl), *((unsigned int*) (tentbl + 4)), nten, ntenmax);
    
    if (nten > 0) {
        printf("\nTrying to reset the update nag... ");
        memset(tentbl + 0x2C, 0x00, 4);
        printf("done!\n");
        printf("Writing changes to file... ");
        fp = fopen(argv[1], "wb");
        if(fp == NULL) {
            printf("open %s failed!\n\n", argv[1]);
            return 0;
        }
        fsize = fwrite(titledb, 1, BUFFER_SIZE, fp);
        fclose(fp);
        printf("done!\n");
    } else {
        printf("\nUpdate nag not detected!\n");
    }
    
    printf("\nPress any key to quit\n\n");
    getchar();
    
    return 0;
}
