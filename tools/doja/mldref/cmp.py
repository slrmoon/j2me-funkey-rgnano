import wave, struct, sys, glob, os
def rd(p):
    w = wave.open(p); d = w.readframes(w.getnframes()); return struct.unpack('<%dh' % (len(d)//2), d)
tot_ok = 0
for ref in sorted(glob.glob('/root/work/ref-wav/*.wav')):
    name = os.path.basename(ref)
    mine = '/root/work/mldc/out/' + name
    a = rd(ref); b = rd(mine)
    n = min(len(a), len(b))
    diff = sum(1 for i in range(n) if a[i] != b[i])
    mx = max((abs(a[i]-b[i]) for i in range(n)), default=0)
    first = next((i for i in range(n) if a[i] != b[i]), -1)
    ok = diff == 0 and len(a) == len(b)
    tot_ok += ok
    print('%-22s len %7d/%7d  diff %7d  max %5d  first %d %s' % (name, len(a)//2, len(b)//2, diff, mx, first//2 if first>=0 else -1, 'OK' if ok else ''))
print('identical:', tot_ok)
