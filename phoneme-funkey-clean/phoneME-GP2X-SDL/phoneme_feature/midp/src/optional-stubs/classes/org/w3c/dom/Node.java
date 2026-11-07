package org.w3c.dom;

public interface Node {
    String getNamespaceURI();

    String getLocalName();

    Node getParentNode();

    Node appendChild(Node newChild) throws DOMException;

    Node removeChild(Node oldChild) throws DOMException;

    Node insertBefore(Node newChild, Node refChild) throws DOMException;
}
