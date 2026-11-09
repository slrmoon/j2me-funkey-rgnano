package org.w3c.dom.svg;

import org.w3c.dom.DOMException;

public interface SVGAnimationElement extends SVGElement {
    void beginElementAt(float offset) throws DOMException;

    void endElementAt(float offset) throws DOMException;
}
