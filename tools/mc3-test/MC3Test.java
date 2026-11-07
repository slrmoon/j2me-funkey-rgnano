import javax.microedition.lcdui.*;
import javax.microedition.midlet.MIDlet;

import com.mascotcapsule.micro3d.v3.*;

/* MascotCapsule v3 smoke test: an animated lit figure (perspective),
 * primitives and point sprites, and the same figure drawn into a
 * translated/clipped corner view with parallel projection. */
public class MC3Test extends MIDlet implements Runnable {
    private Canvas canvas;
    private volatile boolean running = true;
    private final Graphics3D g3 = new Graphics3D();
    private Figure fig;
    private ActionTable act;
    private Texture tex;
    private Texture spark;
    private final FigureLayout layout = new FigureLayout();
    private final FigureLayout mini = new FigureLayout();
    private final Effect3D effect = new Effect3D();
    private final Effect3D plain = new Effect3D(null, Effect3D.NORMAL_SHADING, true, null);
    private final AffineTrans view = new AffineTrans();
    private final AffineTrans world = new AffineTrans();
    private final AffineTrans tmp = new AffineTrans();
    private int frame;
    private int angle;
    private String error;

    protected void startApp() {
        canvas = new Canvas() {
            protected void paint(Graphics g) {
                draw(g, getWidth(), getHeight());
            }
        };
        try {
            fig = new Figure("/cube.mbac");
            act = new ActionTable("spin.mtra");
            tex = new Texture("/tex.bmp", true);
            spark = new Texture("/spark.bmp", true);
            fig.setTexture(tex);
            effect.setLight(new Light(new Vector3D(-2000, 3000, 3000), 3500, 1200));
            effect.setShadingType(Effect3D.NORMAL_SHADING);
            System.out.println("MC3Test: actions=" + act.getNumActions()
                    + " frames=" + (act.getNumFrames(0) >> 16)
                    + " patterns=" + fig.getNumPattern()
                    + " sin(1024)=" + Util3D.sin(1024) + " sqrt(1000000)=" + Util3D.sqrt(1000000));
        } catch (Throwable e) {
            error = e.toString();
            e.printStackTrace();
        }
        Display.getDisplay(this).setCurrent(canvas);
        new Thread(this).start();
    }

    private void draw(Graphics g, int w, int h) {
        g.setColor(0x203048);
        g.fillRect(0, 0, w, h);
        if (error != null) {
            g.setColor(0xff4040);
            g.drawString(error, 2, 2, Graphics.TOP | Graphics.LEFT);
            return;
        }
        /* camera 2500 units in front, slightly above (y down) */
        view.lookAt(new Vector3D(0, -900, -2500), new Vector3D(0, 700, 2500), new Vector3D(0, -4096, 0));
        world.setIdentity();
        world.rotationY(angle);
        tmp.mul(view, world);
        layout.setAffineTrans(tmp);
        layout.setPerspective(100, 10000, 700);
        layout.setCenter(w / 2, h / 2);
        fig.setPosture(act, 0, frame << 16);

        g3.bind(g);
        g3.renderFigure(fig, 0, 0, layout, effect);

        /* a colored triangle fan floor and point sprites, sharing depth */
        layout.setAffineTrans(view);
        int[] tri = {
            -1500, 400, -1500, 1500, 400, -1500, 1500, 400, 1500,
            -1500, 400, -1500, 1500, 400, 1500, -1500, 400, 1500 };
        int[] colors = { 0x406040, 0x305030 };
        g3.renderPrimitives(null, 0, 0, layout, plain,
                Graphics3D.PRIMITVE_TRIANGLES | Graphics3D.PDATA_COLOR_PER_FACE, 2,
                tri, null, null, colors);
        int[] pts = new int[3 * 6];
        int[] sp = new int[8 * 6];
        for (int i = 0; i < 6; i++) {
            int a = angle * 2 + i * 682;
            pts[i * 3] = Util3D.cos(a) * 1100 / 4096;
            pts[i * 3 + 1] = -300 - (i & 1) * 300;
            pts[i * 3 + 2] = Util3D.sin(a) * 1100 / 4096;
            sp[i * 8] = 160;
            sp[i * 8 + 1] = 160;
            sp[i * 8 + 2] = 0;
            sp[i * 8 + 3] = 0;
            sp[i * 8 + 4] = 0;
            sp[i * 8 + 5] = 32;
            sp[i * 8 + 6] = 32;
            sp[i * 8 + 7] = Graphics3D.POINT_SPRITE_LOCAL_SIZE | Graphics3D.POINT_SPRITE_PERSPECTIVE;
        }
        g3.renderPrimitives(spark, 0, 0, layout, plain,
                Graphics3D.PRIMITVE_POINT_SPRITES | Graphics3D.PDATA_POINT_SPRITE_PARAMS_PER_FACE
                | Graphics3D.PATTR_COLORKEY | Graphics3D.PATTR_BLEND_ADD, 6,
                pts, null, sp, null);
        g3.flush();
        g3.release(g);

        /* corner view: translated, clipped Graphics, parallel projection */
        int cw = w / 3;
        int ch = h / 3;
        g.translate(w - cw - 4, 4);
        g.setClip(0, 0, cw, ch);
        g.setColor(0x000000);
        g.fillRect(0, 0, cw, ch);
        world.setIdentity();
        world.rotationX(512);
        tmp.mul(world, new AffineTrans(4096, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 4096, 0));
        tmp.rotationY(-angle);
        tmp.m23 = 5000;
        mini.setAffineTrans(tmp);
        mini.setScale(120, 120);
        mini.setCenter(cw / 2, ch / 2 + 10);
        g3.bind(g);
        g3.drawFigure(fig, 0, 0, mini, effect);
        g3.release(g);
        g.translate(-(w - cw - 4), -4);
        g.setClip(0, 0, w, h);
        g.setColor(0xffffff);
        g.drawRect(w - cw - 5, 3, cw + 1, ch + 1);
        g.drawString("MC3 frame " + frame, 2, 2, Graphics.TOP | Graphics.LEFT);
    }

    public void run() {
        while (running) {
            frame = (frame + 1) % 60;
            angle += 24;
            canvas.repaint();
            canvas.serviceRepaints();
            try {
                Thread.sleep(40);
            } catch (InterruptedException e) {
            }
        }
    }

    protected void pauseApp() {
    }

    protected void destroyApp(boolean u) {
        running = false;
    }
}
