#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#define GLOBAL extern
#include "nutcpu.h"
#include "display.h"
#include "nut_rom_cx.h"
#include "hp41_key_bridge.h"
extern const uint16_t rom_frames_p0[4096];
static int pump(int iters) { int ret=0; for(int i=0;i<iters;i++){ret=executeNUT(1000); if(ret!=0)break;} return ret; }
static bool asleep_state=false;
static int type_byte(int c){ if(asleep_state){flagKey=0;regPC=0;asleep_state=false;} hp41_key_bridge_feed_byte(c); int ret=pump(200000); if(ret==1)asleep_state=true; return ret; }
static void type_str(const char*s){for(;*s;s++)type_byte((unsigned char)*s);}
static void xeq(const char*n){type_byte(0x18);type_byte(0x01);type_str(n);type_byte(0x01);}

/* Read the stack file's header (abs XM reg = 64 + 0 = register "1" of
 * the file, since CRFLD's first data register lands right after the
 * catalog's own bookkeeping - we don't know the exact absolute XM index
 * a priori, so instead read back via GETX/SEEKPTA through real keystrokes
 * for verification, independent of the module's own internal addressing. */
static int query_S(void) {
    type_byte(0x01); type_byte(0x01); /* clear alpha */
    type_str("1");
    xeq("SEEKPTA");
    xeq("GETX");
    char dispbuf[32];
    const char *d = display_to_buf(dispbuf);
    /* crude parse: strip spaces, take leading digits */
    char buf[32]; int j=0;
    for (int i=0; d[i] && j<31; i++) if (d[i] != ' ') buf[j++] = d[i];
    buf[j]=0;
    return atoi(buf);
}

int main(void) {
    char dispbuf[32];
    setvbuf(stdout, NULL, _IONBF, 0);
    nut_boot_cx();
    tabpage[8] = (short *)(const void *)rom_frames_p0;
    typmod[8] = 1;
    int ret = pump(300000);
    printf("cold boot disp=%s\n", display_to_buf(dispbuf));
    if (ret == 1) asleep_state = true;

    /* One-time setup: create the MFSTK file (LCLS itself no longer does
     * this - see frames.s's scope-cut note - calling CRFLD unconditionally
     * on every LCLS hits DUP FL on the 2nd+ call, and an error return
     * doesn't hand control back mid-routine).
     *
     * Size 2, not 1: confirmed empirically that SEEKPTA to a position
     * that is the LAST register of a file fails with "END OF FL" (works
     * fine on a size-10 file seeking to register 1, fails on a size-1
     * file seeking to its only register). Register 1 is the permanent
     * header here, so the file must never be sized such that register 1
     * is also the last one - size 2 keeps it safely non-last from the
     * very first LCLS onward (every real size after that is 6, 10, 14...,
     * still safely bigger than 1). */
    type_byte(0x01); type_str("MFSTK"); type_byte(0x01);
    type_str("2");
    xeq("CRFLD");
    printf("setup CRFLD(MFSTK,2): disp=\"%s\"\n", display_to_buf(dispbuf));

    unsigned char snap0[128 * 8];
    memcpy(snap0, &espaceRAM[64 * 8], sizeof(snap0));

    xeq("LCLS");
    printf("after LCLS #1: disp=\"%s\" S=%d (expect 4)\n", display_to_buf(dispbuf), query_S());

    unsigned char snap1[128 * 8];
    memcpy(snap1, &espaceRAM[64 * 8], sizeof(snap1));
    printf("XM RAM diff after LCLS #1:\n");
    for (int reg = 0; reg < 128; reg++) {
        if (memcmp(&snap0[reg * 8], &snap1[reg * 8], 8) != 0) {
            printf("  XM reg %d: ", reg);
            for (int b = 0; b < 8; b++) printf("%02x ", snap0[reg * 8 + b]);
            printf("-> ");
            for (int b = 0; b < 8; b++) printf("%02x ", snap1[reg * 8 + b]);
            printf("\n");
        }
    }
    printf("regPer=0x%02x regPT=%d regPQ[0]=%d regPQ[1]=%d flagPrgm=? dspon=%d\n",
           regPer, regPT, regPQ[0], regPQ[1], dspon);

    xeq("LCLS");
    printf("after LCLS #2: disp=\"%s\" S=%d (expect 8)\n", display_to_buf(dispbuf), query_S());

    xeq("LCLS");
    printf("after LCLS #3: disp=\"%s\" S=%d (expect 12)\n", display_to_buf(dispbuf), query_S());

    xeq("LCLX");
    printf("after LCLX #1: disp=\"%s\" S=%d (expect 8)\n", display_to_buf(dispbuf), query_S());

    xeq("LCLX");
    printf("after LCLX #2: disp=\"%s\" S=%d (expect 4)\n", display_to_buf(dispbuf), query_S());

    xeq("LCLX");
    printf("after LCLX #3: disp=\"%s\" S=%d (expect 0)\n", display_to_buf(dispbuf), query_S());

    return 0;
}
