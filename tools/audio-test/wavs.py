import sys,os,subprocess,struct,wave,numpy as np
R=22050
def load(p):
    w=wave.open(p); n=w.getnframes(); d=np.frombuffer(w.readframes(n),dtype=np.int16).astype(np.float64)
    if w.getnchannels()==2: d=d.reshape(-1,2).mean(axis=1)
    return d, w.getframerate()
def hdr(p):
    b=open(p,"rb").read(); i=12; info={}
    while i+8<=len(b):
        cid=b[i:i+4]; sz=struct.unpack_from("<I",b,i+4)[0]
        if cid==b"fmt ":
            fmt,ch,rate,br,ba,bits=struct.unpack_from("<HHIIHH",b,i+8); info=dict(fmt=fmt,ch=ch,rate=rate,bits=bits)
        if cid==b"data": info["data"]=min(sz,len(b)-i-8); 
        i+=8+sz+(sz&1)
    return info
def db(x): return 20*np.log10(max(x,1e-9))
rows=[]
for g in sys.argv[1:]:
    for f in sorted(os.listdir("media/"+g)):
        if not f.endswith(".wav"): continue
        p="media/%s/%s"%(g,f); h=hdr(p)
        out=subprocess.run(["tool/fkrender",p,"/tmp/o.wav","20"],capture_output=True,text=True).stdout
        dur=[l for l in out.splitlines() if l.startswith("type")]
        subprocess.run(["sox",p,"-r",str(R),"-c","1","-b","16","/tmp/r.wav","rate","-v"],capture_output=True)
        try:
            o,_=load("/tmp/o.wav"); r,_=load("/tmp/r.wav")
        except Exception as e:
            print(g,f,h,"ERR",e,out.strip()); continue
        n=min(len(o),len(r)); 
        # trim our trailing silence
        nz=np.nonzero(np.abs(o)>8)[0]; olen=(nz[-1]+1)/R if len(nz) else 0
        rms_o=np.sqrt((o[:n]**2).mean()); rms_r=np.sqrt((r[:n]**2).mean())
        # best lag
        k=min(n,R*2); c=np.correlate(o[:k],r[:k],"full"); lag=c.argmax()-(k-1)
        a=o[max(lag,0):n]; b=r[max(-lag,0):n]; m=min(len(a),len(b)); corr=np.corrcoef(a[:m],b[:m])[0,1] if m>100 else 0
        # energy above source nyquist
        def hi(x):
            S=np.abs(np.fft.rfft(x*np.hanning(len(x))))**2; fr=np.fft.rfftfreq(len(x),1/R); return S[fr>h["rate"]/2*1.05].sum()/max(S.sum(),1e-9)
        print("%-5s %-24s %5d Hz %2d bit ch%d fmt%d | len src %.3f ours %.3f | level %+5.1f dB | peak %5d | lag %4d corr %.3f | above-nyquist ours %.2f%% ref %.3f%%"%(g,f,h["rate"],h["bits"],h["ch"],h["fmt"],h["data"]/(h["rate"]*h["ch"]*h["bits"]/8),olen,db(rms_o)-db(rms_r),np.abs(o).max(),lag,corr,hi(o[:n])*100,hi(r[:n])*100))
