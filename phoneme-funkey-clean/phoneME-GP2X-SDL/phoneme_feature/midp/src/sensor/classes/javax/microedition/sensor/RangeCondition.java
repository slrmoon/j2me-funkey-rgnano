package javax.microedition.sensor;

public final class RangeCondition implements Condition {
    private final double lowerLimit;
    private final String lowerOp;
    private final double upperLimit;
    private final String upperOp;

    public RangeCondition(double lowerLimit, String lowerOp,
            double upperLimit, String upperOp) {
        if (!LimitCondition.isValidOperator(lowerOp) ||
                !LimitCondition.isValidOperator(upperOp)) {
            throw new IllegalArgumentException("Unsupported operator");
        }

        this.lowerLimit = lowerLimit;
        this.lowerOp = lowerOp;
        this.upperLimit = upperLimit;
        this.upperOp = upperOp;
    }

    public final double getLowerLimit() {
        return lowerLimit;
    }

    public final String getLowerOp() {
        return lowerOp;
    }

    public final double getUpperLimit() {
        return upperLimit;
    }

    public final String getUpperOp() {
        return upperOp;
    }

    public boolean isMet(double value) {
        return LimitCondition.compare(value, lowerLimit, lowerOp) &&
            LimitCondition.compare(value, upperLimit, upperOp);
    }

    public boolean isMet(Object value) {
        return false;
    }
}
