package com.cadre.wvds.gprsmon;

import java.io.File;

import javax.swing.UIManager;

import gnu.io.NoSuchPortException;
import net.skaldchen.commons.comm.ComAdapter;
import net.skaldchen.commons.comm.ComChooser;
import net.skaldchen.commons.util.FileUtil;
import net.skaldchen.commons.util.PID;

public class Main {

	public static ComAdapter com;
	public static GPRSMonitor frame;

	public static String com_name = null;
	public static int com_baud = 9600;

	public static void main(String[] args) {
		initTheme();

		ComChooser cho = new ComChooser();
        cho.setLocationRelativeTo(null);
        cho.setVisible(true);

        com_name = null;
        while (com_name == null) {
            try { Thread.sleep(200); }
            catch (InterruptedException e) { }
        }

		try {
			com = new ComAdapter(com_name, com_baud);
			com.addPacketFilter(new M26Filter());
			com.open();
		} catch (NoSuchPortException e) {
			e.printStackTrace();
		} catch (Exception e) {
			e.printStackTrace();
		}

		frame = new GPRSMonitor();
		frame.setTitle(String.format("GPRS·ÖÎöÆ÷/M26 @ %s:%d", com_name, com_baud));
		com.addPacketListener(frame);
		frame.setLocationRelativeTo(null);
		frame.setVisible(true);

        try {
            String pid = PID.getPID();
            FileUtil.writeToFile("gprsmon.pid", pid);
        } catch(Exception e) {
            e.printStackTrace();
        }
	}

    private static void initTheme() {
        String lnf = null;
        //lnf = "com.sun.java.swing.plaf.windows.WindowsLookAndFeel";
        lnf = "com.jtattoo.plaf.acryl.AcrylLookAndFeel";
        try {
            UIManager.setLookAndFeel(lnf);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
