/* vim: set tabstop=3 expandtab:
**
** This file is in the public domain.
**
** osd.c
**
** $Id: osd.c,v 1.2 2001/04/27 14:37:11 neil Exp $
**
*/

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <noftypes.h>
#include <nofconfig.h>
#include <log.h>
#include <osd.h>
#include <nofrendo.h>

#include <version.h>

#include <eadk.h>

char configfilename[]="na";

int osd_init() {
  return 0;
}

void osd_shutdown() {
}

// CRC algorithm taken from https://barrgroup.com/blog/crc-series-part-3-crc-implementation-code-cc
// We took the slow implementation to save memory, as we only need to compute it once
/*
 * The width of the CRC calculation and result.
 * Modify the typedef for a 16 or 32-bit CRC standard.
 */
typedef uint32_t crc;

#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0x04C11DB7

crc crcSlow(uint8_t const message[], int nBytes) {
    crc  remainder = 0;
    /*
     * Perform modulo-2 division, a byte at a time.
     */
    for (int byte = 0; byte < nBytes; ++byte) {
        /*
         * Bring the next byte into the remainder.
         */
        remainder ^= (message[byte] << (WIDTH - 8));

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (uint8_t bit = 8; bit > 0; --bit) {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & TOPBIT) {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
    }

    /*
     * The final remainder is the CRC result.
     */
    return (remainder);
}   /* crcSlow() */


/* This is os-specific part of main() */
int osd_main(int argc, char *argv[]) {
  config.filename = configfilename;
  uint32_t crc = crcSlow(eadk_external_data, eadk_external_data_size);
  char crcHex[9];
  sprintf(crcHex, "%08x",  crc);

  return main_loop(crcHex, system_autodetect);
}

void osd_getmouse(int *x, int *y, int *button) {
}

/* File system interface */
void osd_fullname(char *fullname, const char *shortname)
{
   strncpy(fullname, shortname, PATH_MAX);
}

/* This gives filenames for storage of saves */
char *osd_newextension(char *string, char *ext)
{
    int l=strlen(string);
    while(l && string[l]!='.') {
        l--;
    }
    if (l) string[l]=0;
    strcat(string, ext);
    return string;
}

/* This gives filenames for storage of PCX snapshots */
int osd_makesnapname(char *filename, int len)
{
   return -1;
}
