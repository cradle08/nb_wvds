package com.cadre.wvds.wvdsota;

import java.io.File;
import java.sql.SQLException;

import javax.swing.UIManager;

import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import gnu.io.NoSuchPortException;
import net.skaldchen.commons.comm.ComAdapter;
import net.skaldchen.commons.dao.BaseDAO;
import net.skaldchen.commons.dao.SQLiteDAO;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.I18N;
import net.skaldchen.commons.utils.PID;

public class Main {

	private static Logger log = LoggerFactory.getLogger(Main.class);

	public static String com_name = "COM1";
	public static int com_baud = 115200;

	public static final String inifile = "wvdsota.ini";
	public static final String dbfile  = "dat/sqlite/wvdsota.db";
	public static final String pidfile = "wvdsota.pid";

	public static ComAdapter com;
	public static WVDSNetwork network;
	public static BaseDAO dao;
	public static DlgOTAUpdater gui;

	public static void main(String[] args) {
		Config.init(inifile);
		I18N.init("wvdsota");
		log.info(String.format("WVDSOTA start, version %s commit %s on %s", Version.version, Version.commit, Version.build));

		initTheme();

		ComChooser cho = new ComChooser();
		cho.selectCOM(Config.get("com-name"));
		cho.selectBaud(Config.get("com-baud"));
		cho.setLocationRelativeTo(null);
		cho.setVisible(true);

		com_name = null;
		while (com_name == null) {
			try { Thread.sleep(200); }
			catch (InterruptedException e) { }
		}

		try {
			com = new ComAdapter(com_name, com_baud);
			com.addPacketFilter(new WVDSFilter());
			com.open();

			if (!com_name.equals(Config.get("com-name"))
				|| (com_baud != Config.getInt("com-baud"))) {
				Config.set("com-name", com_name);
				Config.set("com-baud", String.valueOf(com_baud));
				Config.save();
			}
		} catch (NoSuchPortException e) {
			log.warn(String.format("no port %s", com_name));
		} catch (Exception e) {
			log.warn("COM error", e);
		}

		dao = new SQLiteDAO(dbfile);
		try {
			dao.connect();
		} catch (SQLException e1) {
			log.warn("cannot open sqlite database " + dbfile);
		}

		network = new WVDSNetwork(com);

		try {
			String pid = PID.getPID();
			FileUtils.writeStringToFile(new File(pidfile), pid);
		} catch(Exception e) {
			e.printStackTrace();
		}

		gui = new DlgOTAUpdater();
		com.addPacketListener(gui);
		gui.setTitle(String.format("WVDS OTAÉý¼¶ @ %s:%d", com_name, com_baud));
		//gui.setLocationRelativeTo(null);
		gui.setLocation(Config.getInt("frame-x"), Config.getInt("frame-y"));
		gui.setVisible(true);
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
