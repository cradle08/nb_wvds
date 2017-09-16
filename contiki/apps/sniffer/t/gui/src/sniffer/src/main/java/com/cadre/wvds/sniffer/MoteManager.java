package com.cadre.wvds.sniffer;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.alibaba.fastjson.JSON;

import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.FileUtil;

public class MoteManager {

	private static final Logger log = LoggerFactory.getLogger(MoteManager.class);

	private static MoteManager instance;

	private final String fname = "dat/motes.json";
	private List<Mote> motes;

	private List<MVCView> views = new ArrayList<MVCView>();

	private MoteManager() {

	}

	public static MoteManager getInstance() {
		if (instance == null) {
			instance = new MoteManager();
			instance.load();
		}
		return instance;
	}

	public void load() {
		String text = FileUtil.readAsString(fname);
		motes = JSON.parseArray(text, Mote.class);
		if (motes == null)
			motes = new ArrayList<Mote>();
		for (Mote m : motes) {
			m.eventN = 0;
		}
		log.debug("loaded " + motes.size() + " motes from " + fname);

		//if (motes == null)
		//	motes = new ArrayList<Mote>();
		//motes.clear();
		//String sql = "select meter_no,name from meters";
		//try {
		//	BaseDAO dao = new SQLiteDAO("dat/motes.db");
		//	dao.connect();
		//	ResultSet rs = dao.query(sql);
		//	while (rs != null && rs.next()) {
		//		Mote m = new Mote();
		//		m.setMeterNo(strToBytes(rs.getString(1)));
		//		m.setName(rs.getString(2));
		//		motes.add(m);
		//	}
		//} catch (SQLException e) {
		//	e.printStackTrace();
		//}
	}

	public void save() {
		String text = JSON.toJSONString(motes, false);
		FileUtil.writeToFile(fname, text);
		log.debug("saved " + motes.size() + " motes into " + fname);
	}

	public List<Mote> getMotes() {
		return motes;
	}

	public Mote getMote(int saddr) {
		for (Mote m : motes) {
			if (m.getSaddr() == saddr)
				return m;
		}
		return null;
	}

	public Mote getMoteByMAC(byte[] mac) {
		for (Mote m : motes) {
			if (Arrays.equals(mac, m.getMac()))
				return m;
		}
		return null;
	}

	public Mote getMoteByDev(byte[] dev) {
		for (Mote m : motes) {
			if (Arrays.equals(dev, m.getDev()))
				return m;
		}
		return null;
	}

	public void addMote(Mote m) {
		//if ((m.getDev()[0] != 1) && (m.getDev()[0] != 2) && (m.getDev()[0] != 4))
		//	throw new RuntimeException("invalid devmac " + BytesUtil.strBytes(m.getDev()));
		//if ((m.getMac()[2] != 1) && (m.getMac()[2] != 2) && (m.getMac()[2] != 4))
		//	throw new RuntimeException("invalid mac " + BytesUtil.strBytes(m.getMac()));
		motes.add(m);
		save();
		notifyViews("add", m);
	}

	public void removeMote(Mote m) {
		motes.remove(m);
		save();
		notifyViews("delete", m);
	}

	public void changeMote(Mote m) {
		notifyViews("change", m);
	}

	public boolean hasMote(Mote m) {
		return motes.contains(m);
	}

	public void clear() {
		motes.clear();
	}

	private byte[] strToBytes(String str) {
		String[] nums = str.split(" ");
		byte[] bytes = new byte[nums.length];
		for (int i = 0; i < bytes.length; i++)
			bytes[i] = (byte)Integer.parseInt(nums[i], 16);
		return bytes;
	}

	public void addView(MVCView v) {
		if (!views.contains(v))
			views.add(v);
	}
	
	public void removeView(MVCView v) {
		if (views.contains(v))
			views.remove(v);
	}

	private void notifyViews(String evt, Object obj) {
		if ("add".equals(evt)) {
			for (MVCView v : views)
				v.dataAdded(obj);
		}
		else if ("delete".equals(evt)) {
			for (MVCView v : views)
				v.dataRemoved(obj);
		}
		else if ("change".equals(evt)) {
			for (MVCView v : views)
				v.dataChanged(obj);
		}
	}

}
