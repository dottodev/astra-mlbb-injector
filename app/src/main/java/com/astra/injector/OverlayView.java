package com.astra.injector;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;

import java.util.ArrayList;
import java.util.List;

public class OverlayView extends View {
    private final Paint boxPaint = new Paint();
    private final Paint textPaint = new Paint();
    private final Paint healthPaint = new Paint();
    private final Paint lobbyPaint = new Paint();
    private final List<Entity> entities = new ArrayList<>();
    private LobbyEntry[] lobby = new LobbyEntry[0];
    private int screenW = 1080, screenH = 1920;

    public OverlayView(Context context, AttributeSet attrs) {
        super(context, attrs);
        boxPaint.setStyle(Paint.Style.STROKE);
        boxPaint.setStrokeWidth(2f);
        boxPaint.setColor(Color.GREEN);
        textPaint.setColor(Color.WHITE);
        textPaint.setTextSize(20f);
        textPaint.setStrokeWidth(1f);
        healthPaint.setColor(Color.RED);
        healthPaint.setStyle(Paint.Style.FILL);
        lobbyPaint.setColor(Color.YELLOW);
        lobbyPaint.setTextSize(16f);
        setWillNotDraw(false);
    }

    public void updateEntities(Entity[] ents) {
        entities.clear();
        if (ents != null) {
            for (Entity e : ents) if (e != null && e.valid != 0) entities.add(e);
        }
    }

    public void updateLobby(LobbyEntry[] l) { lobby = l != null ? l : new LobbyEntry[0]; }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        screenW = w; screenH = h;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // ESP boxes
        for (Entity e : entities) {
            int cx = (int)(e.screenX * screenW);
            int cy = (int)(e.screenY * screenH);
            int half = 40;
            Rect r = new Rect(cx - half, cy - half, cx + half, cy + half);
            boxPaint.setColor(e.team == 0 ? Color.GREEN : Color.RED);
            canvas.drawRect(r, boxPaint);
            canvas.drawText("HP:" + (int)e.health + " H" + e.heroId, cx + half + 4, cy - half + 16, textPaint);
        }
        // Lobby panel (top-right)
        int x = screenW - 220;
        int y = 60;
        canvas.drawText("LOBBY INFO:", x, y, lobbyPaint);
        y += 22;
        for (LobbyEntry le : lobby) {
            canvas.drawText(le.name + "  H" + le.heroId + "  WR:" + le.winRate + "%", x, y, lobbyPaint);
            y += 20;
            if (y > screenH - 40) break;
        }
        invalidate(); // loop
    }
}