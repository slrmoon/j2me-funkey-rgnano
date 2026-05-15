package com.sun.midp.io.j2me.sensor;

import java.io.IOException;
import javax.microedition.io.Connection;
import javax.microedition.sensor.SensorManager;
import com.sun.cldc.io.ConnectionBaseInterface;

public final class Protocol implements ConnectionBaseInterface {
    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        return SensorManager.openStubConnection("sensor:" + name);
    }
}
