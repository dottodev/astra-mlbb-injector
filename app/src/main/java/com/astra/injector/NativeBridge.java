package com.astra.injector;

public class NativeBridge {
    static { System.loadLibrary("core"); }
    public static native void init();
    public static native boolean start(String pkg, String version);
    public static native void stop();
    public static native Entity[] getFrame();
    public static native LobbyEntry[] getLobby();
}