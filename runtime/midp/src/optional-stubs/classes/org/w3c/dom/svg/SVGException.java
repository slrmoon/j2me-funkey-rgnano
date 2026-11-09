package org.w3c.dom.svg;

public class SVGException extends RuntimeException {
    public static final short SVG_WRONG_TYPE_ERR = 0;
    public static final short SVG_INVALID_VALUE_ERR = 1;
    public static final short SVG_MATRIX_NOT_INVERTABLE = 2;

    public short code;

    public SVGException(short code, String message) {
        super(message);
        this.code = code;
    }
}
