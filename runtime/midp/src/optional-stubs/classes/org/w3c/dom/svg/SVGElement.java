package org.w3c.dom.svg;

import org.w3c.dom.DOMException;
import org.w3c.dom.Element;
import org.w3c.dom.events.EventTarget;

public interface SVGElement extends Element, EventTarget {
    void setId(String id) throws DOMException;

    String getId();

    Element getFirstElementChild();

    Element getNextElementSibling();

    String getTrait(String name) throws DOMException;

    String getTraitNS(String namespaceURI, String name) throws DOMException;

    float getFloatTrait(String name) throws DOMException;

    SVGMatrix getMatrixTrait(String name) throws DOMException;

    SVGRect getRectTrait(String name) throws DOMException;

    SVGPath getPathTrait(String name) throws DOMException;

    SVGRGBColor getRGBColorTrait(String name) throws DOMException;

    void setTrait(String name, String value) throws DOMException;

    void setTraitNS(String namespaceURI, String name, String value) throws DOMException;

    void setFloatTrait(String name, float value) throws DOMException;

    void setMatrixTrait(String name, SVGMatrix matrix) throws DOMException;

    void setRectTrait(String name, SVGRect rect) throws DOMException;

    void setPathTrait(String name, SVGPath path) throws DOMException;

    void setRGBColorTrait(String name, SVGRGBColor color) throws DOMException;
}
