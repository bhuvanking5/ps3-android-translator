package org.ps3translator.prototype;

import android.app.Activity;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Graphics translation scaffold: a tiny internal command list is mapped to GLES 2.0.
 * This is NOT an RSX packet decoder and does NOT render PS3 game graphics.
 */
public final class GraphicsTranslationTestActivity extends Activity {
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        FrameLayout root = new FrameLayout(this);
        GLSurfaceView surface = new GLSurfaceView(this);
        surface.setEGLContextClientVersion(2);
        surface.setRenderer(new TranslationRenderer());
        root.addView(surface, new FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        TextView label = new TextView(this);
        label.setTextColor(0xffffffff);
        label.setTextSize(15);
        label.setPadding(18, 18, 18, 18);
        label.setText("GRAPHICS TRANSLATION SCAFFOLD\nInternal CLEAR + DRAW_TRIANGLE commands → OpenGL ES 2.0\nNot a PS3 RSX decoder yet");
        root.addView(label);
        setContentView(root);
    }

    private interface GraphicsCommand {}
    private static final class ClearCommand implements GraphicsCommand {
        final float r, g, b, a;
        ClearCommand(float r, float g, float b, float a) {
            this.r=r; this.g=g; this.b=b; this.a=a;
        }
    }
    private static final class TriangleCommand implements GraphicsCommand {}

    /** The translator turns our documented prototype commands into GLES operations. */
    private static final class CommandTranslator {
        static void execute(GraphicsCommand command, GLESBackend backend) {
            if (command instanceof ClearCommand) {
                ClearCommand c = (ClearCommand) command;
                backend.clear(c.r, c.g, c.b, c.a);
            } else if (command instanceof TriangleCommand) {
                backend.drawTriangle();
            }
        }
    }

    private static final class GLESBackend {
        private static final String VERTEX =
            "attribute vec2 aPosition; attribute vec4 aColor; varying vec4 vColor;" +
            "void main(){ gl_Position=vec4(aPosition,0.0,1.0); vColor=aColor; }";
        private static final String FRAGMENT =
            "precision mediump float; varying vec4 vColor; void main(){ gl_FragColor=vColor; }";
        private final FloatBuffer vertices;
        private int program;
        private int positionLocation, colorLocation;

        GLESBackend() {
            float[] data = {
                0f, 0.65f, 1f, 0.35f, 0.2f, 1f,
                -0.65f, -0.45f, 0.2f, 0.85f, 1f, 1f,
                0.65f, -0.45f, 1f, 0.8f, 0.2f, 1f
            };
            vertices = ByteBuffer.allocateDirect(data.length * 4)
                .order(ByteOrder.nativeOrder()).asFloatBuffer();
            vertices.put(data).position(0);
        }
        void initialize() {
            int vs = compile(GLES20.GL_VERTEX_SHADER, VERTEX);
            int fs = compile(GLES20.GL_FRAGMENT_SHADER, FRAGMENT);
            program = GLES20.glCreateProgram();
            GLES20.glAttachShader(program, vs);
            GLES20.glAttachShader(program, fs);
            GLES20.glLinkProgram(program);
            int[] status = new int[1];
            GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, status, 0);
            if (status[0] == 0) throw new IllegalStateException("GLES shader link failed");
            positionLocation = GLES20.glGetAttribLocation(program, "aPosition");
            colorLocation = GLES20.glGetAttribLocation(program, "aColor");
        }
        private static int compile(int type, String source) {
            int shader = GLES20.glCreateShader(type);
            GLES20.glShaderSource(shader, source);
            GLES20.glCompileShader(shader);
            int[] status = new int[1];
            GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, status, 0);
            if (status[0] == 0) {
                String log = GLES20.glGetShaderInfoLog(shader);
                GLES20.glDeleteShader(shader);
                throw new IllegalStateException("GLES shader compile failed: " + log);
            }
            return shader;
        }
        void clear(float r, float g, float b, float a) {
            GLES20.glClearColor(r, g, b, a);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        }
        void drawTriangle() {
            GLES20.glUseProgram(program);
            vertices.position(0);
            GLES20.glEnableVertexAttribArray(positionLocation);
            GLES20.glVertexAttribPointer(positionLocation, 2, GLES20.GL_FLOAT,
                false, 6 * 4, vertices);
            vertices.position(2);
            GLES20.glEnableVertexAttribArray(colorLocation);
            GLES20.glVertexAttribPointer(colorLocation, 4, GLES20.GL_FLOAT,
                false, 6 * 4, vertices);
            GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 3);
            GLES20.glDisableVertexAttribArray(positionLocation);
            GLES20.glDisableVertexAttribArray(colorLocation);
        }
    }

    private static final class TranslationRenderer implements GLSurfaceView.Renderer {
        private GLESBackend backend;
        @Override public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            backend = new GLESBackend();
            backend.initialize();
        }
        @Override public void onSurfaceChanged(GL10 gl, int width, int height) {
            GLES20.glViewport(0, 0, width, height);
        }
        @Override public void onDrawFrame(GL10 gl) {
            // A tiny deterministic command stream, deliberately not PS3 RSX data.
            CommandTranslator.execute(new ClearCommand(0.035f, 0.055f, 0.11f, 1f), backend);
            CommandTranslator.execute(new TriangleCommand(), backend);
        }
    }
}
