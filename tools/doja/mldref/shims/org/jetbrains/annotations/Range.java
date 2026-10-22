package org.jetbrains.annotations; import java.lang.annotation.*; @Retention(RetentionPolicy.CLASS) public @interface Range { long from() default 0; long to() default 0; }
