package com.cadre.wvds.sniffer;

import java.io.File;

import javax.swing.UIManager;

import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.packet.PacketParser;
import net.skaldchen.commons.serial.ComAdapter;
import net.skaldchen.commons.serial.CommAdapter;
import net.skaldchen.commons.serial.HDLCAdapter;
import net.skaldchen.commons.utils.ComUtil;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.I18N;
import net.skaldchen.commons.utils.PID;

public class Main {

    private static Logger log = LoggerFactory.getLogger(Main.class);

    public static boolean com_done = false;
    public static CommAdapter comm;
    public static Network nwk;
    public static SnifferApp gui;
    public static PacketParser parser;
    public static TraceFileManager tracemgr;

    public static final String inifile = "sniffer.ini";
    public static final String pidfile = "sniffer.pid";

    /**
     * Auto-generated main method to display this JFrame
     */
    public static void main(final String[] args) {
        Config.init(inifile);
        I18N.init("sniffer");
        initTheme();

        if (Config.getBool("com-select") || !ComUtil.avail(Config.get("com-name"))
            || ("COM1".equals(Config.get("com-name")) && !Config.getBool("native-com")))
        {
            ComChooser dlg = new ComChooser();
            dlg.setLocationRelativeTo(null);
            dlg.setVisible(true);

            com_done = false;
            while (!com_done) {
                try { Thread.sleep(200); }
                catch (InterruptedException e) { }
            }
        }

        parser = new PacketParser("packet.xml");
        tracemgr = new TraceFileManager("dat/trace.json");

        String comName = Config.get("com-name");
        int comBaud = Config.getInt("com-baud");
        try {
            if ("raw".equals(Config.get("serial-proto"))) {
                comm = new ComAdapter(comName, comBaud);
            }
            else if ("hdlc".equals(Config.get("serial-proto"))) {
                comm = new HDLCAdapter(comName, comBaud);
            }
            else {
                log.warn(String.format("unsupport proto %s, support: raw,hdlc", Config.get("serial-proto")));
                System.exit(1);
            }

            comm.open();
        } catch(Exception e) {
            e.printStackTrace();
            System.exit(2);
        }

        nwk = new WVDSNetwork(comm);
        comm.addPacketListener(nwk);

        gui = new SnifferApp();
        comm.addPacketListener(gui);
        gui.setComm(comm);
        gui.setTitle(String.format("%s @ %s:%d", I18N.get("title.app"), comName, comBaud));

        //gui.setLocationRelativeTo(null); // center of screen
        gui.setLocation(Config.getInt("frame-x"), Config.getInt("frame-y"));
        gui.setVisible(true);
        gui.jPanel1.setDividerLocation(Config.getDouble("packet-vert-div"));
        gui.jPanel4.setDividerLocation(Config.getDouble("packet-hori-div"));
        gui.tabAnalyse.splitPane1.setDividerLocation(0.18);
        gui.tabAnalyse.splitPane2.setDividerLocation(0.3);

        nwk.getChannel();
        nwk.getTXPower();

        try {
            String pid = PID.getPID();
            FileUtils.writeStringToFile(new File(pidfile), pid);
            log.info(String.format("PID: %s", pid));
        } catch(Exception e) {
            log.warn("fail write pid file", e);
        }
    }

    private static void initTheme() {
        String lnf = null;
        lnf = "com.sun.java.swing.plaf.windows.WindowsLookAndFeel";
        //lnf = "com.jtattoo.plaf.acryl.AcrylLookAndFeel";
        try {
            UIManager.setLookAndFeel(lnf);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
