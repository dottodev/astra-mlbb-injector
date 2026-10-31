package com.astra.injector.env;

public final class EnvResult {
    public final boolean root;
    public final ContainerType container;
    public final boolean canAttach;

    public EnvResult(boolean root, ContainerType container, boolean canAttach) {
        this.root = root;
        this.container = container;
        this.canAttach = canAttach;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof EnvResult)) return false;
        EnvResult other = (EnvResult) o;
        return root == other.root && canAttach == other.canAttach && container == other.container;
    }

    @Override
    public int hashCode() {
        int result = Boolean.hashCode(root);
        result = 31 * result + (container == null ? 0 : container.hashCode());
        result = 31 * result + Boolean.hashCode(canAttach);
        return result;
    }

    @Override
    public String toString() {
        return "EnvResult{root=" + root + ", container=" + container + ", canAttach=" + canAttach + "}";
    }
}