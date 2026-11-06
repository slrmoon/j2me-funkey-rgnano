# Usage: python3 topng.py frame.raw out.png  (320x240 RGB565)
import struct,zlib,sys
d=open(sys.argv[1],'rb').read();W,H=320,240
rows=b''.join(b'\0'+b''.join(bytes(((p>>11)<<3,((p>>5)&63)<<2,(p&31)<<3)) for p in struct.unpack('<320H',d[y*640:(y+1)*640])) for y in range(H))
c=lambda t,x:struct.pack('>I',len(x))+t+x+struct.pack('>I',zlib.crc32(t+x))
open(sys.argv[2],'wb').write(b'\x89PNG\r\n\x1a\n'+c(b'IHDR',struct.pack('>IIBBBBB',W,H,8,2,0,0,0))+c(b'IDAT',zlib.compress(rows))+c(b'IEND',b''))
