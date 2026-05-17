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

public class MIDIPlayer extends ABBBasicPlayer implements Runnable, VolumeControl
       { private native int  nMidiPlayerInit();
         private native int  nMidiPlayerRealize(int id, byte[] buf);
         private native int  nMidiPlayerPrefetch(int id);
         private native int  nMidiPlayerStart(int id);
         private native void nMidiPlayerStop(int id);
         private native void nMidiPlayerDeallocate(int id);
         private native void nMidiPlayerClose(int id);
         private native long nGetMediaTime(int id);
         private native int  nCheckEOM(int id);
         private native void nSetVolumeLevel(int id, int value);
         private native int  nSetLoopCount(int id, int count);
         private Thread      CheckThread;
         private int         MIDIPlayer_ID;
         private int         VolumeLevel;
         
         public MIDIPlayer() throws Exception
                { super();
                  this.MIDIPlayer_ID = nMidiPlayerInit();
                  if (this.MIDIPlayer_ID == 0) throw new Exception("Out Of Memory Error!!!"); 
                  this.CheckThread = null;
                  this.VolumeLevel = 100;
                  System.out.println("MIDIPlayer.<init> id=" + this.MIDIPlayer_ID);
                }
    
         public int getAudioType() 
                { return AUDIO_MIDI;
                }

         protected void doRealize() throws MediaException
                   { try { byte[] Buffer = readAll(this.source);
                           int Ret;
                           System.out.println("MIDIPlayer.doRealize bytes=" + Buffer.length);
                           Ret = nMidiPlayerRealize(this.MIDIPlayer_ID, Buffer);
                           System.out.println("MIDIPlayer.doRealize nativeRet=" + Ret);
                           if (Ret != 0) throw new MediaException("MIDI realize error " + Ret);
                         } catch(IOException E) { throw new MediaException(E.getMessage()); }
                   }

         private byte[] readAll(InputStream in) throws IOException
                   { ByteArrayOutputStream out = new ByteArrayOutputStream();
                     byte[] tmp = new byte[4096];
                     int read;
                     while ((read = in.read(tmp)) != -1)
                        { out.write(tmp, 0, read);
                        }
                     return out.toByteArray();
                   }

         protected void doPrefetch() throws MediaException
                   { int Ret = nMidiPlayerPrefetch(this.MIDIPlayer_ID);
                     System.out.println("MIDIPlayer.doPrefetch nativeRet=" + Ret);
                     if (Ret != 0) throw new MediaException("MIDI Prefetch Error!");
                   }

         protected boolean doStart()
                   { int Ret = nMidiPlayerStart(this.MIDIPlayer_ID);
                     System.out.println("MIDIPlayer.doStart nativeRet=" + Ret);
                     if (Ret == 0) return true;
                     return false;
                   }

         protected void doPostStart() 
                   { if (this.CheckThread == null || !this.CheckThread.isAlive())
                        { this.CheckThread = new Thread(this);
                          this.CheckThread.start();
                        }
                   }
                   
         protected void doStop() throws MediaException
                   { nMidiPlayerStop(this.MIDIPlayer_ID);
                   }

         protected void doDeallocate()
                   { nMidiPlayerDeallocate(this.MIDIPlayer_ID);
                   }

         protected void doClose()
                   { nMidiPlayerClose(this.MIDIPlayer_ID);
                     this.MIDIPlayer_ID = 0;
                   }

         protected void doSetLoopCount(int count)
                   { if (nSetLoopCount(this.MIDIPlayer_ID, count) != 0)
                        throw new IllegalStateException("setLoopCount");
                   }

         protected long doSetMediaTime(long now) throws MediaException
                   { return now;
                   }

         protected long doGetMediaTime()
                   { return nGetMediaTime(this.MIDIPlayer_ID);
                   }

         protected long doGetDuration()
                   { return TIME_UNKNOWN;
                   }

         protected Control doGetControl(String type)
                   { if (type.compareTo("javax.microedition.media.control.VolumeControl") == 0) return this;
                     System.out.println("MIDIPlayer: doGetControl("+type+")");
                     return null;
                   }

         public String getContentType() 
                { chkClosed(true);
                  return "audio/midi";
                }

         public void run()
                { while(this.state == Player.STARTED)
                       { if (nCheckEOM(this.MIDIPlayer_ID) != 0) 
                            { sendEvent(PlayerListener.END_OF_MEDIA, new Long(this.doGetMediaTime()));
                              return;
                            }
                         Thread.yield();
                       }
                }

         public void setMute(boolean mute)
                { if (mute) 
                     { this.VolumeLevel = 0;
                       nSetVolumeLevel(this.MIDIPlayer_ID, this.VolumeLevel);
                     }
                }

         public boolean isMuted()
                { if (this.VolumeLevel == 0) return true;
                  return false;
                }

         public int setLevel(int level)
                { if (level < 0) level = 0;
                  if (level > 100) level = 100;
                  this.VolumeLevel = level;
                  nSetVolumeLevel(this.MIDIPlayer_ID, this.VolumeLevel);
                  return this.VolumeLevel;
                }

         public int getLevel()
                { return this.VolumeLevel;
                }

}
