/*
 * Copyright (c) 2004 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.util.logging;

import java.util.logging.Formatter;
import java.util.logging.LogRecord;


/**
 * EclipseFormatter.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 040913 nsano initial version <br>
 */
public class EclipseFormatter extends Formatter {

    String[] prefixes = { "com", "sun" };

    /** */
    public String format(LogRecord record) {
        StringBuilder sb = new StringBuilder();
        StackTraceElement[] stes = new Throwable().getStackTrace();
        int i = 0;
outer:
        for (; i < stes.length; i++) {
            for (String prefix : prefixes) {
                if (!stes[i].getClassName().startsWith(prefix)) {
                    break outer;
                }
            }
    	}
                    
        sb.append("at ");
        sb.append(stes[i].getClassName());
        sb.append("(");
        sb.append(stes[i].getMethodName());
        sb.append(":");
        sb.append(stes[i].getLineNumber());
        sb.append(")");
        return sb.toString();
    }
}

/* */
