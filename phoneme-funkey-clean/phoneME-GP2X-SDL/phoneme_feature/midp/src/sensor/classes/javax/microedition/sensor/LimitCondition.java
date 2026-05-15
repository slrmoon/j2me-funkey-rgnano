package javax.microedition.sensor;

public final class LimitCondition implements Condition {
    private final double limit;
    private final String op;

    public LimitCondition(double limit, String operator) {
        if (!isValidOperator(operator)) {
            throw new IllegalArgumentException("Unsupported operator");
        }

        this.limit = limit;
        this.op = operator;
    }

    public final double getLimit() {
        return limit;
    }

    public final String getOperator() {
        return op;
    }

    public boolean isMet(double value) {
        return compare(value, limit, op);
    }

    public boolean isMet(Object value) {
        return false;
    }

    static boolean isValidOperator(String operator) {
        return OP_EQUALS.equals(operator) ||
            OP_GREATER_THAN.equals(operator) ||
            OP_GREATER_THAN_OR_EQUALS.equals(operator) ||
            OP_LESS_THAN.equals(operator) ||
            OP_LESS_THAN_OR_EQUALS.equals(operator);
    }

    static boolean compare(double value, double limit, String operator) {
        if (OP_EQUALS.equals(operator)) {
            return value == limit;
        } else if (OP_GREATER_THAN.equals(operator)) {
            return value > limit;
        } else if (OP_GREATER_THAN_OR_EQUALS.equals(operator)) {
            return value >= limit;
        } else if (OP_LESS_THAN.equals(operator)) {
            return value < limit;
        } else if (OP_LESS_THAN_OR_EQUALS.equals(operator)) {
            return value <= limit;
        }

        return false;
    }
}
