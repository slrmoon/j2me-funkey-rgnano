#!/bin/bash
# tcmp.sh name : compare the sampler call traces of ORender (openDoJa) and
# the C port ($W/c/trender, built with -DFKA_MLD_TRACE) for /root/work/mld/<name>.mld
W=${W:-/tmp/odref}; f=/root/work/mld/$1.mld
/usr/lib/jvm/java-21-openjdk-amd64/bin/java -cp $W/out TRender $f /dev/null 22050 30 2>&1 >/dev/null | grep -v Picked > $W/tj.txt
$W/c/trender $f /dev/null 22050 30 2>$W/tc.txt >/dev/null
python3 - "$W/tj.txt" "$W/tc.txt" <<'P'
import sys
import re
def norm(p):
    out=[]
    for l in open(p):
        t=l.split(); r=[]
        for x in t:
            try:
                if re.match(r'^-?\d+$',x): r.append(x)
                else: r.append('%.6g'%float(x))
            except: r.append(x)
        out.append(' '.join(r))
    return out
a=norm(sys.argv[1]); b=norm(sys.argv[2])
for i,(x,y) in enumerate(zip(a,b)):
    if x!=y:
        print('line',i,'J:',x,'| C:',y); print(' J ctx', a[max(0,i-3):i+3]); print(' C ctx', b[max(0,i-3):i+3]); break
else: print('same', len(a), len(b))
P
