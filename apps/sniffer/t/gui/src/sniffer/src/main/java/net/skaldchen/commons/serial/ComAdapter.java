package net.skaldchen.commons.serial;

import gnu.io.*;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.cadre.wvds.sniffer.Main;

import net.skaldchen.commons.utils.Config;

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

    private final int RXBUF_SIZE = 1024;
    private RingByteBuffer rxbuf = new RingByteBuffer(RXBUF_SIZE);

    private String comName = "COM1";
    private int baudrate = 9600;

    private CommPortIdentifier comIdent;
    private SerialPort com;
    private InputStream in;
    private OutputStream out;
    private boolean running = false;

    private List<PacketListener> listeners;

    private List<byte[]> outQueue;

    public ComAdapter(String comName, int baudrate) throws NoSuchPortException {
        this.comName = comName;
        this.baudrate = baudrate;
        comIdent = CommPortIdentifier.getPortIdentifier(comName);
        listeners = new ArrayList<PacketListener>();
        outQueue = new ArrayList<byte[]>();
    }

    public void open() throws Exception {
        log.info("open " + comName + ":" + baudrate);
        com = (SerialPort) comIdent.open("ComAgent", CONN_TIMEOUT);

        com.setSerialPortParams(baudrate, SerialPort.DATABITS_8,
                SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
        com.addEventListener(this);
        com.notifyOnDataAvailable(true);

        in = com.getInputStream();
        out = com.getOutputStream();

        log.info("" + comName + " opened");
        running = true;
    }

    public void close() throws Exception {
        log.info("close " + comName);
        running = false;
        in.close();
        out.close();
        com.close();
        log.info("" + comName + " closed");
    }

    public void addPacketListener(PacketListener l) {
        listeners.add(l);
    }

    private void notifyListeners(byte[] pkt) {
        for (PacketListener l : listeners)
            l.packetReceived(pkt);
    }

    public void send(byte[] pkt) {
        directSend(pkt);
    }

    public void directSend(byte[] pkt) {
        try {
            log.info(String.format("send: (%2d) %s", pkt.length, strBytes(pkt)));
            out.write(pkt);
        } catch (IOException e) {
            log.warn("fail send", e);
        }
    }

    @Override
    public void serialEvent(SerialPortEvent e) {
        switch (e.getEventType()) {
            case SerialPortEvent.DATA_AVAILABLE:
                try {
                    log.debug(String.format("data avail, %d bytes", in.available()));
                    long deadline = Long.MAX_VALUE;
                    do {
                        if (in.available() > 0) {
                            rxbuf.putFrom(in);
                            deadline = System.currentTimeMillis() + Config.getInt("com-timeout");
                        }
                    } while (System.currentTimeMillis() < deadline);

                    byte[] pkt = rxbuf.pop(rxbuf.length());
                    if (pkt != null && pkt.length > 0) {
                        log.info(String.format("rcvd: (%2d) %s", pkt.length, strBytes(pkt)));
                        notifyListeners(pkt);
                    }
                }
                catch (IOException e1) {
                    log.warn("COM disconnected", e1);
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

    private String strBytes(byte[] bytes) {
        return strBytes(bytes, 0, bytes.length);
    }

    private String strBytes(byte[] bytes, int ofs, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++)
            sb.append(String.format("%02X ", bytes[ofs+i]));
        return (sb.length() > 1) ? sb.substring(0, sb.length()-1) : "";
    }

    private String txtBytes(byte[] bytes, int ofs, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            if (bytes[ofs+i] == 0x0D) {
                sb.append("\\r");
            } else if (bytes[ofs+i] == 0x0A) {
                sb.append("\\n");
            } else {
                sb.append((char)bytes[ofs+i]);
            }
        }
        return sb.toString();
    }

}
