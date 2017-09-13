package net.skaldchen.commons.comm;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
//import java.util.ArrayList;
//import java.util.List;
import java.util.Properties;

import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.cadre.wvds.wvdsota.Main;

import gnu.io.CommPortIdentifier;
import gnu.io.NoSuchPortException;
import gnu.io.SerialPort;
import gnu.io.SerialPortEvent;
import gnu.io.SerialPortEventListener;
import net.skaldchen.commons.utils.BytesUtil;

/**
 * Created with IntelliJ IDEA.
 * User: Administrator
 * Date: 13-5-27
 * Time: 涓嬪崍4:36
 * To change this template use File | Settings | File Templates.
 */
public class ComAdapter extends CommAdapter implements SerialPortEventListener {

    private final Logger log = LoggerFactory.getLogger(ComAdapter.class);

    public static boolean DEBUG = false;

    private final int CONN_TIMEOUT = 2000;

    private String comName = "COM1";
    private int comBaud = 9600;
    private String comMode = "8N1";

    private CommPortIdentifier comIdent;
    private SerialPort com;
    private InputStream in;
    private OutputStream out;

    //private int rx_timeout = 10; // in milliseconds
    //private List<byte[]> outQueue;

    private int[][] dataBitsMap = new int[][]{
        {'8',SerialPort.DATABITS_8},
        {'7',SerialPort.DATABITS_7},
        {'6',SerialPort.DATABITS_6},
        {'5',SerialPort.DATABITS_5}
    };
    private int[][] stopBitsMap = new int[][]{
        {'1',SerialPort.STOPBITS_1},
        {'2',SerialPort.STOPBITS_2},
        {'5',SerialPort.STOPBITS_1_5}
    };
    private int[][] parityMap = new int[][] {
        {'N',SerialPort.PARITY_NONE},
        {'E',SerialPort.PARITY_EVEN},
        {'O',SerialPort.PARITY_ODD},
        {'M',SerialPort.PARITY_MARK},
        {'S',SerialPort.PARITY_SPACE}
    };

    public ComAdapter(String comName, int comBaud) throws NoSuchPortException {
        this(comName, comBaud, "8N1");
    }

    public ComAdapter(String comName, int comBaud, String comMode) throws NoSuchPortException {
        this.comName = comName;
        this.comBaud = comBaud;
        this.comMode = comMode;

        // Handle portable issue, such as for /dev/ttyAMA0 on RPi
        Properties props = System.getProperties();
        String currentPorts = props.getProperty("gnu.io.rxtx.SerialPort", comName);
        if (currentPorts.equals(comName)) {
            props.setProperty("gnu.io.rxtx.SerialPort", comName);
        } else {
            props.setProperty("gnu.io.rxtx.SerialPort", currentPorts + File.pathSeparator + comName);
        }

        comIdent = CommPortIdentifier.getPortIdentifier(comName);
        //outQueue = new ArrayList<byte[]>();
    }

    public void open() throws Exception {
        super.open();
        log.info("open " + comName + ":" + comBaud + "," + comMode);
        com = (SerialPort) comIdent.open("ComAgent", CONN_TIMEOUT);

        int data = mode(comMode.charAt(0), dataBitsMap);
        int parity = mode(comMode.charAt(1), parityMap);
        int stop = mode(comMode.charAt(2), stopBitsMap);
        log.debug(String.format("data: %d, parity: %d, stop: %d", data, parity, stop));
        com.setSerialPortParams(comBaud, data, stop, parity);
        com.addEventListener(this);
        com.notifyOnDataAvailable(true);

        in = com.getInputStream();
        out = com.getOutputStream();
        log.info("" + comName + " opened");
    }

    private int mode(int key, int[][] map) {
        for (int i = 0; i < map.length; i++) {
            if (key == map[i][0])
                return map[i][1];
        }
        return -1;
    }

    public void close() throws Exception {
        super.close();
        log.info("close " + comName);
        in.close();
        out.close();
        com.close();
        log.info("" + comName + " closed");
    }

    public void send(byte[] pkt) {
        directSend(pkt);
    }

    public void directSend(byte[] pkt) {
        try {
        	log.debug(String.format("send: (%2d) %s", pkt.length, BytesUtil.strBytes(pkt)));
            out.write(pkt);
        } catch (IOException e) {
            log.warn("fail send", e);
        }
    }

    public void setTimeout(int timeout) {
        //this.rx_timeout = timeout;
    }

    @Override
    public void serialEvent(SerialPortEvent e) {
        switch (e.getEventType()) {
            case SerialPortEvent.DATA_AVAILABLE:
                try {
                    log.debug(String.format("data avail, %d bytes", in.available()));
                    int len = 0;
                    if ((len = in.available()) > 0) {
                        byte[] buf = new byte[len];
                        len = in.read(buf);
                        if (len > 0) {
                            log.debug(String.format("rcvd: (%2d) %s", buf.length, BytesUtil.strBytes(buf)));
                            inQueue.add(buf);
                        }
                    }
                }
                catch (IOException e1) {
                    log.warn("COM error", e1);
                    SwingUtilities.invokeLater(new Runnable() {
                        @Override
                        public void run() {
                            String message = "串口已断开，请重新连接和启动程序";
                            JOptionPane.showMessageDialog(Main.gui, message);
                            System.exit(1);
                        }
                    });
                    com.notifyOnDataAvailable(false);
                    com.removeEventListener();
                }
                break;
        }
    }

}
