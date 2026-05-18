package com.nttdocomo.ui;

import com.funkey.doja.DoJaRuntime;

public class IApplication {
    private static IApplication currentApp;
    private String[] args;

    public void start() {
    }

    public void pause() {
    }

    public void destroy() {
    }

    public void resume() {
    }

    public static IApplication getCurrentApp() {
        return currentApp;
    }

    public static void setCurrentApp(IApplication app) {
        currentApp = app;
    }

    public String[] getArgs() {
        if (args == null) {
            String ver = DoJaRuntime.getenv("DOJA_APP_VER");
            String param = DoJaRuntime.getenv("DOJA_APP_PARAM");
            if (ver != null && ver.length() > 0 && param != null && param.length() > 0) {
                args = new String[] { ver, param };
            } else if (ver != null && ver.length() > 0) {
                args = new String[] { ver };
            } else if (param != null && param.length() > 0) {
                args = new String[] { param };
            } else {
                args = new String[0];
            }
        }
        return args;
    }

    public void terminate() {
        if (DoJaRuntime.getMIDlet() != null) {
            DoJaRuntime.getMIDlet().notifyDestroyed();
        }
    }

    public void launch(int type, String[] args) {
        System.out.println("DoJa launch ignored type=" + type);
        if (args != null) {
            for (int i = 0; i < args.length; i++) {
                System.out.println("DoJa launch arg[" + i + "]=" + args[i]);
            }
        }
    }

    public String getSourceURL() {
        String value = DoJaRuntime.getenv("DOJA_SOURCE_URL");
        return value == null ? "" : value;
    }

    public String getParameter(String key) {
        if ("AppParam".equals(key)) {
            return DoJaRuntime.getenv("DOJA_APP_PARAM");
        }
        if ("AppVer".equals(key)) {
            return DoJaRuntime.getenv("DOJA_APP_VER");
        }
        return DoJaRuntime.getenv("DOJA_PARAM_" + key);
    }
}
