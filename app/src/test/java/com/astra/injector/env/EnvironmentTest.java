package com.astra.injector.env;

import org.junit.Test;

import static org.junit.Assert.assertFalse;

public class EnvironmentTest {
    @Test
    public void detect_noRootOnDevMachine() {
        EnvResult r = Environment.detect();
        assertFalse("root should be false in test environment", r.root);
    }
}