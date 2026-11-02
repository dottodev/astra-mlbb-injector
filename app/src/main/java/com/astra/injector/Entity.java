package com.astra.injector;

public class Entity {
    public final float screenX, screenY;
    public final float health;
    public final int team;
    public final int heroId;
    public final int valid;

    public Entity(float sx, float sy, float health, int team, int heroId) {
        this(sx, sy, health, team, heroId, 1);
    }

    public Entity(float sx, float sy, float health, int team, int heroId, int valid) {
        this.screenX = sx;
        this.screenY = sy;
        this.health = health;
        this.team = team;
        this.heroId = heroId;
        this.valid = valid;
    }
}