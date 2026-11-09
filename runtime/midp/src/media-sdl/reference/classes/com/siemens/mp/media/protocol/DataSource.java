/* Siemens media API (pre-MMAPI): data sources are not supported. */
package com.siemens.mp.media.protocol;

public abstract class DataSource {
    private final String locator;

    public DataSource(String locator) {
        this.locator = locator;
    }

    public String getLocator() {
        return locator;
    }
}
