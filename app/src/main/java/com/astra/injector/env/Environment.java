package com.astra.injector.env;

import java.io.File;
import java.util.concurrent.TimeUnit;

public final class Environment {
    private Environment() {
    }

    public static EnvResult detect() {
        boolean root = probeRoot();
        ContainerType container = detectContainer();
        boolean canAttach = root || container != null;
        return new EnvResult(root, container, canAttach);
    }

    private static boolean probeRoot() {
        if (Boolean.getBoolean("astra.forceNoRoot")) {
            return false;
        }
        Process process = null;
        try {
            process = new ProcessBuilder("su", "-c", "id")
                    .redirectErrorStream(true)
                    .start();
            boolean finished = process.waitFor(2, TimeUnit.SECONDS);
            if (!finished) {
                process.destroyForcibly();
                return false;
            }
            return process.exitValue() == 0;
        } catch (Exception e) {
            if (process != null) {
                process.destroyForcibly();
            }
            return false;
        }
    }

    private static ContainerType detectContainer() {
        // VMOS
        if (anyExists(new String[]{
                "/data/data/com.vmos.pro",
                "/data/user/0/com.vmos.pro",
                "/sdcard/vmos"
        }) || envContains("VMOS")) {
            return ContainerType.VMOS;
        }
        // Parallel Space
        if (anyExists(new String[]{
                "/data/data/com.parallel.space",
                "/data/user/0/com.parallel.space",
                "/data/data/com.lbe.parallel"
        }) || envContains("PARALLEL")) {
            return ContainerType.PARALLEL_SPACE;
        }
        // VirtualApp / Lody Virtual
        if (anyExists(new String[]{
                "/data/data/io.virtualapp",
                "/data/user/0/io.virtualapp",
                "/data/data/com.lody.virtual",
                "/data/user/0/com.lody.virtual"
        }) || envContains("VIRTUALAPP")) {
            return ContainerType.VIRTUAL_APP;
        }
        return null;
    }

    private static boolean anyExists(String[] paths) {
        for (String p : paths) {
            if (new File(p).exists()) {
                return true;
            }
        }
        return false;
    }

    private static boolean envContains(String needle) {
        for (String key : System.getenv().keySet()) {
            if (key != null && key.toUpperCase().contains(needle)) {
                return true;
            }
        }
        // also inspect values
        for (String v : System.getenv().values()) {
            if (v != null && v.toUpperCase().contains(needle)) {
                return true;
            }
        }
        return false;
    }
}