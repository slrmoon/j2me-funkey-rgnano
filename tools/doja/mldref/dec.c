#include "fka_decode.h"
#include <stdio.h>
#include <stdlib.h>
int main(int c, char **v){ FILE *f=fopen(v[1],"rb"); long n; unsigned char *d; FkaContent ct; long long i; FILE *o;
 fseek(f,0,2); n=ftell(f); fseek(f,0,0); d=malloc(n); if(fread(d,1,n,f)!=(size_t)n) return 1; fclose(f);
 if (fka_decode(d,(int)n,NULL,&ct)!=0){printf("decode failed\n");return 1;}
 printf("kind %d type %s len %lld\n", ct.kind, ct.type, (long long)ct.pcm.length);
 o=fopen(v[2],"wb"); for(i=0;;i++){ const short *s=fka_pcm_at(&ct.pcm,i); if(!s) break; fwrite(s,2,1,o);} fclose(o);
 printf("samples %lld\n", i); return 0; }
