/*
 *
 *
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

package com.sun.midp.installer;

import java.io.IOException;
import java.io.InputStream;

import javax.microedition.io.Connector;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midlet.MIDletStateHandler;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.util.Properties;
import com.sun.midp.io.j2me.storage.RandomAccessStream;

/**
 * Implements a the required MIDletSuite functionality needed by the
 * system. The class is only needed for internal romized midlets.
 */
public class InternalMIDletSuiteImpl implements MIDletSuite {
    /** Display name for permission dialogs. */
    private String displayName = null;

    /** The ID of this suite. */
    private int id;

    /** Permissions for this suite. */
    private byte[] permissions;

    /** Flag for trusted suites. If true the system trust icon is displayed. */
    private boolean trusted;

    /** Suite properties for this suite. */
    private Properties properties;

    /**
     * number of midlets in this suite. For a rommized suite assume 1.
     */
    private int numberOfMidlets = 1;

    /**
     * Creates MIDletSuite for rommized MIDlet.
     *
     * @param theDisplayName display name to use in permission dialogs,
     *        and in the MIDlet proxy list
     * @param theId ID to separate this suite's resources from others
     *
     * @return new MIDletSuite object
     */
    public static MIDletSuite create(String theDisplayName, int theId) {
        return new InternalMIDletSuiteImpl(theDisplayName, theId);
    }

    /**
     * Creates MIDletSuite for rommized MIDlet.
     *
     * @param theDisplayName display name to use in permission dialogs,
     *        and in the MIDlet proxy list
     * @param theId unique identifier for this suite
     */
    private InternalMIDletSuiteImpl(String theDisplayName, int theId) {
        if (theDisplayName != null) {
            displayName = theDisplayName;
        } else {
            displayName =
                Resource.getString(ResourceConstants.AMS_SYSTEM_SUITE_NAME);
        }

        id = theId;

        trusted = true;

        permissions =
            (Permissions.forDomain(Permissions.MANUFACTURER_DOMAIN_BINDING))
                [Permissions.CUR_LEVELS];

        properties = new Properties();
        loadRuntimeSuiteProperties();
    }

    /**
     * Populate suite properties for direct classpath/internal suites.
     * In this mode phoneME launches the MIDlet as an internal suite and
     * otherwise leaves getAppProperty() empty, even though classpathext
     * points at a real JAR and a sibling JAD may exist beside it.
     */
    private void loadRuntimeSuiteProperties() {
        String classPathExt = System.getProperty("classpathext");
        String jarPath = normalizeClassPathExt(classPathExt);

        if (jarPath == null || jarPath.length() == 0) {
            return;
        }

        loadJadProperties(deriveJadPath(jarPath));
        loadManifestProperties(jarPath);
        guessPlatform(jarPath);
    }

    /**
     * Games made for one handset check microedition.platform and refuse to
     * run elsewhere (KaoKang wants "Nokia3510i"). When the JAR's classes
     * name exactly one handset model and no platform was given
     * (-Dmicroedition.platform), report that model. Several models are
     * per-handset tweaks (Siberian Strike picks its sound mode, an NPE
     * with the wrong one): keep the generic platform then.
     * -Dmidp.platform.guess=0 turns this off.
     */
    private void guessPlatform(String jarPath) {
        try {
            if (System.getProperty("doja.app.class") != null) {
                /* DoCoMo handsets: "DoCoMoProfile-<DoJa version>" (games
                 * read the digits after "Profile-": Yurepai Jan, Mobage) */
                String ver = jamValue("ProfileVer");
                if (ver != null && ver.startsWith("DoJa-")) {
                    ver = ver.substring(5);
                }
                if (ver == null || ver.length() < 3 || ver.charAt(1) != '.') {
                    ver = "5.1";
                }
                com.sun.cldc.util.PropertyOverrides.set("microedition.profiles",
                                                        "DoCoMoProfile-" + ver.substring(0, 3));
            }
            if (!"j2me/phoneME".equals(System.getProperty("microedition.platform"))
                    || "0".equals(System.getProperty("midp.platform.guess"))) {
                return;
            }
            if (System.getProperty("doja.app.class") != null) {
                /* an i-appli whose .jam names no TargetDevice: a handset
                 * name, as every DoCoMo phone had (games cut the maker
                 * prefix before the series digit: Downtown Special) */
                com.sun.cldc.util.PropertyOverrides.set("microedition.platform", "N905i");
                return;
            }
            String[] names = jarClassNames(jarPath);
            String best = null;
            boolean several = false;
            for (int i = 0; names != null && i < names.length; i++) {
                byte[] b = JarReader.readOwnJarEntry(jarPath, names[i]);
                if (b == null) {
                    continue;
                }
                for (int k = 0; k + 8 < b.length; k++) {
                    String m = modelAt(b, k);
                    if (m == null) {
                        continue;
                    }
                    if (best == null) {
                        best = m;
                    } else if (!best.equals(m)) {
                        several = true;
                    }
                }
            }
            if (best != null && !several) {
                com.sun.cldc.util.PropertyOverrides.set("microedition.platform", best);
                System.out.println("platform: " + best + " (a handset model named in the JAR)");
            }
        } catch (Throwable t) {
            /* keep the default platform */
        }
    }

