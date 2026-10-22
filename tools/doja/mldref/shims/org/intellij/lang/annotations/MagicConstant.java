package org.intellij.lang.annotations; public @interface MagicConstant { int[] intValues() default {}; long[] longValues() default {}; Class<?> valuesFromClass() default Object.class; }
