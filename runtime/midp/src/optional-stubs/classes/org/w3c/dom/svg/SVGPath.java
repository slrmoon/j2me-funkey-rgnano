package org.w3c.dom.svg;

import org.w3c.dom.DOMException;

public interface SVGPath {
    short MOVE_TO = 77;
    short LINE_TO = 76;
    short CURVE_TO = 67;
    short QUAD_TO = 81;
    short CLOSE = 90;

    int getNumberOfSegments();

    short getSegment(int cmdIndex) throws DOMException;

    float getSegmentParam(int cmdIndex, int paramIndex) throws DOMException;

    void moveTo(float x, float y);

    void lineTo(float x, float y);

    void quadTo(float x1, float y1, float x2, float y2);

    void curveTo(float x1, float y1, float x2, float y2, float x3, float y3);

    void close();
}
