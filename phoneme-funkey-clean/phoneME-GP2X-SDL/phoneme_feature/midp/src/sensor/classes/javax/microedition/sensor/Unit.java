package javax.microedition.sensor;

import java.util.Hashtable;

public class Unit {
    private static final Hashtable UNITS = new Hashtable();

    private final String symbol;

    private Unit(String symbol) {
        this.symbol = symbol;
    }

    public static Unit getUnit(String symbol) {
        if (symbol == null) {
            throw new NullPointerException();
        }

        synchronized (UNITS) {
            Unit unit = (Unit)UNITS.get(symbol);

            if (unit == null) {
                unit = new Unit(symbol);
                UNITS.put(symbol, unit);
            }

            return unit;
        }
    }

    public String toString() {
        return symbol;
    }
}
