package net.skaldchen.commons.serial;

import gnu.io.CommPortIdentifier;
import gnu.io.NoSuchPortException;
import gnu.io.SerialPort;
import gnu.io.SerialPortEvent;
import gnu.io.SerialPortEventListener;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.utils.BytesUtil;
import com.cadre.wvds.sniffer.Main;

/**
 * Created with IntelliJ IDEA.
 * User: Administrator
 * Date: 13-5-27
 * Time: ????4:36
 * To change this template use File | Settings | File Templates.
 */
public class HDLCAdapter extends CommAdapter implements SerialPortEventListener, Runnable {

	private final Logger log = LoggerFactory.getLogger(HDLCAdapter.class);

	public static boolean DEBUG = false;

	private final int CONN_TIMEOUT = 2000;
	private final int READ_TIMEOUT = 2;    // TODO: should be calculated according to baudrate

	private final int RXBUF_SIZE = 1024;

	private String comName = "COM1";
	private int baudrate = 9600;

	private CommPortIdentifier comIdent;
	private SerialPort com;
	private InputStream in;
	private OutputStream out;

	private List<PacketListener> listeners;

	private List<byte[]> inQueue;
	private List<byte[]> outQueue;
	private List<Long> inQueueTs;
	private byte txseq;

	public HDLCAdapter(String comName, int baudrate) throws NoSuchPortException {
		this.comName = comName;
		this.baudrate = baudrate;
		comIdent = CommPortIdentifier.getPortIdentifier(comName);
		listeners = new ArrayList<PacketListener>();
		outQueue = new ArrayList<byte[]>();
		inQueue = new ArrayList<byte[]>();
		inQueueTs = new ArrayList<Long>();
		//open();
	}

	public void open() throws Exception {
		log.debug("open " + comName + ":" + baudrate);
		com = (SerialPort) comIdent.open("ComAgent", CONN_TIMEOUT);

		com.setSerialPortParams(baudrate, SerialPort.DATABITS_8,
				SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
		com.addEventListener(this);
		com.notifyOnDataAvailable(true);
		log.debug("connected " + comName);

		in = com.getInputStream();
		out = com.getOutputStream();

		log.debug("" + comName + " opened");
		running = true;
		new Thread(this).start();
	}

	public void close() throws Exception {
		log.debug("close " + comName);
		running = false;
		in.close();
		out.close();
		com.close();
		log.debug("" + comName + " closed");
	}

	public void addPacketListener(PacketListener l) {
		if (!listeners.contains(l)) {
			log.debug("add packet listener " + l);
			listeners.add(l);
		}
	}

	public void removePacketListener(PacketListener l) {
		if (listeners.contains(l)) {
			log.debug("remove packet listener " + l);
			listeners.remove(l);
		}
	}

	private void notifyListeners(byte[] pkt, long rcvtime) {
		for (PacketListener l : listeners)
			l.packetReceived(pkt);
	}

	public void send(byte[] pkt) {
		send(pkt, HDLC.PROTO_PACKET_NOACK);
	}

	public void send(byte[] pkt, int proto) {
		byte[] outpkt;
		outpkt = HDLC.encode(pkt, proto, txseq, 0);
		try {
			log.debug(String.format("send: (%2d) %s", outpkt.length, BytesUtil.strBytes(outpkt)));
			out.write(outpkt);
		} catch (IOException e) {
			log.warn("fail send", e);
		}
		//log.debug(String.format("enq data len %d", pkt.length));
		//outQueue.add(encode(pkt, proto, txseq, 0));
		//data_out = true;
	}

	@Override
	public void serialEvent(SerialPortEvent e) {
		switch (e.getEventType()) {
		case SerialPortEvent.DATA_AVAILABLE:
			try {
				log.debug(String.format("data avail, %d bytes", in.available()));
				data_in = true;
			} catch (IOException e1) {
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

	private boolean running = false;
	private boolean data_in = false;
	//private boolean data_out = false;

	private final int RXBUF_LEN = 160;
	private boolean insyn = false;
	private boolean inesc = false;
	private byte[] rxbuf = new byte[RXBUF_LEN];
	private int rxidx;
	private int rxlen;

	@Override
	public void run() {
		byte[] rbuf = new byte[RXBUF_LEN];
		int rlen = 0;
		boolean idle = true;

		while (running) {
			try {
				if (data_in) {
					while (in.available() > 0) {
						rlen = in.read(rbuf);
						for (int i = 0; i < rlen; i++) {
							byte b = rbuf[i];
							if (!insyn) {
								if (b == HDLC.FLAG_BYTE) {
									insyn = true;
									rxidx = 0;
									rxbuf[rxidx++] = b;
								}
							}
							else {
								if (inesc) {
									if (b == HDLC.FLAG_BYTE) {
										insyn = false;
										rxidx = 0;
										continue; // invalid sync
									}
									rxbuf[rxidx++] = (byte) (b ^ 0x20);
									inesc = false;
								}
								else if (b == HDLC.ESC_BYTE) {
									inesc = true;
								}
								else if (b == HDLC.FLAG_BYTE) {
									if (rxidx == 1) {
										rxidx = 0;
										rxbuf[rxidx++] = b;
										continue; // start next packet
									}
									else if (rxidx < 4) {
										insyn = false;
										rxidx = 0;
										continue; // packet too small
									}

									rxbuf[rxidx++] = b;
									rxlen = rxidx;
									rxidx = 0;
									insyn = false;
									log.debug(String.format("rcvd: (%d) %s", rxlen, BytesUtil.strBytes(rxbuf, 0, rxlen)));

									byte[] pkt = BytesUtil.getBytes(rxbuf, 4, rxlen - 7);
									inQueue.add(pkt);
									inQueueTs.add(Long.valueOf(System.currentTimeMillis()));
								}
								else {
									rxbuf[rxidx++] = b;
								}
							}
						}
					}
				}

				if (outQueue.size() > 0) {
					byte[] pkt = outQueue.get(0);
					try {
						log.debug(String.format("send: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
						out.write(pkt);
						outQueue.remove(0);
					} catch (IOException e) {
						log.warn("failed send to " + comName, e);
					}

					idle = (outQueue.size() == 0);
				}

				if (inQueue.size() > 0) {
					byte[] pkt = inQueue.get(0);
					Long t = inQueueTs.get(0).longValue();

					//log.debug(String.format("proc: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
					notifyListeners(pkt, t);

					inQueue.remove(0);
					inQueueTs.remove(0);

					idle = (inQueue.size() == 0);
				}

				if (idle) {
					Thread.sleep(2);
				}
			} catch (Exception e) {
				log.warn("", e);
			}
		}
	}

}
