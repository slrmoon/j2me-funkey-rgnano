package org.w3c.dom.svg;

import org.w3c.dom.DOMException;

public interface SVGMatrix {
    float getComponent(int index) throws DOMException;

    SVGMatrix mMultiply(SVGMatrix secondMatrix);

    SVGMatrix inverse() throws SVGException;

    SVGMatrix mTranslate(float x, float y);

    SVGMatrix mScale(float scaleFactor);

    SVGMatrix mRotate(float angle);
}
