package javax.microedition.sensor;

public interface Channel {
    public void addCondition(ConditionListener listener, Condition condition);
    public ChannelInfo getChannelInfo();
    public Condition[] getConditions(ConditionListener listener);
    public String getChannelUrl();
    public void removeAllConditions();
    public void removeCondition(ConditionListener listener, Condition condition);
    public void removeConditionListener(ConditionListener listener);
}
