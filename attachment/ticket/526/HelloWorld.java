import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import javax.imageio.ImageIO;

public class HelloWorld extends Frame 
{
  public static void main (String [] argv)
  {
	HelloWorld h = new HelloWorld();
  }

  public HelloWorld()
  {
	setTitle("Hello World!");		
	setSize(300, 250);

	Panel P = new Panel();
	add("Center", P);

	Button button = new Button("CAPTURE ME");
	add("South", button);
	button.addActionListener(new ActionListener() {
	    public void actionPerformed(ActionEvent e) 
		{
		  try
			{
			  Robot rb = new Robot();
			  Rectangle R = getBounds();
			  Point P = getLocationOnScreen();
			  R.setLocation(P);
			  BufferedImage img = rb.createScreenCapture(R); 
			  ImageIO.write(img,"jpg",new File("dump.jpg"));
			  System.out.println("Wrote dump.jpg");
			} 
		  catch(Exception ee)
			{
			  System.out.println("Exception "+ee);
			} 
		  System.exit(0);
	    }
	  });

	pack();
	setVisible(true);

	int width = 200;
	int height = 200;
	Graphics g = P.getGraphics();
	g.drawString("XXX",0, height/2);

	addWindowListener(new WindowAdapter()
	  {
	    public void windowClosing(WindowEvent e)
		{		  
		  System.exit(0);
		};
	  });
  }
}