    /** a value of the i-appli's .jam (passed whole as -Ddoja.jam.hex), or null */
    private static String jamValue(String key) {
        String hex = System.getProperty("doja.jam.hex");
        if (hex == null) {
            return null;
        }
        StringBuffer line = new StringBuffer();
        int n = hex.length() / 2;
        for (int i = 0; i <= n; i++) {
            int c = i < n ? (Character.digit(hex.charAt(2 * i), 16) << 4)
                    | Character.digit(hex.charAt(2 * i + 1), 16) : '\n';
            if (c == '\n' || c == '\r') {
                String l = line.toString();
                line.setLength(0);
                int eq = l.indexOf('=');
                if (eq > 0 && l.substring(0, eq).trim().equals(key)) {
                    return l.substring(eq + 1).trim();
                }
            } else {
                line.append(c < 128 ? (char)c : '?');
            }
        }
        return null;
    }

    /** "Nokia3510i", "SonyEricssonK660" at b[k] (a whole string), or null */
    private static String modelAt(byte[] b, int k) {
        String prefix;
        int digitsMin;
        if (b[k] == 'N' && match(b, k, "Nokia")) {
            prefix = "Nokia";
            digitsMin = 4;
        } else if (b[k] == 'S' && match(b, k, "SonyEricsson")) {
            prefix = "SonyEricsson";
            digitsMin = 3;
        } else {
            return null;
        }
        /* a string of its own in the constant pool: length bytes before */
        if (k > 0 && isAlnum(b[k - 1])) {
            return null;
        }
        int i = k + prefix.length();
        int start = i;
        if (prefix.charAt(0) == 'S') {
            if (i >= b.length || b[i] < 'A' || b[i] > 'Z') {
                return null;
            }
            i++;
            start = i;
        }
        while (i < b.length && b[i] >= '0' && b[i] <= '9') {
            i++;
        }
        int digits = i - start;
        if (digits < digitsMin || digits > 4) {
            return null;
        }
        if (i < b.length && b[i] >= 'a' && b[i] <= 'z') {
            i++;
        }
        if (i < b.length && isAlnum(b[i])) {
            return null;
        }
        return new String(b, k, i - k);
    }

    private static boolean match(byte[] b, int k, String s) {
        if (k + s.length() > b.length) {
            return false;
        }
        for (int i = 0; i < s.length(); i++) {
            if (b[k + i] != s.charAt(i)) {
                return false;
            }
        }
        return true;
    }

