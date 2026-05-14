/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */
package javax.microedition.media;

import java.util.Vector;
import java.util.Enumeration;
import java.util.Hashtable;

import java.io.*;

import javax.microedition.media.*;
import javax.microedition.media.control.*;

public class ToneSequencePlayer extends ABBBasicPlayer implements Runnable, ToneControl
       { private native int  nTSPlayerInit();
         private native int  nTSPlayerRealize(int id, byte[] buf);
         private native int  nTSPlayerStart(int id);
         private native void nTSPlayerStop(int id);
         private native void nTSPlayerDeallocate(int id);
         private native void nTSPlayerClose(int id);
         private native long nTSGetMediaTime(int id);
         private native int  nTSCheckEOM(int id);
         private Thread      CheckThread;
         private int         TSPlayer_ID;
         private byte[]      TS_Sequence;
         private boolean     SequenceLoaded;
         
         public ToneSequencePlayer() throws Exception
                { super();
                  this.TSPlayer_ID = nTSPlayerInit();
                  if (this.TSPlayer_ID == 0) throw new Exception("Out Of Memory Error!!!"); 
                  this.CheckThread = new Thread(this);
                  TS_Sequence = new byte[0];
                  SequenceLoaded = false;
                }
    
         public int getAudioType() 
                { return AUDIO_MIDI;
                }

         protected void doRealize() throws MediaException
                   { 
                   }

         protected void doPrefetch() throws MediaException
                   { 
                   }

         protected boolean doStart()
                   { if (nTSPlayerStart(this.TSPlayer_ID) == 0) return true;
                     return false;
                   }

         protected void doPostStart() 
                   { this.CheckThread.start();
                   }
                   
         protected void doStop() throws MediaException
                   { nTSPlayerStop(this.TSPlayer_ID);
                   }

         protected void doDeallocate()
                   { nTSPlayerDeallocate(this.TSPlayer_ID);
                   }

         protected void doClose()
                   { nTSPlayerClose(this.TSPlayer_ID);
                     this.TSPlayer_ID = 0;
                   }

         protected long doSetMediaTime(long now) throws MediaException
                   { return now;
                   }

         protected long doGetMediaTime()
                   { return nTSGetMediaTime(this.TSPlayer_ID);
                   }

         protected long doGetDuration()
                   { return TIME_UNKNOWN;
                   }

         protected Control doGetControl(String type)
                   { if (type.compareTo("javax.microedition.media.control.ToneControl") == 0) return this;
                     System.out.println("ToneSequencePlayer: doGetControl("+type+")");
                     return null;
                   }

         public String getContentType() 
                { chkClosed(true);
                  return "audio/tone-sequence";
                }

         public void run()
                { while(this.state == Player.STARTED)
                       { if (nTSCheckEOM(this.TSPlayer_ID) != 0) 
                            { sendEvent(PlayerListener.END_OF_MEDIA, new Long(this.doGetMediaTime()));
                              return;
                            }
                         Thread.yield();
                       }
                }

         public void setSequence(byte[] sequence)
                { this.TS_Sequence = new byte[sequence.length];
                  for(int i=0; i<sequence.length; i++)
                     this.TS_Sequence[i] = sequence[i];
                  if (nTSPlayerRealize(this.TSPlayer_ID, TS_Sequence) == 0) this.SequenceLoaded = true;
                }

}

