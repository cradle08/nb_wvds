package com.cadre.wvds.wvdsota;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class SubnetManager {

	private static SubnetManager instance;

	private List<Subnet> subnets;
	private List<MVCView> views;

	private SubnetManager() {
		this.subnets = new ArrayList<Subnet>();
		this.views = new ArrayList<MVCView>();
	}

	public static SubnetManager getInstance() {
		if (instance == null) {
			instance = new SubnetManager();
			instance.load();
		}
		return instance;
	}

	private void load() {
		String sql = "select id,city,district,street,rfchan,rfpower,vd_ver,vd_file,vd_size,rp_ver,rp_file,rp_size,ap_ver,ap_file,ap_size from subnets";
		List<Map<String, Object>> res = Main.dao.queryAsList(sql);
		Iterator<Map<String, Object>> it = res.iterator();
		subnets.clear();
		while (it.hasNext()) {
			Map<String, Object> e = it.next();
			Subnet net = new Subnet();
			net.id = (Integer)e.get("id");
			net.city = (String)e.get("city");
			net.district = (String)e.get("district");
			net.street = (String)e.get("street");
			net.rfchan = (Integer)e.get("rfchan");
			net.rfpower = (Integer)e.get("rfpower");
			net.vdVer = (Integer)e.get("vd_ver");
			net.rpVer = (Integer)e.get("rp_ver");
			net.apVer = (Integer)e.get("ap_ver");
			net.vdFile = (String)e.get("vd_file");
			net.rpFile = (String)e.get("rp_file");
			net.apFile = (String)e.get("ap_file");
			net.vdSize = (Integer)e.get("vd_size");
			net.rpSize = (Integer)e.get("rp_size");
			net.apSize = (Integer)e.get("ap_size");
			subnets.add(net);
		}
	}

	public List<Subnet> getSubnets() {
		return subnets;
	}

	public void addSubnet(Subnet net) {
		int nextId = 0;
		for (Subnet sub : subnets) {
			if (sub.id > nextId)
				nextId = sub.id;
		}
		nextId += 1;
		net.id = nextId;
		subnets.add(net);

		String sql = "insert into subnets(id,city,district,street,rfchan,rfpower,vd_ver,vd_file,vd_size,rp_ver,rp_file,rp_size,ap_ver,ap_file,ap_size)"
				+ " values(%d,'%s','%s','%s',%d,%d,%d,'%s',%d,%d,'%s',%d,%d,'%s',%d)";
		sql = String.format(sql, net.id, net.city, net.district, net.street, net.rfchan, net.rfpower,
				net.vdVer, net.vdFile, net.vdSize, net.rpVer, net.rpFile, net.rpSize, net.apVer, net.apFile, net.apSize);
		Main.dao.execute(sql);

		notifyViews("add", net);
	}

	public void delSubnet(Subnet net) {
		subnets.remove(net);

		String sql = "delete from subnets where id=%d";
		sql = String.format(sql, net.id);
		Main.dao.execute(sql);

		notifyViews("del", net);
	}

	public void updSubnet(Subnet net) {
		String sql = "update subnets set rfchan=%d,rfpower=%d,vd_ver=%d,vd_file='%s',vd_size=%d,rp_ver=%d,rp_file='%s',rp_size=%d,ap_ver=%d,ap_file='%s',ap_size=%d where id=%d";
		sql = String.format(sql, net.rfchan, net.rfpower, net.vdVer, net.vdFile, net.vdSize, net.rpVer, net.rpFile, net.rpSize, net.apVer, net.apFile, net.apSize, net.id);
		Main.dao.execute(sql);
	}

	public void addView(MVCView view) {
		if (!views.contains(view))
			views.add(view);
	}
	
	public void removeView(MVCView view) {
		if (views.contains(view))
			views.remove(view);
	}

	private void notifyViews(String evt, Object obj) {
		for (MVCView view : views) {
			if ("add".equals(evt)) view.dataAdded(obj);
			else if ("del".equals(evt)) view.dataRemoved(obj);
			else if ("cha".equals(evt)) view.dataChanged(obj);
		}
	}
}
