package com.cadre.wvds.wvdsota;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class MoteManager {

	private static MoteManager instance;

	private List<Mote> motes;

	private MoteManager() {
		this.motes = new ArrayList<Mote>();
	}

	public static MoteManager getInstance() {
		if (instance == null) {
			instance = new MoteManager();
			instance.load();
		}
		return instance;
	}

	private void load() {
		String sql = "select mac,type,ver,net_id from motes";
		List<Map<String, Object>> res = Main.dao.queryAsList(sql);
		Iterator<Map<String, Object>> it = res.iterator();
		motes.clear();
		while (it.hasNext()) {
			Map<String, Object> e = it.next();
			Mote m = new Mote();
			m.setMac(Mote.toMAC((String)e.get("mac")));
			m.setType((Integer)e.get("type"));
			m.setVer((Integer)e.get("ver"));
			m.setNetId((Integer)e.get("net_id"));
			motes.add(m);
		}
	}

	public List<Mote> getMotes() {
		return motes;
	}

	public List<Mote> getMotes(int net, int type) {
		List<Mote> list = new ArrayList<Mote>();
		for (Mote m : motes) {
			if (m.getNetId() == net && m.getType() == type)
				list.add(m);
		}
		return list;
	}

	public Mote getMote(byte[] mac) {
		for (Mote m : motes) {
			if (Arrays.equals(mac, m.getMac()))
				return m;
		}
		return null;
	}

	public void addMote(Mote m) {
		if (!motes.contains(m)) {
			motes.add(m);

			String sql = "insert into motes(mac,type,ver,net_id) values('%s',%d,%d,%d)";
			sql = String.format(sql, Mote.strMAC(m.getMac()), m.getType(), m.getVer(), m.getNetId());
			Main.dao.execute(sql);
		}
	}

	public void removeMote(Mote m) {
		if (motes.contains(m)) {
			motes.remove(m);

			String mac = Mote.strMAC(m.getMac());
			String sql = String.format("delete from motes where mac='%s'", mac);
			Main.dao.execute(sql);
		}
	}

	public void removeMote(String mac) {
		Iterator<Mote> it = motes.iterator();
		while (it.hasNext()) {
			Mote m = it.next();
			if (m.getMac().equals(mac))
				it.remove();
		}

		String sql = "delete from motes where mac='%s'";
		sql = String.format(sql, mac);
		Main.dao.execute(sql);
	}

	public void removeMote(int net) {
		Iterator<Mote> it = motes.iterator();
		while (it.hasNext()) {
			Mote m = it.next();
			if (net == m.getNetId())
				it.remove();
		}

		String sql = "delete from motes where net_id=%d";
		sql = String.format(sql, net);
		Main.dao.execute(sql);
	}

	public void updateMote(Mote m) {
		String sql = "update motes set type=%d,ver=%d,net_id=%d where mac='%s'";
		sql = String.format(sql, m.getType(), m.getVer(), m.getNetId(), Mote.strMAC(m.getMac()));
		Main.dao.execute(sql);
	}

}
