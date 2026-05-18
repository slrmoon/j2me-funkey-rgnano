package com.nttdocomo.ui;

public interface MediaImage extends MediaResource {
    void use();

    void dispose();

    Image getImage();
}
