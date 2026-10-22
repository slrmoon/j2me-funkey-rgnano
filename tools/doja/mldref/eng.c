#include "fka.h"
#include "fka_eas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int fka_eas_init(int e){(void)e;return 0;} void fka_eas_shutdown(void){} int fka_eas_set_engine(int e){(void)e;return 0;}
void fka_eas_stream_engine(void*s,int e){(void)s;(void)e;} int fka_eas_frame(void){return 128;} void *fka_eas_open(void){return NULL;}
void fka_eas_close(void*s){(void)s;} void fka_eas_write(void*s,const uint8_t*m,int l){(void)s;(void)m;(void)l;} void fka_eas_volume(void*s,int g){(void)s;(void)g;}
void fka_eas_transpose(void*s,int t){(void)s;(void)t;} void fka_eas_silence(void*s,int i){(void)s;(void)i;} void fka_eas_reset(void*s){(void)s;}
int fka_eas_channel_volume(void*s,int c){(void)s;(void)c;return 0;} int fka_eas_channel_program(void*s,int c,int*b){(void)s;(void)c;(void)b;return 0;}
int fka_eas_active(void*s){(void)s;return 0;} int fka_eas_render(int16_t*o){memset(o,0,256);return 0;} int fka_eas_load_bank(const char*p){(void)p;return 0;}
int fka_output_open(void){return 0;} void fka_output_close(void){} void fka_lock(void){} void fka_unlock(void){}
void PhoneMEProfileThread(void){}
int main(int c,char**v){ FILE*f=fopen(v[1],"rb"); long n; unsigned char*d; int id,i; int16_t buf[1024]; FILE*o=fopen(v[2],"wb");
 fseek(f,0,2);n=ftell(f);fseek(f,0,0);d=malloc(n);if(fread(d,1,n,f)!=(size_t)n)return 1;fclose(f);
 fka_init(); id=fka_player_new(d,(int)n,NULL); printf("id %d kind %d\n",id,fka_player_kind(id)); fka_player_prefetch(id); fka_player_start(id);
 for(i=0;i<22050*10/1024;i++){ fka_render(buf,1024); fwrite(buf,2,1024,o);} fclose(o); return 0; }
