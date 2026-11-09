package org.w3c.dom;

public interface Document extends Node {
    Element createElementNS(String namespaceURI, String qualifiedName) throws DOMException;

    Element getDocumentElement();

    Element getElementById(String id);
}
