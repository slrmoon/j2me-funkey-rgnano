import wave, struct, glob, os, sys
def rd(p):
    w = wave.open(p); d = w.readframes(w.getnframes()); return struct.unpack('<%dh' % (len(d)//2), d)
ok=0
for ref in sorted(glob.glob(sys.argv[1]+'/*.wav')):
    name=os.path.basename(ref); a=rd(ref); b=rd(sys.argv[2]+'/'+name); n=min(len(a),len(b))
    diff=sum(1 for i in range(n) if a[i]!=b[i]); mx=max((abs(a[i]-b[i]) for i in range(n)),default=0)
    first=next((i for i in range(n) if a[i]!=b[i]),-1)
    tail_ref=max((abs(x) for x in a[n:]),default=0)
    good = diff==0 and tail_ref<=1
    ok+=good
    print('%-18s ref %7d c %7d diff %7d max %5d first %7d reftail %5d %s'%(name,len(a)//2,len(b)//2,diff,mx,first//2 if first>=0 else -1,tail_ref,'OK' if good else ''))
print('ok',ok)
