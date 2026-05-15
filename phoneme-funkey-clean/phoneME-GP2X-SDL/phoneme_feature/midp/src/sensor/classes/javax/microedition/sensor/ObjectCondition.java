package javax.microedition.sensor;

public final class ObjectCondition implements Condition {
    private final Object limit;

    public ObjectCondition(Object limit) {
        if (limit == null) {
            throw new NullPointerException();
        }

        this.limit = limit;
    }

    public final Object getLimit() {
        return limit;
    }

    public boolean isMet(double value) {
        return false;
    }

    public boolean isMet(Object value) {
        return limit.equals(value);
    }
}
