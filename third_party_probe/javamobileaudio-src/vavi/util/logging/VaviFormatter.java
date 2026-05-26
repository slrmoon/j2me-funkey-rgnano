/*
 * Copyright (c) 2002 by Naohide Sano, All rights reserved.
 *
 * Programmed by Naohide Sano
 */

package vavi.util.logging;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.logging.Formatter;
import java.util.logging.LogRecord;

import vavi.util.StringUtil;


/**
 * VaviFormatter.
 * 
 * @author <a href="mailto:vavivavi@yahoo.co.jp">Naohide Sano</a> (nsano)
 * @version 0.00 021027 nsano initial version <br>
 *          0.01 031220 nsano clean imports <br>
 */
public class VaviFormatter extends Formatter {

    /** */
    private static final DateFormat sdf = new SimpleDateFormat("yyyy-MM-dd hh:mm:ss,SSS");
    
    /** */
    public String format(LogRecord record) {
        StringBuilder sb = new StringBuilder();
        if (record.getThrown() != null) {
            sb.append(record.getThrown());
            sb.append("\n");
            StackTraceElement[] stes = record.getThrown().getStackTrace();
            for (int i = 0; i < stes.length; i++) {
                sb.append("\tat ");
                sb.append(stes[i]);
                sb.append("\n");
            }
            return sb.toString();
        } else {
            StackTraceElement[] stes = new Exception().getStackTrace();
            StackTraceElement ste = null;
            for (int i = 0; i < stes.length; i++) {
                if (!stes[i].getClassName().startsWith("vavi.util.logging") &&
                    !stes[i].getClassName().startsWith("java.util.logging") &&
                    !stes[i].getClassName().startsWith("vavi.util.Debug")) {
                    ste = stes[i];
                    break;
                }
            }
            if (ste != null) {
                sb.append(sdf.format(new Date()));
                sb.append(" [");
                sb.append(record.getLevel());
                sb.append("] ");
                sb.append(record.getMessage().replaceAll("\n$", ""));
                sb.append("\n");
                sb.append("\tat ");
                sb.append(ste.getClassName());
                sb.append(".");
                sb.append(ste.getMethodName());
                sb.append("(");
                sb.append(ste.getFileName());
                sb.append(":");
                sb.append(ste.getLineNumber());
                sb.append(")");
                sb.append("\n");
            } else {
                sb.append(StringUtil.getClassName(record.getSourceClassName()));
                sb.append("::");
                sb.append(record.getSourceMethodName());
                sb.append(": ");
                sb.append(record.getMessage());
            }
            return sb.toString();
        }
    }
}

/* */
