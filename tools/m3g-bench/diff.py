# Usage: python3 diff.py a.raw b.raw  (320x240 RGB565 frames from bench)
import struct,sys
a=struct.unpack('<76800H',open(sys.argv[1],'rb').read()); b=struct.unpack('<76800H',open(sys.argv[2],'rb').read())
def ch(p): return ((p>>11)<<3,((p>>5)&63)<<2,(p&31)<<3)
n=0;mx=0;big=0
for p,q in zip(a,b):
    if p!=q:
        n+=1;d=max(abs(x-y) for x,y in zip(ch(p),ch(q)));mx=max(mx,d);big+=d>8
print(f"diff px {n} ({100*n/76800:.2f}%), max channel diff {mx}, >8: {big}")
