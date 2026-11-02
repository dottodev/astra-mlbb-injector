package com.astra.injector;

public class LobbyEntry {
    public final int heroId;
    public final int winRate;
    public final String name;

    public LobbyEntry(int heroId, int winRate, String name) {
        this.heroId = heroId;
        this.winRate = winRate;
        this.name = name;
    }
}