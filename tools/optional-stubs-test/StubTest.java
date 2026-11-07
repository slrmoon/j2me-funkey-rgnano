import java.io.ByteArrayInputStream;
import java.io.IOException;

import javax.bluetooth.BluetoothStateException;
import javax.bluetooth.DeviceClass;
import javax.bluetooth.DiscoveryListener;
import javax.bluetooth.LocalDevice;
import javax.bluetooth.RemoteDevice;
import javax.bluetooth.ServiceRecord;
import javax.bluetooth.UUID;
import javax.microedition.io.Connector;
import javax.microedition.io.file.FileSystemRegistry;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.lcdui.Canvas;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Graphics;
import javax.microedition.location.Coordinates;
import javax.microedition.location.LocationException;
import javax.microedition.location.LocationProvider;
import javax.microedition.m2g.ScalableGraphics;
import javax.microedition.m2g.ScalableImage;
import javax.microedition.midlet.MIDlet;
import javax.microedition.pim.PIM;
import javax.microedition.pim.PIMException;

/**
 * Optional JSR stubs (JSR 75, 82, 179, 226, 239): a game class that
 * implements a Bluetooth interface loads, and every entry point refuses
 * the way games expect. Prints "StubTest: PASS" or the failed checks.
 */
public class StubTest extends MIDlet implements DiscoveryListener {
    private final StringBuffer log = new StringBuffer();
    private int failed;

    private void check(String what, boolean ok) {
        String line = (ok ? "ok   " : "FAIL ") + what;
        System.out.println("StubTest: " + line);
        log.append(line).append('\n');
        if (!ok) {
            failed++;
        }
    }

    protected void startApp() {
        String[] props = { "microedition.io.file.FileConnection.version", "microedition.pim.version",
            "bluetooth.api.version", "microedition.location.version", "microedition.m2g.version" };
        for (int i = 0; i < props.length; i++) {
            check(props[i] + " unset", System.getProperty(props[i]) == null);
        }

        /* JSR 75 */
        check("listRoots empty", !FileSystemRegistry.listRoots().hasMoreElements());
        try {
            Connector.open("file:///root1/save.dat");
            check("file: not found", false);
        } catch (IOException e) {
            check("file: not found (" + e.getClass().getName() + ")", true);
        }
        PIM pim = PIM.getInstance();
        check("PIM lists empty", pim.listPIMLists(PIM.CONTACT_LIST).length == 0);
        try {
            pim.openPIMList(PIM.CONTACT_LIST, PIM.READ_ONLY);
            check("PIM open refused", false);
        } catch (PIMException e) {
            check("PIM open refused", e.getReason() == PIMException.FEATURE_NOT_SUPPORTED);
        }

        /* JSR 82 */
        try {
            LocalDevice.getLocalDevice();
            check("Bluetooth off", false);
        } catch (BluetoothStateException e) {
            check("Bluetooth off", true);
        }
        check("isPowerOn false", !LocalDevice.isPowerOn());
        check("UUID 0x1101", new UUID(0x1101).equals(new UUID("1101", true))
                && new UUID(0x1101).toString().equals("110100001000800000805F9B34FB"));
        try {
            Connector.open("btspp://localhost:" + new UUID(0x1101) + ";name=game");
            check("btspp: not found", false);
        } catch (IOException e) {
            check("btspp: not found", true);
        }

        /* JSR 179 */
        try {
            LocationProvider.getInstance(null);
            check("location unavailable", false);
        } catch (LocationException e) {
            check("location unavailable", true);
        }
        float d = new Coordinates(55.7558, 37.6173, 0).distance(new Coordinates(59.9343, 30.3351, 0));
        check("Moscow-St Petersburg " + (int) (d / 1000) + " km", d > 625000 && d < 640000);
        float az = new Coordinates(0, 0, 0).azimuthTo(new Coordinates(0, 1, 0));
        check("azimuth east " + az, az > 89.9f && az < 90.1f);

        /* JSR 226 */
        try {
            ScalableImage.createImage(new ByteArrayInputStream("<svg/>".getBytes()), null);
            check("SVG refused", false);
        } catch (IOException e) {
            check("SVG refused", true);
        }
        check("ScalableGraphics", ScalableGraphics.createInstance() != null);

        /* JSR 239 */
        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        check("EGL no display", dpy == EGL10.EGL_NO_DISPLAY);
        check("eglInitialize fails", !egl.eglInitialize(dpy, new int[2]));
        java.nio.ByteBuffer bb = java.nio.ByteBuffer.allocateDirect(16);
        bb.putFloat(1.5f).putShort((short) -2).putInt(0x01020304);
        bb.position(0);
        java.nio.FloatBuffer fb = bb.asFloatBuffer();
        check("nio float view", fb.get(0) == 1.5f && bb.getShort(4) == -2 && bb.get(6) == 4
                && bb.getInt(6) == 0x01020304);
        java.nio.ShortBuffer sb = java.nio.ShortBuffer.wrap(new short[] { 1, 2, 3, 4 }, 1, 2);
        check("nio wrap/slice", sb.remaining() == 2 && sb.slice().get(0) == 2 && sb.hasArray());

        System.out.println(failed == 0 ? "StubTest: PASS" : "StubTest: " + failed + " FAILED");
        final String text = log.toString() + (failed == 0 ? "PASS" : failed + " FAILED");
        Display.getDisplay(this).setCurrent(new Canvas() {
            protected void paint(Graphics g) {
                g.setColor(0);
                g.fillRect(0, 0, getWidth(), getHeight());
                g.setColor(failed == 0 ? 0x80ff80 : 0xff8080);
                int y = 0;
                int s = 0;
                for (int i = 0; i <= text.length(); i++) {
                    if (i == text.length() || text.charAt(i) == '\n') {
                        g.drawString(text.substring(s, i), 1, y, Graphics.TOP | Graphics.LEFT);
                        y += 10;
                        s = i + 1;
                    }
                }
            }
        });
    }

    public void deviceDiscovered(RemoteDevice btDevice, DeviceClass cod) {
    }

    public void servicesDiscovered(int transID, ServiceRecord[] servRecord) {
    }

    public void serviceSearchCompleted(int transID, int respCode) {
    }

    public void inquiryCompleted(int discType) {
    }

    protected void pauseApp() {
    }

    protected void destroyApp(boolean unconditional) {
    }
}
