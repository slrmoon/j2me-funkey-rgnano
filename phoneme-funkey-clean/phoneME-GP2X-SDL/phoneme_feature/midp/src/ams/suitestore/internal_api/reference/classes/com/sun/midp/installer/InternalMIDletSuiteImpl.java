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

        separator = classPathExt.indexOf(':');
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
