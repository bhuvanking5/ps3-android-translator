package org.ps3translator.prototype;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/** Android OpenGL ES 1.x 3D smoke test; not a PS3 game/emulator. */
public final class ThreeDTestActivity extends Activity {
    private CubeView cube;
    private TextView overlay;

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        FrameLayout root = new FrameLayout(this);
        cube = new CubeView();
        root.addView(cube);
        overlay = new TextView(this);
        overlay.setTextColor(0xffffffff);
        overlay.setTextSize(16);
        overlay.setPadding(18, 18, 18, 18);
        overlay.setText("3D DEVICE TEST\nOpenGL ES rotating cube\nTap to change rotation speed");
        root.addView(overlay);
        setContentView(root);
    }

    @Override protected void onResume() { super.onResume(); if (cube != null) cube.onResume(); }
    @Override protected void onPause() { if (cube != null) cube.onPause(); super.onPause(); }

    private final class CubeView extends GLSurfaceView {
        private final CubeRenderer renderer;
        CubeView() {
            super(ThreeDTestActivity.this);
            setEGLContextClientVersion(1);
            renderer = new CubeRenderer();
            setRenderer(renderer);
            setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        }
        @Override public boolean onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                renderer.speed = renderer.speed >= 2.0f ? 0.35f : renderer.speed + 0.45f;
                overlay.setText("3D DEVICE TEST\nOpenGL ES rotating cube\nRotation speed: " + renderer.speed + "x\nTap to change speed");
                return true;
            }
            return true;
        }
    }

    private static final class CubeRenderer implements GLSurfaceView.Renderer {
        private final FloatBuffer vertices;
        private final FloatBuffer colors;
        private float angle;
        volatile float speed = 1.0f;
        private long lastNs = System.nanoTime();
        private long fpsWindow = lastNs;
        private int frames;
        private volatile int fps;

        private static final float[] V = {
            -1,-1,-1,  1,-1,-1,  1,1,-1, -1,1,-1,
            -1,-1, 1,  1,-1, 1,  1,1, 1, -1,1, 1
        };
        private static final float[] C = {
            1,0.2f,0.2f,1, 0.2f,1,0.3f,1, 0.2f,0.5f,1,1, 1,0.8f,0.2f,1,
            0.8f,0.2f,1,1, 0.2f,1,1,1, 1,0.5f,0.1f,1, 0.7f,1,0.2f,1
        };
        private static final short[] I = {
            0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1,
            3,2,6,3,6,7, 1,5,6,1,6,2, 0,3,7,0,7,4
        };
        private final java.nio.ShortBuffer indices;

        CubeRenderer() {
            vertices = ByteBuffer.allocateDirect(V.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
            vertices.put(V).position(0);
            colors = ByteBuffer.allocateDirect(C.length*4).order(ByteOrder.nativeOrder()).asFloatBuffer();
            colors.put(C).position(0);
            indices = ByteBuffer.allocateDirect(I.length*2).order(ByteOrder.nativeOrder()).asShortBuffer();
            indices.put(I).position(0);
        }
        @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            gl.glClearColor(0.04f,0.06f,0.12f,1);
            gl.glEnable(GL10.GL_DEPTH_TEST);
            gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
            gl.glEnableClientState(GL10.GL_COLOR_ARRAY);
        }
        @Override public void onSurfaceChanged(GL10 gl, int width, int height) {
            gl.glViewport(0,0,width,height);
            gl.glMatrixMode(GL10.GL_PROJECTION); gl.glLoadIdentity();
            float aspect = height == 0 ? 1 : (float)width/height;
            gl.glFrustumf(-aspect,aspect,-1,1,2,12);
            gl.glMatrixMode(GL10.GL_MODELVIEW);
        }
        @Override public void onDrawFrame(GL10 gl) {
            long now = System.nanoTime();
            float dt = Math.min(0.1f,(now-lastNs)/1_000_000_000.0f);
            lastNs = now; angle += 70.0f*dt*speed;
            frames++;
            if (now-fpsWindow >= 1_000_000_000L) { fps=frames; frames=0; fpsWindow=now; }
            gl.glClear(GL10.GL_COLOR_BUFFER_BIT|GL10.GL_DEPTH_BUFFER_BIT);
            gl.glLoadIdentity(); gl.glTranslatef(0,0,-6);
            gl.glRotatef(angle,1,1,0.35f);
            gl.glVertexPointer(3,GL10.GL_FLOAT,0,vertices);
            gl.glColorPointer(4,GL10.GL_FLOAT,0,colors);
            gl.glDrawElements(GL10.GL_TRIANGLES,I.length,GL10.GL_UNSIGNED_SHORT,indices);
        }
    }
}
