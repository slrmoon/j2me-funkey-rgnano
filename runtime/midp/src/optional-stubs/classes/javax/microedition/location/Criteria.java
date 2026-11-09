package javax.microedition.location;

public class Criteria {
    public static final int NO_REQUIREMENT = 0;
    public static final int POWER_USAGE_LOW = 1;
    public static final int POWER_USAGE_MEDIUM = 2;
    public static final int POWER_USAGE_HIGH = 3;

    private int horizontalAccuracy = NO_REQUIREMENT;
    private int verticalAccuracy = NO_REQUIREMENT;
    private int responseTime = NO_REQUIREMENT;
    private int powerConsumption = NO_REQUIREMENT;
    private boolean costAllowed = true;
    private boolean speedRequired;
    private boolean altitudeRequired;
    private boolean addressInfoRequired;

    public Criteria() {
    }

    public int getPreferredPowerConsumption() {
        return powerConsumption;
    }

    public boolean isAllowedToCost() {
        return costAllowed;
    }

    public int getVerticalAccuracy() {
        return verticalAccuracy;
    }

    public int getHorizontalAccuracy() {
        return horizontalAccuracy;
    }

    public int getPreferredResponseTime() {
        return responseTime;
    }

    public boolean isSpeedAndCourseRequired() {
        return speedRequired;
    }

    public boolean isAltitudeRequired() {
        return altitudeRequired;
    }

    public boolean isAddressInfoRequired() {
        return addressInfoRequired;
    }

    public void setHorizontalAccuracy(int accuracy) {
        horizontalAccuracy = accuracy;
    }

    public void setVerticalAccuracy(int accuracy) {
        verticalAccuracy = accuracy;
    }

    public void setPreferredResponseTime(int time) {
        responseTime = time;
    }

    public void setPreferredPowerConsumption(int level) {
        powerConsumption = level;
    }

    public void setCostAllowed(boolean costAllowed) {
        this.costAllowed = costAllowed;
    }

    public void setSpeedAndCourseRequired(boolean speedAndCourseRequired) {
        speedRequired = speedAndCourseRequired;
    }

    public void setAltitudeRequired(boolean altitudeRequired) {
        this.altitudeRequired = altitudeRequired;
    }

    public void setAddressInfoRequired(boolean addressInfoRequired) {
        this.addressInfoRequired = addressInfoRequired;
    }
}
