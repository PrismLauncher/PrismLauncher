package org.multimc;/*
 * Copyright 2012-2017 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import net.minecraft.Launcher;

import javax.imageio.ImageIO;
import java.applet.Applet;
import java.awt.*;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;

public class LegacyFrame extends Frame implements WindowListener
{
    private Launcher appletWrap = null;
    public LegacyFrame(String title)
    {
        super ( title );
        BufferedImage image;
        try {
            image = ImageIO.read ( new File ( "icon.png" ) );
            setIconImage ( image );
        } catch ( IOException e ) {
            e.printStackTrace();
        }
        this.addWindowListener ( this );
    }

    public void start ( Applet mcApplet, String user, String session, Dimension winSize, boolean maximize )
    {
        try {
            appletWrap = new Launcher( mcApplet, new URL ( "http://www.minecraft.net/game" ) );
        } catch ( MalformedURLException ignored ) {}
        appletWrap.setParameter ( "username", user );
        appletWrap.setParameter ( "sessionid", session );
        appletWrap.setParameter ( "stand-alone", "true" ); // Show the quit button.
        appletWrap.setParameter ( "haspaid", "true" ); // Some old versions need this for world saves to work.
		appletWrap.setParameter ( "demo", "false" );
		appletWrap.setParameter("fullscreen", "false");
        mcApplet.setStub(appletWrap);
        this.add ( appletWrap );
        appletWrap.setPreferredSize ( winSize );
        this.pack();
        this.setLocationRelativeTo ( null );
        this.setResizable ( true );
        if ( maximize ) {
            this.setExtendedState ( MAXIMIZED_BOTH );
        }
        validate();
        appletWrap.init();
        appletWrap.start();
        setVisible ( true );
    }

    @Override
    public void windowActivated ( WindowEvent e ) {}

    @Override
    public void windowClosed ( WindowEvent e ) {}

    @Override
    public void windowClosing ( WindowEvent e )
    {
        new Thread() {
            public void run() {
                try {
                    Thread.sleep ( 30000L );
                } catch ( InterruptedException localInterruptedException ) {
                    localInterruptedException.printStackTrace();
                }
                System.out.println ( "FORCING EXIT!" );
                System.exit ( 0 );
            }
        }
        .start();

        if ( appletWrap != null ) {
            appletWrap.stop();
            appletWrap.destroy();
        }
        // old minecraft versions can hang without this >_<
        System.exit ( 0 );
    }

    @Override
    public void windowDeactivated ( WindowEvent e ) {}

    @Override
    public void windowDeiconified ( WindowEvent e ) {}

    @Override
    public void windowIconified ( WindowEvent e ) {}

    @Override
    public void windowOpened ( WindowEvent e ) {}
}
