import sys,os,subprocess,struct,wave,numpy as np
R=22050
def smf_len(p):
    b=open(p,"rb").read(); fmt,ntr,div=struct.unpack_from(">HHH",b,8); i=14; evs=[]; chans=set(); progs={}; notes=0
    for t in range(ntr):
        assert b[i:i+4]==b"MTrk"; ln=struct.unpack_from(">I",b,i+4)[0]; j=i+8; end=j+ln; tick=0; rs=0
        while j<end:
            v=0
            while True:
                c=b[j]; j+=1; v=(v<<7)|(c&0x7f)
                if c<0x80: break
            tick+=v; st=b[j]
            if st<0x80: st=rs
            else: j+=1
            if st==0xff:
                mt=b[j]; j+=1; l=0
                while True:
                    c=b[j]; j+=1; l=(l<<7)|(c&0x7f)
                    if c<0x80: break
                if mt==0x51: evs.append((tick,0,int.from_bytes(b[j:j+3],"big")))
                evs.append((tick,2,0)); j+=l
            elif st in (0xf0,0xf7):
                l=0
                while True:
                    c=b[j]; j+=1; l=(l<<7)|(c&0x7f)
                    if c<0x80: break
                j+=l; evs.append((tick,2,0))
            else:
                rs=st; hi=st&0xf0; n=1 if hi in (0xc0,0xd0) else 2
                if hi==0x90 and b[j+1]>0: notes+=1; chans.add(st&15)
                if hi==0xc0: progs[st&15]=b[j]
                j+=n; evs.append((tick,1,0))
        i=end
    evs.sort(key=lambda e:(e[0],e[1])); us=0; last=0; tempo=500000
    for tick,k,v in evs:
        us+=(tick-last)*tempo/div; last=tick
        if k==0: tempo=v
    return us/1e6,fmt,ntr,notes,sorted(chans),progs
def load(p):
    w=wave.open(p); d=np.frombuffer(w.readframes(w.getnframes()),dtype=np.int16).astype(np.float64)
    if w.getnchannels()==2: d=d.reshape(-1,2).mean(axis=1)
    return d
def db(x): return 20*np.log10(max(x,1e-9)/32768)
def stats(d):
    nz=np.nonzero(np.abs(d)>16)[0]
    if len(nz)==0: return (-99,-99,0)
    d2=d[:nz[-1]+1]; return (db(np.sqrt((d2**2).mean())),db(np.abs(d).max()),len(d2)/R)
def spec(d):
    n=min(len(d),R*20); S=np.abs(np.fft.rfft(d[:n]*np.hanning(n)))**2; fr=np.fft.rfftfreq(n,1/R)
    edges=np.geomspace(60,11000,25); return np.array([S[(fr>=a)&(fr<b)].sum() for a,b in zip(edges[:-1],edges[1:])])+1e-3
print("%-6s %-18s %6s | %-27s | %-22s | %-22s | %-22s | lsd W/F/H vs ref"%("game","file","smf s","ours: dur  eom  audible","wavetable rms/peak","fm rms/peak","hybrid rms/peak"))
for g in sys.argv[1:]:
    for f in sorted(os.listdir("media/"+g)):
        if not f.endswith(".mid"): continue
        p="media/%s/%s"%(g,f); L,fmt,ntr,notes,chans,progs=smf_len(p)
        subprocess.run(["fluidsynth","-ni","-g","0.5","-R","0","-C","0","-r",str(R),"-F","/tmp/ref.wav","/usr/share/sounds/sf2/FluidR3_GM.sf2",p],capture_output=True)
        ref=load("/tmp/ref.wav"); rs=stats(ref); rsp=spec(ref)
        res=[]; info=""
        for synth in (1,2,3):
            out=subprocess.run(["tool/fkrender",p,"/tmp/o%d.wav"%synth,"60",str(synth)],capture_output=True,text=True).stdout
            d=load("/tmp/o%d.wav"%synth); s=stats(d)
            dur=float(out.split("duration")[1].split()[0]); eom=[l for l in out.splitlines() if l.startswith("event 1")]
            eomt=float(eom[0].split("render")[1].split()[0]) if eom else -1
            sp=spec(d); w=rsp/rsp.sum(); lsd=np.sqrt((w*(10*np.log10(sp/sp.sum())-10*np.log10(rsp/rsp.sum()))**2).sum())
            res.append((s,lsd)); 
            if synth==1: info="%6.3f %6.3f %6.3f"%(dur,eomt,s[2])
        print("%-6s %-18s %6.3f | %-27s | %s | ref %5.1f/%5.1f len %.2f | lsd %s | ch %s"%(g,f,L,info," | ".join("%6.1f/%5.1f"%(r[0][0],r[0][1]) for r in res),rs[0],rs[1],rs[2]," ".join("%.1f"%r[1] for r in res),len(chans)))
