import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Vector;

import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Graphics;
import javax.microedition.media.Control;
import javax.microedition.media.Manager;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.control.ToneControl;
import javax.microedition.media.control.VolumeControl;
import javax.microedition.midlet.MIDlet;

public final class SoundLab extends MIDlet implements CommandListener {
    private SoundLabCanvas canvas;

    public void startApp() {
        if (canvas == null) {
            canvas = new SoundLabCanvas(this);
        }
        Display.getDisplay(this).setCurrent(canvas);
    }

    public void pauseApp() {
    }

    public void destroyApp(boolean unconditional) {
    }

    public void commandAction(Command command, Displayable displayable) {
        if (command == canvas.exitCommand) {
            notifyDestroyed();
        } else if (command == canvas.runCommand) {
            canvas.runTests();
        }
    }
}

final class SoundLabCanvas extends Canvas implements Runnable, PlayerListener {
        final Command runCommand = new Command("Run", Command.OK, 1);
        final Command exitCommand = new Command("Exit", Command.EXIT, 2);
        private final SoundLab midlet;
        private final Vector lines = new Vector();
        private Thread worker;
        private int selected;
        private int singleTest = -1;

        private final String[] testNames = {
            "Manager.playTone",
            "MIDI stream",
            "MIDI locator",
            "WAV stream",
            "WAV locator",
            "ToneControl sequence",
            "VolumeControl MIDI",
            "Nokia frequency skipped",
            "Nokia byte skipped",
            "MP3 stream probe",
            "AMR stream probe",
            "DOOM 0.mid",
            "DOOM 1.mid",
            "DOOM 2.mid",
            "DOOM 3.mid",
            "DOOM 4.mid",
            "DOOM 5.mid",
            "DOOM 6.mid",
            "DOOM 7.mid",
            "DOOM 8.mid",
            "DOOM 9.mid",
            "DOOM 10.mid",
            "DOOM 11.mid",
            "DOOM m.mid",
            "DOOM n.mid",
            "DOOM p.mid"
        };

        private final String[] doomMidiPaths = {
            "/doom_0.mid",
            "/doom_1.mid",
            "/doom_2.mid",
            "/doom_3.mid",
            "/doom_4.mid",
            "/doom_5.mid",
            "/doom_6.mid",
            "/doom_7.mid",
            "/doom_8.mid",
            "/doom_9.mid",
            "/doom_10.mid",
            "/doom_11.mid",
            "/doom_m.mid",
            "/doom_n.mid",
            "/doom_p.mid"
        };

        SoundLabCanvas(SoundLab midlet) {
            this.midlet = midlet;
            addCommand(runCommand);
            addCommand(exitCommand);
            setCommandListener(midlet);
            log("SoundLab ready. Fire/5 = run all, arrows = select.");
        }

        protected void paint(Graphics g) {
            int y;
            int i;
            g.setColor(0);
            g.fillRect(0, 0, getWidth(), getHeight());
            g.setColor(0xffffff);
            g.drawString("SoundLab", 2, 0, Graphics.TOP | Graphics.LEFT);
            y = 14;
            for (i = 0; i < testNames.length && y < getHeight() - 50; i++) {
                g.setColor(i == selected ? 0xffff00 : 0xffffff);
                g.drawString((i == selected ? "> " : "  ") + testNames[i],
                        2, y, Graphics.TOP | Graphics.LEFT);
                y += 12;
            }
            g.setColor(0x88ff88);
            y = getHeight() - 48;
            for (i = Math.max(0, lines.size() - 4); i < lines.size(); i++) {
                g.drawString((String)lines.elementAt(i), 2, y,
                        Graphics.TOP | Graphics.LEFT);
                y += 12;
            }
        }

        protected void keyPressed(int keyCode) {
            int action = getGameAction(keyCode);
            if (action == UP && selected > 0) {
                selected--;
                repaint();
            } else if (action == DOWN && selected < testNames.length - 1) {
                selected++;
                repaint();
            } else if (action == FIRE || keyCode == KEY_NUM5) {
                runTests();
            } else if (keyCode >= KEY_NUM0 && keyCode <= KEY_NUM9) {
                selected = keyCode - KEY_NUM0;
                if (selected >= testNames.length) {
                    selected = testNames.length - 1;
                }
                runOne(selected);
            }
        }

        void runTests() {
            if (worker == null || !worker.isAlive()) {
                singleTest = -1;
                worker = new Thread(this);
                worker.start();
            }
        }

        void runOne(int index) {
            if (worker == null || !worker.isAlive()) {
                singleTest = index;
                worker = new Thread(this);
                worker.start();
            }
        }

        public void run() {
            int i;
            if (singleTest >= 0) {
                selected = singleTest;
                repaint();
                runTest(singleTest);
                singleTest = -1;
                return;
            }
            for (i = 0; i < testNames.length; i++) {
                selected = i;
                repaint();
                runTest(i);
                sleep(450);
            }
            log("all tests finished");
        }

