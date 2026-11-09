package org.w3c.dom;

public interface Element extends Node {
    String getAttributeNS(String namespaceURI, String localName) throws DOMException;

    void setAttributeNS(String namespaceURI, String qualifiedName, String value)
            throws DOMException;
}
