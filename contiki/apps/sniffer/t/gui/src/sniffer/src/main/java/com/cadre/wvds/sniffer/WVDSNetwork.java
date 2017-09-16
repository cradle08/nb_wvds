package com.cadre.wvds.sniffer;

import java.io.IOException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.serial.CommAdapter;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.CRC16;

public class WVDSNetwork extends Network {

	private static final Logger log = LoggerFactory.getLogger(WVDSNetwork.class);

	public WVDSNetwork(CommAdapter comm) {
		super(comm);
	}

	private void sendCommand(int addr, byte[] dev, int op, byte[] arg) {
		sendCommand(addr, dev, op, arg, false);
	}

	private void sendCommand(int addr, byte[] dev, int op, byte[] arg, boolean rep) {
		int arglen = ((arg != null) ? arg.length : 0);
		int len = 3 + 9 + 6 + arglen + 3;
		byte[] frame = new byte[len];
		int i = 0;
		
		frame[i++] = (byte) 0x11;          // WVDS command
		frame[i++] = (byte) (addr >> 8);
		frame[i++] = (byte) (addr & 0xFF);

		frame[i++] = (byte) WVDS.PKT_BEG;
		frame[i++] = (byte) (7 + 6 + arglen);
		System.arraycopy(dev, 0, frame, i, 6); i += 6;
		frame[i++] = (byte) (op + 0x80 + (rep ? 0x40 : 0x00));

		System.arraycopy(WVDS.getTS(), 0, frame, i, 6); i += 6;
		if (arg != null) {
			System.arraycopy(arg, 0, frame, i, arg.length); i += arg.length;
		}

		int ccrc = CRC16.calc(frame, 0x0000);
		frame[i++] = (byte) (ccrc >> 8);
		frame[i++] = (byte) (ccrc & 0xff);
		frame[i++] = (byte) WVDS.PKT_END;

		log.info(String.format("send: (%d) %s", frame.length, BytesUtil.strBytes(frame)));
		try {
			comm.send(frame);
		} catch (IOException e) {
			log.warn("fail send", e);
		}
	}

	public void reset(int addr, byte[] dev) {
		log.info(String.format("reset node %d/%s", addr, Mote.strDev(dev)));
		sendCommand(addr, dev, WVDS.PKT_RESET, null);
	}

	public void freset(int addr, byte[] dev) {
		log.info(String.format("freset node %d/%s", addr, Mote.strDev(dev)));
		sendCommand(addr, dev, WVDS.PKT_FRESET, null);
	}

	public void reinit(int addr, byte[] dev) {
		log.info(String.format("reinit node %d/%s", addr, Mote.strDev(dev)));
		sendCommand(addr, dev, WVDS.PKT_REINIT, null);
	}

	public void algoGet(int addr, byte[] dev) {
		log.info(String.format("get algo of node %d/%s", addr, Mote.strDev(dev)));
		byte[] arg = new byte[1];
		arg[0] = WVDS.GET;
		sendCommand(addr, dev, WVDS.PKT_ALGO, arg, true);
	}

	public void algoSet(int addr, byte[] dev, AlgoParam algo) {
		log.info(String.format("set algo of node %d/%s to be %s", addr, Mote.strDev(dev), algo.toString()));
		byte[] arr = algo.toBytes();
		byte[] arg = new byte[1 + arr.length];
		arg[0] = WVDS.SET;
		System.arraycopy(arr, 0, arg, 1, arr.length);
		sendCommand(addr, dev, WVDS.PKT_ALGO, arg, true);
	}

	public void actack(int addr, byte[] dev) {
		log.info(String.format("actack node %d/%s", addr, Mote.strDev(dev)));
		byte[] arg = new byte[1];
		arg[0] = 0;
		sendCommand(addr, dev, WVDS.PKT_VDACT, arg);
	}

	public void connack(int addr, byte[] dev) {
		log.info(String.format("connack node %d/%s", addr, Mote.strDev(dev)));
		byte[] arg = new byte[1];
		arg[0] = 0;
		sendCommand(addr, dev, WVDS.PKT_FAECONN, arg);
	}

	public void disconn(int addr, byte[] dev, int chan) {
		log.info(String.format("disconn node %d/%s", addr, Mote.strDev(dev)));
		byte[] arg = new byte[2];
		arg[0] = 1;
		arg[1] = (byte) chan;
		sendCommand(addr, dev, WVDS.PKT_DISCONN, arg, true);
	}

}
