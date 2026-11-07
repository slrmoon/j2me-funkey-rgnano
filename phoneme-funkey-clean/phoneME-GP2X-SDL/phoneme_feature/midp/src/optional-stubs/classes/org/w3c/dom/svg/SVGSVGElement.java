package org.w3c.dom.svg;

import org.w3c.dom.DOMException;

public interface SVGSVGElement extends SVGLocatableElement {
    void setCurrentScale(float value) throws DOMException;

    float getCurrentScale();

    void setCurrentRotate(float value);

    float getCurrentRotate();

    SVGPoint getCurrentTranslate();

    float getCurrentTime();

    void setCurrentTime(float seconds);

    SVGMatrix createSVGMatrixComponents(float a, float b, float c, float d, float e, float f);

    SVGRect createSVGRect();

    SVGPath createSVGPath();

    SVGRGBColor createSVGRGBColor(int red, int green, int blue) throws SVGException;
}
