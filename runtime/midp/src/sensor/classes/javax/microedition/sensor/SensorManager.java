package javax.microedition.sensor;

public final class SensorManager {
    private static final SensorInfo[] SENSORS = {
        new StubSensorInfo()
    };

    private SensorManager() {
    }

    public static SensorInfo[] findSensors(String quantity, String contextType) {
        if ((quantity == null || "acceleration".equals(quantity)) &&
                (contextType == null ||
                SensorInfo.CONTEXT_TYPE_DEVICE.equals(contextType))) {
            return copySensors();
        }

        return new SensorInfo[0];
    }

    public static SensorInfo[] findSensors(String url) {
        if (url == null) {
            throw new NullPointerException();
        }

        if (StubSensorInfo.URL.equals(url)) {
            return copySensors();
        }

        return new SensorInfo[0];
    }

    public static void addSensorListener(SensorListener listener,
            SensorInfo info) {
        if (listener == null || info == null) {
            throw new NullPointerException();
        }
    }

    public static void addSensorListener(SensorListener listener,
            String quantity) {
        if (listener == null || quantity == null) {
            throw new NullPointerException();
        }
    }

    public static void removeSensorListener(SensorListener listener) {
        if (listener == null) {
            throw new NullPointerException();
        }
    }

    public static SensorConnection openStubConnection(String url) {
        if (url == null) {
            throw new NullPointerException();
        }

        return new StubSensorConnection((StubSensorInfo)SENSORS[0]);
    }

    private static SensorInfo[] copySensors() {
        SensorInfo[] sensors = new SensorInfo[SENSORS.length];
        System.arraycopy(SENSORS, 0, sensors, 0, SENSORS.length);
        return sensors;
    }
}

final class StubSensorInfo implements SensorInfo {
    static final String URL = "sensor:acceleration";

    private static final ChannelInfo[] CHANNELS = {
        new StubChannelInfo("axis_x"),
        new StubChannelInfo("axis_y"),
        new StubChannelInfo("axis_z")
    };

    public ChannelInfo[] getChannelInfos() {
        ChannelInfo[] channels = new ChannelInfo[CHANNELS.length];
        System.arraycopy(CHANNELS, 0, channels, 0, CHANNELS.length);
        return channels;
    }

    public int getConnectionType() {
        return CONN_EMBEDDED;
    }

    public String getContextType() {
        return CONTEXT_TYPE_DEVICE;
    }

    public String getDescription() {
        return "Stub acceleration sensor";
    }

    public int getMaxBufferSize() {
        return 1;
    }

    public String getModel() {
        return "stub";
    }

    public Object getProperty(String name) {
        if (PROP_VENDOR.equals(name)) {
            return "phoneME";
        }

        if (PROP_VERSION.equals(name)) {
            return "1.0";
        }

        if (PROP_MAX_RATE.equals(name)) {
            return "0";
        }

        return null;
    }

    public String[] getPropertyNames() {
        return new String[] {
            PROP_VENDOR,
            PROP_VERSION,
            PROP_MAX_RATE
        };
    }

    public String getQuantity() {
        return "acceleration";
    }

    public String getUrl() {
        return URL;
    }

    public boolean isAvailabilityPushSupported() {
        return false;
    }

    public boolean isAvailable() {
        return true;
    }

    public boolean isConditionPushSupported() {
        return false;
    }
}

final class StubChannelInfo implements ChannelInfo {
    private static final MeasurementRange[] RANGES = {
        new MeasurementRange(-1000.0D, 1000.0D, 1.0D)
    };

    private final String name;

    StubChannelInfo(String name) {
        this.name = name;
    }

    public float getAccuracy() {
        return 1.0F;
    }

    public int getDataType() {
        return TYPE_INT;
    }

    public MeasurementRange[] getMeasurementRanges() {
        MeasurementRange[] ranges = new MeasurementRange[RANGES.length];
        System.arraycopy(RANGES, 0, ranges, 0, RANGES.length);
        return ranges;
    }

    public String getName() {
        return name;
    }

    public int getScale() {
        return 0;
    }

    public Unit getUnit() {
        return Unit.getUnit("m/s^2");
    }
}

final class StubSensorConnection implements SensorConnection {
    private final StubSensorInfo info;
    private boolean closed;

    StubSensorConnection(StubSensorInfo info) {
        this.info = info;
    }

    public Channel getChannel(ChannelInfo channelInfo) {
        if (channelInfo == null) {
            throw new NullPointerException();
        }

        return new StubChannel(channelInfo);
    }

    public Data[] getData(int bufferSize) {
        return createData();
    }

    public Data[] getData(int bufferSize, long bufferingPeriod,
            boolean isTimestampIncluded) {
        return createData();
    }

    public SensorInfo getSensorInfo() {
        return info;
    }

    public int getState() {
        return closed ? STATE_CLOSED : STATE_OPENED;
    }

    public void removeDataListener() {
    }

    public void setDataListener(DataListener listener, int bufferSize) {
        if (listener == null) {
            throw new NullPointerException();
        }

        listener.dataReceived(this, createData(), false);
    }

    public void setDataListener(DataListener listener, int bufferSize,
            long bufferingPeriod, boolean isTimestampIncluded) {
        setDataListener(listener, bufferSize);
    }

    public int[] getErrorCodes() {
        return new int[0];
    }

    public String getErrorText(int errorCode) {
        return null;
    }

    public void close() {
        closed = true;
    }

    private Data[] createData() {
        ChannelInfo[] channels = info.getChannelInfos();
        Data[] data = new Data[channels.length];

        for (int i = 0; i < data.length; i++) {
            data[i] = new StubData(channels[i]);
        }

        return data;
    }
}

final class StubData implements Data {
    private final ChannelInfo channelInfo;

    StubData(ChannelInfo channelInfo) {
        this.channelInfo = channelInfo;
    }

    public ChannelInfo getChannelInfo() {
        return channelInfo;
    }

    public double[] getDoubleValues() {
        return new double[] { 0.0D };
    }

    public int[] getIntValues() {
        return new int[] { 0 };
    }

    public Object[] getObjectValues() {
        return new Object[] { null };
    }

    public long getTimestamp(int index) {
        return System.currentTimeMillis();
    }

    public float getUncertainty(int index) {
        return 0.0F;
    }

    public boolean isValid(int index) {
        return index == 0;
    }
}

final class StubChannel implements Channel {
    private static final Condition[] NO_CONDITIONS = new Condition[0];

    private final ChannelInfo channelInfo;

    StubChannel(ChannelInfo channelInfo) {
        this.channelInfo = channelInfo;
    }

    public void addCondition(ConditionListener listener, Condition condition) {
        if (listener == null || condition == null) {
            throw new NullPointerException();
        }
    }

    public ChannelInfo getChannelInfo() {
        return channelInfo;
    }

    public Condition[] getConditions(ConditionListener listener) {
        if (listener == null) {
            throw new NullPointerException();
        }

        return NO_CONDITIONS;
    }

    public String getChannelUrl() {
        return StubSensorInfo.URL + "/" + channelInfo.getName();
    }

    public void removeAllConditions() {
    }

    public void removeCondition(ConditionListener listener,
            Condition condition) {
        if (listener == null || condition == null) {
            throw new NullPointerException();
        }
    }

    public void removeConditionListener(ConditionListener listener) {
        if (listener == null) {
            throw new NullPointerException();
        }
    }
}