    private static boolean isAlnum(byte c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    /** the .class entries of a JAR, from its central directory */
    private static String[] jarClassNames(String jarPath) throws IOException {
        RandomAccessStream stream = new RandomAccessStream();
        try {
            stream.connect(jarPath, Connector.READ);
            int size = stream.getSizeOf();
            int tail = size < 65557 ? size : 65557;
            byte[] t = new byte[tail];
            stream.setPosition(size - tail);
            readFully(stream, t);
            int e = -1;
            for (int i = tail - 22; i >= 0; i--) {
                if (u32(t, i) == 0x06054b50) {
                    e = i;
                    break;
                }
            }
            if (e < 0) {
                return null;
            }
            int count = u16(t, e + 10);
            int cdSize = u32(t, e + 12);
            int cdOff = u32(t, e + 16);
            if (cdSize <= 0 || cdOff < 0 || cdOff + cdSize > size) {
                return null;
            }
            byte[] cd = new byte[cdSize];
            stream.setPosition(cdOff);
            readFully(stream, cd);
            java.util.Vector v = new java.util.Vector();
            int p = 0;
            for (int n = 0; n < count && p + 46 <= cdSize; n++) {
                if (u32(cd, p) != 0x02014b50) {
                    break;
                }
                int nl = u16(cd, p + 28);
                int xl = u16(cd, p + 30);
                int cl = u16(cd, p + 32);
                if (p + 46 + nl > cdSize) {
                    break;
                }
                String name = new String(cd, p + 46, nl);
                if (name.endsWith(".class")) {
                    v.addElement(name);
                }
                p += 46 + nl + xl + cl;
            }
            String[] r = new String[v.size()];
            v.copyInto(r);
            return r;
        } finally {
            try {
                stream.disconnect();
            } catch (IOException ignored) {
            }
        }
    }

    private static void readFully(RandomAccessStream s, byte[] b) throws IOException {
        int off = 0;
        while (off < b.length) {
            int n = s.readBytes(b, off, b.length - off);
            if (n <= 0) {
                throw new IOException("short read");
            }
            off += n;
        }
    }

    private static int u16(byte[] b, int i) {
        return (b[i] & 0xff) | ((b[i + 1] & 0xff) << 8);
    }

    private static int u32(byte[] b, int i) {
        return u16(b, i) | (u16(b, i + 2) << 16);
    }

    /**
     * The JAR's own manifest fills what no JAD gave (on a handset the
     * suite's attributes come from both): games read their settings with
     * getAppProperty ("Period-Speed-ms" in Nokia's Highroller Casino...).
     * Read from the launched JAR itself: a resource lookup finds the
     * system classes' manifest first.
     */
    private void loadManifestProperties(String jarPath) {
        try {
            byte[] mf = JarReader.readOwnJarEntry(jarPath, "META-INF/MANIFEST.MF");
            if (mf == null) {
                return;
            }
            ManifestProperties props = new ManifestProperties();
            props.load(new java.io.ByteArrayInputStream(mf));
            for (int i = 0; i < props.size(); i++) {
                String key = props.getKeyAt(i);
                if (key != null && properties.getProperty(key) == null) {
                    properties.setProperty(key, props.getValueAt(i));
                }
            }
        } catch (Throwable t) {
            /* a broken manifest: keep what the JAD gave */
        }
    }

    /**
     * The runtime passes classpathext as a raw path or a simple path list.
     * Keep only the first entry so we can inspect the primary launched JAR.
     *
     * @param classPathExt value of the classpathext system property
     * @return primary JAR path or null when unavailable
     */
    private String normalizeClassPathExt(String classPathExt) {
        int separator;

        if (classPathExt == null) {
            return null;
        }

        separator = classPathExt.indexOf(';');
        if (separator != -1) {
            classPathExt = classPathExt.substring(0, separator);
        }
        /*
         * ':' separates entries too, but not after a Windows drive letter
         * (C:\games\game.jar): cut there, the .jad was never found and
         * getAppProperty() gave null for everything (Doom RPG: no map).
         */
        separator = classPathExt.indexOf(':');
        if (separator == 1 && classPathExt.length() > 2 &&
                (classPathExt.charAt(2) == '\\' || classPathExt.charAt(2) == '/')) {
            separator = classPathExt.indexOf(':', 2);
        }
        if (separator != -1) {
            classPathExt = classPathExt.substring(0, separator);
        }

        return classPathExt;
    }

    /**
     * Derive a sibling JAD path from the launched JAR path.
     *
     * @param jarPath path to the runtime JAR
     * @return likely JAD path
     */
    private String deriveJadPath(String jarPath) {
        if (jarPath.endsWith(".jar")) {
            return jarPath.substring(0, jarPath.length() - 4) + ".jad";
        }

        if (jarPath.endsWith(".JAR")) {
            return jarPath.substring(0, jarPath.length() - 4) + ".jad";
        }

        return jarPath + ".jad";
    }

    /**
     * Merge properties from a sibling JAD when present. JAD values override
     * defaults for this runtime mode. In our OPK flow the JAD is materialized
     * beside the JAR and already contains the properties we need.
     *
     * @param jadPath path to the probable JAD file
     */
    private void loadJadProperties(String jadPath) {
        RandomAccessStream stream = null;
        InputStream input = null;

        try {
            stream = new RandomAccessStream();
            stream.connect(jadPath, Connector.READ);
            input = stream.openInputStream();

            JadProperties jad = new JadProperties();
            jad.load(input, null);
            mergeProperties(jad);
        } catch (Throwable t) {
            /* Missing JAD is fine; manifest-only suites are still valid. */
        } finally {
            if (input != null) {
                try {
                    input.close();
                } catch (IOException e) {
                    /* Ignore close failure for best-effort metadata loading. */
                }
            }

            if (stream != null) {
                try {
                    stream.disconnect();
                } catch (IOException e) {
                    /* Ignore close failure for best-effort metadata loading. */
                }
            }
        }
    }

    /**
     * Copy loaded metadata into the suite property bag.
     *
     * @param source source properties to merge
     */
    private void mergeProperties(Properties source) {
        for (int i = 0; i < source.size(); i++) {
            properties.setProperty(source.getKeyAt(i), source.getValueAt(i));
        }
    }

    /**
     * Provides the number of MIDlets in this suite.
     *
     * @return number of MIDlet in the suite
     */
    public int getNumberOfMIDlets() {
        return numberOfMidlets;
    }

    /**
     * Get the name of a MIDlet to display to the user.
     *
     * @param className class name of the MIDlet to be checked
     *
     * @return name to display to the user
     */
    public String getMIDletName(String className) {
        /*
         * Each internal MIDlet runs in it own suite,
         * just return the suite name
         */
        return displayName;
    }

    /**
     * Gets the unique ID of the suite.
     *
     * @return suite ID
     */
    public int getID() {
        return id;
    }

    /**
     * Gets a property of the suite. A property is an attribute from
     * either the application descriptor or JAR Manifest.
     *
     * @param key the name of the property
     * @return A string with the value of the property.
     *    <code>null</code> is returned if no value is available for
     *          the key.
     */
    public String getProperty(String key) {
        return properties.getProperty(key);
    }

    /**
     * Replace or add a property to the suite for this run only.
     *
     * @param token token with the AMS permission set to allowed,
     *        can be null to use the suite's permission
     * @param key the name of the property
     * @param value the value of the property
     *
     * @exception SecurityException if the caller's token does not have
     *            internal AMS permission
     */
    public void setTempProperty(SecurityToken token, String key,
            String value) {
        if (token != null) {
            token.checkIfPermissionAllowed(Permissions.AMS);
        } else {
            MIDletSuite current = MIDletStateHandler.
                getMidletStateHandler().getMIDletSuite();

            current.checkIfPermissionAllowed(Permissions.AMS);
        }

        properties.setProperty(key, value);
    }

    /**
     * Checks to see the suite has the ALLOW level for specific permission.
     * This is used for by internal APIs that only provide access to
     * trusted system applications.
     *
     * @param permission permission ID from com.sun.midp.security.Permissions
     *
     * @exception SecurityException if the suite is not allowed the permission
     */
    public void checkIfPermissionAllowed(int permission) {
        if (checkPermission(permission) != 1) {
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }
    }

    /**
     * Check for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkForPermission(int permission, String resource)
            throws InterruptedException {
        checkForPermission(permission, resource, null);
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     *
     * @param permission ID of the permission to check for,
     *      the ID must be from
     *      {@link com.sun.midp.security.Permissions}
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     * @param extraValue string to insert into the question,
     *        can be null if no %3 in the question
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkForPermission(int permission, String resource,
            String extraValue) throws InterruptedException {
        checkIfPermissionAllowed(permission);
    }

    /**
     * Gets the status of the specified permission.
     * If no API on the device defines the specific permission
     * requested then it must be reported as denied.
     * If the status of the permission is not known because it might
     * require a user interaction then it should be reported as unknown.
     *
     * @param permission to check if denied, allowed, or unknown
     *
     * @return 0 if the permission is denied; 1 if the permission is allowed;
     *  -1 if the status is unknown
     */
    public int checkPermission(String permission) {
        for (int i = 0; i < Permissions.NUMBER_OF_PERMISSIONS; i++) {
            if (Permissions.getName(i).equals(permission)) {
                return checkPermission(i);

            }
        }

        return 0;
    }

    /**
     * Check to see the suite has the ALLOW level for specific permission.
     * This is used for by internal APIs that only provide access to
     * trusted system applications.
     *
     * @param permission permission ID from com.sun.midp.security.Permissions
     *
     * @param permission to check if denied, allowed, or unknown
     *
     * @return 0 if the permission is denied; 1 if the permission is allowed;
     *  -1 if the status is unknown
     */
    private int checkPermission(int permission) {
        if (permission < 0 || permission >= permissions.length) {
            // report denied
            return 0;
        }

        switch (permissions[permission]) {
        case Permissions.ALLOW:
        case Permissions.BLANKET_GRANTED:
            // report allowed
            return 1;

        case Permissions.BLANKET:
        case Permissions.SESSION:
        case Permissions.ONESHOT:
            // report unknown
            return -1;

        default:
            break;
        }

        // report denied
        return 0;
    }

    /**
     * Indicates if the named MIDlet is registered in the suite
     * with MIDlet-&lt;n&gt; record in the manifest or
     * application descriptor.
     *
     * @param midletClassName class name of the MIDlet to be checked
     *
     * @return true if the MIDlet is registered
     */
    public boolean isRegistered(String midletClassName) {
        // Rommized MIDlets don't have JAD MIDlet-n entries.
        return false;
    }

    /**
     * Indicates if this suite is trusted.
     * (not to be confused with a domain named "trusted",
     * this is used for extra checks beyond permission checking)
     *
     * @return true if the suite is trusted false if not
     */
    public boolean isTrusted() {
        return trusted;
    }

    /**
     * Get state of classes preverification within the suite.
     *
     * @return true because internal suite should be always preverified.
     */
    public boolean isVerified() {
        return true;
    }

    /**
     * Determine if the a MIDlet from this suite can be run. Note that
     * disable suites can still have their settings changed and their
     * install info displayed.
     *
     * @return true if suite is enabled, false otherwise
     */
    public boolean isEnabled() {
        return true;
    }

    /**
     * Asks the user want to interrupt the current MIDlet with
     * a new MIDlet that has received network data.
     *
     * @param connection connection to place in the permission question or
     *        null for alarm
     *
     * @return true if the use wants interrupt the current MIDlet, else false
     */
    public boolean permissionToInterrupt(String connection) {
        // Rommized internal MIDlet can interrupt other MIDlets without asking.
        return true;
    }

    /**
     * Gets push setting for interrupting other MIDlets.
     * Reuses the Permissions.
     *
     * @return push setting for interrupting MIDlets the value
     *        will be permission level from {@link Permissions}
     */
    public byte getPushInterruptSetting() {
        // Rommized internal MIDlet can interrupt other MIDlets without asking.
        return Permissions.ALLOW;
    }

    /**
     * Gets push options for this suite.
     *
     * @return push options are defined in {@link PushRegistryImpl}
     */
    public int getPushOptions() {
        // There are not push options for rommized suites.
        return 0;
    }

    /**
     * Gets list of permissions for this suite.
     *
     * @return array of permissions from {@link Permissions}
     */
    public byte[] getPermissions() {
        return permissions;
    }

    /**
     * Close the opened MIDletSuite
     */
    public void close() {
    }
}
