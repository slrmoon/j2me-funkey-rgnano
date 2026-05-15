package javax.microedition.sensor;

public interface ConditionListener {
    public void conditionMet(SensorConnection sensor, Data data, Condition condition);
}
