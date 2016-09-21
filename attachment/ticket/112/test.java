import java.lang.*;
import java.awt.*;
import java.applet.Applet;
import javax.media.j3d.*;
import javax.vecmath.*;
import com.sun.j3d.utils.universe.*;
import com.sun.j3d.utils.geometry.*;
import com.sun.j3d.utils.applet.MainFrame;

class test extends Applet
{
 static VirtualUniverse vu;

 public test ()
 {
    GraphicsConfiguration gc;
    Canvas3D              c3d;
    System.out.println("Hello Java3D");
    gc = this.getGC();
    System.out.println("gc    = " + gc);
    vu = new VirtualUniverse();
    try
    {
       c3d = new Canvas3D(gc);
	System.out.println("c3d   = " + c3d);
       setLayout(new BorderLayout());
	add("Center", c3d);
	Locale loc = new Locale(vu);
       loc.addBranchGraph(view(c3d));
       loc.addBranchGraph(scene());
    }
    catch (NullPointerException e)
    {
       System.out.println("Exception instantiating Canvas3D object");
	System.out.println("e = " + e.getMessage());
    }
 }

 public GraphicsConfiguration getGC ()
 {
    GraphicsConfigTemplate3D gct3D = new GraphicsConfigTemplate3D();
    GraphicsEnvironment      ge    = GraphicsEnvironment
                                      .getLocalGraphicsEnvironment();
    GraphicsDevice           gd[]  = ge.getScreenDevices();

    System.out.println("gct3D = " + gct3D);
    System.out.println("ge    = " + ge);
    System.out.println("gd[0] = " + gd[0]);

    return gd[0].getBestConfiguration( gct3D );
 }

 public BranchGroup scene()
 {
    BranchGroup    bg  = new BranchGroup();
    TransformGroup tg = new TransformGroup();
    Transform3D    tf = new Transform3D();
    Matrix3d       m1 = new Matrix3d();
    Matrix3d       m2 = new Matrix3d();
    m2.rotY(Math.toRadians(45.0));
    m1.rotX(Math.toRadians(30.0));
    m1.mul(m2);
    tf.setRotation(m1);
    tg.setTransform(tf);
    tg.addChild(new ColorCube());
    bg.addChild(tg);
    bg.compile();
    return bg;
 }

 public BranchGroup view( Canvas3D c )
 {
    BranchGroup         bg  = new BranchGroup();
    TransformGroup      tg = new TransformGroup();
    Transform3D         tf = new Transform3D();
    ViewPlatform        vp  = new ViewPlatform();
    View                v   = new View();
    PhysicalBody        pb  = new PhysicalBody();
    PhysicalEnvironment pe  = new PhysicalEnvironment();
    v.setPhysicalBody(pb);
    v.setPhysicalEnvironment(pe);
    v.addCanvas3D(c);
    v.attachViewPlatform(vp);
    v.setFrontClipDistance(1);
    v.setBackClipDistance(50);
    vp.setViewAttachPolicy(View.RELATIVE_TO_FIELD_OF_VIEW);
    tf.setTranslation(new Vector3d(0.0, 0.0, 5.0));
    tg.setTransform(tf);
    tg.addChild(vp);
    bg.addChild(tg);
    return bg;
 }

 public static void main ( String[] args )
 {
    new MainFrame( new test(), 512, 512 );
    System.out.println("System Properties:  " + System.getProperties());
    System.out.println("Java3D Properties:  " + vu.getProperties());
    try
    {
       Thread.sleep(5000);
    }
    catch (Exception e)
    {
       System.out.println("main: exception: " + e.getMessage());
    }
    System.exit(0);
 }
}
