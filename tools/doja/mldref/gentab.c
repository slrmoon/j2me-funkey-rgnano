#include "fka_mld.c"
#include <stdio.h>
static void arr(const char *n, const int32_t *a, int c){ int i; printf("static const int32_t %s[%d] = {", n, c); for(i=0;i<c;i++) printf("%s%s%d", i?",":"", i%16?"":"\n    ", a[i]); printf("\n};\n"); }
int main(void){ int32_t s[256],t[256],w[512]; int i; tables_init();
 for(i=0;i<256;i++){s[i]=WAVES[0][i]; t[i]=WAVES[16][i];} for(i=0;i<512;i++) w[i]=WAVES[24][i];
 printf("/* Generated: MA-3 synth tables as computed by the Java reference\n * (StrictMath), so every platform gets identical values. */\n#ifndef FKA_MA3_TABLES_H\n#define FKA_MA3_TABLES_H\n#include <stdint.h>\n");
 arr("T_EXP",(int32_t*)EXPT,256); arr("T_SIN",s,256); arr("T_TRI",t,256); arr("T_SAW",w,512); arr("T_SUSTAINS",(int32_t*)SUSTAINS,16); arr("T_WAVE_ENV",(int32_t*)WAVE_ENV,512);
 printf("#endif\n"); return 0; }
