package org.w3c.dom.events;

public interface EventTarget {
    void addEventListener(String type, EventListener listener, boolean useCapture);

    void removeEventListener(String type, EventListener listener, boolean useCapture);
}
