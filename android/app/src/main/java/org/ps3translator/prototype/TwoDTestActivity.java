package org.ps3translator.prototype;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.view.MotionEvent;
import android.view.View;

/** Android-only 2D smoke test; never loads PS3 game files. */
public final class TwoDTestActivity extends Activity {
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        setContentView(new TestScene());
    }

    private final class TestScene extends View {
        private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final float d = getResources().getDisplayMetrics().density;
        private long window = System.nanoTime();
        private int frames, fps, score, misses;
        private float px = .5f, py = .72f, tx = .72f, ty = .35f;

        TestScene() { super(TwoDTestActivity.this); }

        @Override protected void onDraw(Canvas c) {
            super.onDraw(c);
            long now = System.nanoTime();
            frames++;
            if (now - window >= 1_000_000_000L) {
                fps = frames;
                frames = 0;
                window = now;
            }
            int w = getWidth(), h = getHeight();
            c.drawColor(Color.rgb(16, 23, 39));
            p.setColor(Color.rgb(35, 49, 75));
            for (int x = 0; x < w; x += Math.max(1, (int)(32*d))) c.drawLine(x,0,x,h,p);
            for (int y = 0; y < h; y += Math.max(1, (int)(32*d))) c.drawLine(0,y,w,y,p);
            p.setColor(Color.WHITE);
            p.setTextSize(20*d);
            c.drawText("2D DEVICE TEST",18*d,34*d,p);
            p.setTextSize(14*d);
            c.drawText("FPS: "+fps+"  Score: "+score+"  Misses: "+misses,18*d,58*d,p);
            c.drawText("Tap target; drag to move the green player.",18*d,h-24*d,p);
            p.setColor(Color.rgb(255,190,70));
            c.drawCircle(tx*w,ty*h,22*d,p);
            p.setColor(Color.rgb(255,235,170));
            c.drawCircle(tx*w,ty*h,9*d,p);
            p.setColor(Color.rgb(70,220,180));
            c.drawCircle(px*w,py*h,24*d,p);
            p.setColor(Color.WHITE);
            c.drawCircle(px*w,py*h,8*d,p);
            postInvalidateDelayed(16);
        }

        @Override public boolean onTouchEvent(MotionEvent e) {
            float x = e.getX()/Math.max(1,getWidth());
            float y = e.getY()/Math.max(1,getHeight());
            if (e.getAction()==MotionEvent.ACTION_DOWN || e.getAction()==MotionEvent.ACTION_MOVE) {
                px=Math.max(.03f,Math.min(.97f,x));
                py=Math.max(.08f,Math.min(.90f,y));
                if (e.getAction()==MotionEvent.ACTION_DOWN) {
                    if (Math.hypot((px-tx)*getWidth(),(py-ty)*getHeight()) < 65*d) {
                        score++;
                        tx=.08f+(float)Math.random()*.84f;
                        ty=.12f+(float)Math.random()*.66f;
                    } else misses++;
                }
                invalidate();
                return true;
            }
            return true;
        }
    }
}
