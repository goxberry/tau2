/*
 * VisCanvas.java
 *
 * Copyright 2005-2006                                
 * Performance Research Laboratory, University of Oregon
 */
package edu.uoregon.tau.vis;

import java.awt.Canvas;
import java.awt.event.KeyListener;

import net.java.games.jogl.GLCanvas;
import net.java.games.jogl.GLCapabilities;
import net.java.games.jogl.GLDrawableFactory;

/**
 * This class is merely a wrapper over GLCanvas which allows users of the Vis
 * package to build against vis alone (not jogl). 
 *
 * <P>CVS $Id: VisCanvas.java,v 1.5 2006/09/01 20:18:08 amorris Exp $</P>
 * @author  Alan Morris
 * @version $Revision: 1.5 $
 */
public class VisCanvas {

    private GLCanvas glCanvas;

    
    /**
     * Creates a <tt>VisCanvas</tt> with the given <tt>VisRenderer</tt>.
     * 
     * @param visRenderer <tt>VisRenderer</tt> to use.
     */
    public VisCanvas(VisRenderer visRenderer) {
     
        GLCapabilities glCapabilities = new GLCapabilities();

        //glCapabilities.setHardwareAccelerated(true);

        glCanvas = GLDrawableFactory.getFactory().createGLCanvas(glCapabilities);

        glCanvas.setSize(200, 200);
        glCanvas.addGLEventListener(visRenderer);
        
        // for testing
        //canvas.addGLEventListener(new Gears.GearRenderer());

    }
    
    /**
     * Adds a key listener to the underlying glCanvas.
     * @param keyListener the key listener to add.
     */
    public void addKeyListener(KeyListener keyListener) {
        glCanvas.addKeyListener(keyListener);
    }
    
    
    /**
     * Returns the actual GLCanvas as a Canvas.
     * @return the actual GLCanvas as a Canvas.
     */
    public Canvas getActualCanvas() {
        return glCanvas;
    }

    /**
     * Returns the height of the <tt>Canvas</tt>.
     * @return the height of the <tt>Canvas</tt>.
     */
    public int getHeight() {
        return glCanvas.getHeight();
    }
    
    /**
     * Returns the width of the <tt>Canvas</tt>.
     * @return the width of the <tt>Canvas</tt>.
     */
    public int getWidth() {
        return glCanvas.getWidth();
    }
    
    
    
    
}