        private void runTest(int index) {
            log("TEST " + index + " " + testNames[index]);
            try {
                switch (index) {
                case 0: testPlayTone(); break;
                case 1: testPlayerStream("/tone.mid", "audio/midi", false); break;
                case 2: testPlayerLocator("/tone.mid", false); break;
                case 3: testPlayerStream("/tone.wav", "audio/x-wav", false); break;
                case 4: testPlayerLocator("/tone.wav", false); break;
                case 5: testToneControl(); break;
                case 6: testPlayerStream("/tone.mid", "audio/midi", true); break;
                case 7: log("Nokia Sound is in separate compatibility test"); break;
                case 8: log("Nokia byte Sound is in separate compatibility test"); break;
                case 9: testPlayerStream("/probe.mp3", "audio/mpeg", false); break;
                case 10: testPlayerStream("/probe.amr", "audio/amr", false); break;
                default:
                    testDoomMidi(index - 11);
                    break;
                }
            } catch (Throwable t) {
                log("FAIL " + t.getClass().getName() + ": " + t.getMessage());
            }
        }

        private void testPlayTone() throws Exception {
            log("playTone note=69 duration=400 volume=100");
            Manager.playTone(69, 400, 100);
            sleep(600);
            log("OK playTone returned");
        }

        private void testPlayerStream(String path, String type, boolean volume)
                throws Exception {
            byte[] data = readResource(path);
            Player player;
            log("createPlayer stream path=" + path + " type=" + type +
                    " bytes=" + data.length);
            player = Manager.createPlayer(new ByteArrayInputStream(data), type);
            inspectPlayer(player);
            player.addPlayerListener(this);
            player.realize();
            inspectPlayer(player);
            if (volume) {
                setVolume(player, 45);
            }
            player.prefetch();
            log("start stream path=" + path);
            player.start();
            sleep(900);
            log("stop stream state=" + player.getState());
            player.stop();
            player.close();
            log("OK stream path=" + path);
        }

        private void testDoomMidi(int index) throws Exception {
            String path;
            if (index < 0 || index >= doomMidiPaths.length) {
                log("bad doom index=" + index);
                return;
            }
            path = doomMidiPaths[index];
            log("doom midi path=" + path);
            testPlayerStream(path, "audio/midi", false);
            sleep(250);
            log("doom repeat path=" + path);
            testPlayerStream(path, "audio/midi", false);
        }

        private void testPlayerLocator(String path, boolean loop)
                throws Exception {
            Player player;
            log("createPlayer locator=" + path);
            player = Manager.createPlayer(path);
            inspectPlayer(player);
            player.realize();
            if (loop) {
                player.setLoopCount(2);
            }
            player.prefetch();
            player.start();
            sleep(900);
            player.stop();
            player.close();
            log("OK locator=" + path);
        }

        private void testToneControl() throws Exception {
            Player player;
            ToneControl tone;
            log("createPlayer locator=device://tone");
            player = Manager.createPlayer("device://tone");
            player.realize();
            inspectPlayer(player);
            tone = (ToneControl)player.getControl(
                    "javax.microedition.media.control.ToneControl");
            if (tone == null) {
                log("ToneControl missing");
            } else {
                byte[] sequence = {
                    ToneControl.VERSION, 1,
                    ToneControl.TEMPO, 30,
                    69, 8,
                    72, 8,
                    ToneControl.SILENCE, 4,
                    76, 12
                };
                log("ToneControl.setSequence len=" + sequence.length);
                tone.setSequence(sequence);
                player.start();
                sleep(900);
                player.stop();
            }
            player.close();
        }

        private void inspectPlayer(Player player) {
            Control[] controls;
            int i;
            log("player contentType=" + player.getContentType() +
                    " state=" + player.getState());
            controls = player.getControls();
            log("controls count=" + controls.length);
            for (i = 0; i < controls.length; i++) {
                log("control[" + i + "]=" + controls[i].getClass().getName());
            }
        }

        private void setVolume(Player player, int level) {
            VolumeControl volume;
            volume = (VolumeControl)player.getControl(
                    "javax.microedition.media.control.VolumeControl");
            if (volume == null) {
                log("VolumeControl missing");
            } else {
                log("VolumeControl old=" + volume.getLevel() +
                        " set=" + level);
                volume.setLevel(level);
            }
        }

        public void playerUpdate(Player player, String event, Object data) {
            log("event=" + event + " data=" + data);
        }

        private byte[] readResource(String path) throws IOException {
            InputStream in = getClass().getResourceAsStream(path);
            ByteArrayOutputStream out;
            byte[] tmp;
            int read;
            if (in == null) {
                throw new IOException("missing resource " + path);
            }
            out = new ByteArrayOutputStream();
            tmp = new byte[256];
            while ((read = in.read(tmp)) >= 0) {
                out.write(tmp, 0, read);
            }
            return out.toByteArray();
        }

        private void log(String message) {
            String line = "SoundLab: " + message;
            System.out.println(line);
            lines.addElement(message);
            while (lines.size() > 24) {
                lines.removeElementAt(0);
            }
            repaint();
        }

        private static void sleep(long millis) {
            try {
                Thread.sleep(millis);
            } catch (InterruptedException e) {
            }
        }
}
