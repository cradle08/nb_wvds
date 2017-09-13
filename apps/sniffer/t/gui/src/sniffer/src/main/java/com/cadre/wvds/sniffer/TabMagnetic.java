package com.cadre.wvds.sniffer;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Queue;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.border.BevelBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import org.apache.commons.collections4.queue.CircularFifoQueue;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartPanel;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.axis.ValueAxis;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYLineAndShapeRenderer;
import org.jfree.data.Range;
import org.jfree.data.time.Millisecond;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesCollection;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.I18N;

public class TabMagnetic extends JPanel implements MVCView {

	private static final Logger log = LoggerFactory.getLogger(TabMagnetic.class);

	private JTextField xMinInput;
	private JTextField xMaxInput;
	private JTextField yMinInput;
	private JTextField yMaxInput;
	private JTextField xRangeInput;

	//private final String[] SERIES = { "X1", "Y1", "Z1", "X2", "Y2", "Z2" };
	private final String[] SERIES = { "X", "Y", "Z" };
	private JCheckBox[] seriesCBoxs;
	private TimeSeriesCollection dataset;
	private Map<String, TimeSeries> series;
	private JFreeChart chart;
	private XYPlot plot;
	private ValueAxis xAxis;
	private ValueAxis yAxis;
	private JScrollPane mainPane;
	private ActionListener seriesL;
	private JComboBox<Mote> moteCombo;
	private JTextField moteFilter;

	private JCheckBox xScrollCBox;
	private SimpleDateFormat secFmt;
	private XYLineAndShapeRenderer renderer;
	private JCheckBox markerBox;
	private JCheckBox markerFillBox;
	private JLabel stateLbl;

	private Map<Mote,Queue<MagRecord>> dataMap = new HashMap<Mote,Queue<MagRecord>>();
	private Map<Mote,Integer> stateMap = new HashMap<Mote,Integer>();

	public TabMagnetic() {
		setLayout(new BorderLayout(0, 0));

		JPanel topPane = new JPanel();
		add(topPane, BorderLayout.NORTH);
		topPane.setBorder(new BevelBorder(BevelBorder.RAISED));

		JLabel motePre = new JLabel(I18N.get("label.Node"));
		topPane.add(motePre);

		moteFilter = new JTextField();
		topPane.add(moteFilter);
		moteFilter.setColumns(8);

		moteFilter.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				loadMotes();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				loadMotes();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				loadMotes();
			}
		});

		moteCombo = new JComboBox<Mote>();
		topPane.add(moteCombo);
		moteCombo.setPreferredSize(new Dimension(160, 22));
		loadMotes();

		moteCombo.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				Mote m = (Mote) moteCombo.getSelectedItem();
				loadChart(m);

				Integer s = stateMap.get(m);
				if (s != null)
					updState(s);
			}
		});

		JLabel statusPre = new JLabel(I18N.get("label.Status") + ":");
		topPane.add(statusPre);

		stateLbl = new JLabel(" ");
		topPane.add(stateLbl);
		stateLbl.setPreferredSize(new Dimension(50,20));

		JLabel sep = new JLabel();
		topPane.add(sep);
		sep.setPreferredSize(new Dimension(15, 20));

		JLabel topHintLbl = new JLabel(I18N.get("label.Series"));
		topPane.add(topHintLbl);

		seriesL = new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String act = e.getActionCommand();
				act = act.substring("toggle".length());

				int idx = indexOf(act, SERIES);
				XYPlot xyplot = chart.getXYPlot();
				XYLineAndShapeRenderer r = (XYLineAndShapeRenderer) xyplot.getRenderer(0);
				r.setSeriesVisible(idx, seriesCBoxs[idx].isSelected());
			}
		};

		seriesCBoxs = new JCheckBox[SERIES.length];
		for (int i = 0; i < SERIES.length; i++) {
			seriesCBoxs[i] = new JCheckBox(SERIES[i]);
			topPane.add(seriesCBoxs[i]);
			seriesCBoxs[i].setSelected(Config.getBool("chart-series-" + SERIES[i]));
			seriesCBoxs[i].setActionCommand("toggle" + SERIES[i]);
			seriesCBoxs[i].addActionListener(seriesL);
		}

		mainPane = new JScrollPane();
		add(mainPane);
		mainPane.setBorder(new BevelBorder(BevelBorder.RAISED));

		JPanel botPane = new JPanel();
		add(botPane, BorderLayout.SOUTH);
		botPane.setBorder(new BevelBorder(BevelBorder.RAISED));

		markerBox = new JCheckBox(I18N.get("cbox.Marker"));
		markerBox.setSelected(Config.getBool("chart-marker"));
		botPane.add(markerBox);
		markerBox.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean vis = markerBox.isSelected();
				Config.set("chart-marker", String.valueOf(vis), true);
				renderer.setBaseShapesVisible(vis);
			}
		});

		markerFillBox = new JCheckBox(I18N.get("cbox.Fill"));
		markerFillBox.setSelected(Config.getBool("chart-marker-fill"));
		botPane.add(markerFillBox);
		markerFillBox.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean fill = markerFillBox.isSelected();
				Config.set("chart-marker-fill", String.valueOf(fill), true);
				renderer.setBaseShapesFilled(fill);
			}
		});

		JLabel sep3 = new JLabel(" ");
		botPane.add(sep3);

		xScrollCBox = new JCheckBox();
		xScrollCBox.setText(I18N.get("cbox.XScroll"));
		xScrollCBox.setSelected(Config.getBool("chart-x-scroll"));
		botPane.add(xScrollCBox);

		xScrollCBox.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean scroll = xScrollCBox.isSelected();
				Config.set("chart-x-scroll", String.valueOf(scroll), true);
				setXControl(scroll);
			}
		});

		xRangeInput = new JTextField();
		botPane.add(xRangeInput);
		xRangeInput.setColumns(6);
		xRangeInput.setText(Config.get("chart-x-range"));

		xRangeInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				updateXRange();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				updateXRange();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				updateXRange();
			}
		});

		JLabel xRangeUnit = new JLabel("s");
		botPane.add(xRangeUnit);

		JLabel sep1 = new JLabel(" ");
		botPane.add(sep1);

		JLabel xMinLabel = new JLabel(I18N.get("label.MinX"));
		botPane.add(xMinLabel);

		Calendar cal = Calendar.getInstance();
		cal.set(Calendar.MILLISECOND, 0);
		cal.set(Calendar.SECOND, 0);
		cal.set(Calendar.MINUTE, 0);
		cal.set(Calendar.HOUR_OF_DAY, 0);
		secFmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

		xMinInput = new JTextField();
		botPane.add(xMinInput);
		xMinInput.setColumns(20);
		xMinInput.setText(secFmt.format(cal.getTime()));

		xMinInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				updateXMinMax();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				updateXMinMax();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				updateXMinMax();
			}
		});

		JLabel xMaxLabel = new JLabel(I18N.get("label.MaxX"));
		botPane.add(xMaxLabel);

		cal.add(Calendar.DAY_OF_YEAR, 1);
		xMaxInput = new JTextField();
		botPane.add(xMaxInput);
		xMaxInput.setColumns(20);
		xMaxInput.setText(secFmt.format(cal.getTime()));

		xMaxInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				updateXMinMax();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				updateXMinMax();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				updateXMinMax();
			}
		});

		JLabel sep2 = new JLabel(" ");
		botPane.add(sep2);
		sep2.setPreferredSize(new Dimension(20, 20));

		JLabel yMinLabel = new JLabel(I18N.get("label.MinY"));
		botPane.add(yMinLabel);

		yMinInput = new JTextField();
		botPane.add(yMinInput);
		yMinInput.setColumns(8);
		yMinInput.setText(Config.get("chart-y-min"));

		yMinInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				updateYMin();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				updateYMin();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				updateYMin();
			}
		});

		JLabel yMaxLabel = new JLabel(I18N.get("label.MaxY"));
		botPane.add(yMaxLabel);

		yMaxInput = new JTextField();
		botPane.add(yMaxInput);
		yMaxInput.setColumns(8);
		yMaxInput.setText(Config.get("chart-y-max"));

		yMaxInput.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				updateYMax();
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				updateYMax();
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				updateYMax();
			}
		});

		JLabel sepLbl = new JLabel(" ");
		botPane.add(sepLbl);
		sepLbl.setPreferredSize(new Dimension(20,18));

		JButton resetBtn = new JButton(I18N.get("button.reset"));
		botPane.add(resetBtn);
		resetBtn.setPreferredSize(new Dimension(60,22));
		resetBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					double ymin = Double.parseDouble(yMinInput.getText());
					double ymax = Double.parseDouble(yMaxInput.getText());
					yAxis.setRange(ymin, ymax);

					if (xScrollCBox.isSelected()) {
						double xran = Double.parseDouble(xRangeInput.getText()) * 1000;
						double xmax = System.currentTimeMillis() + xran / 4;
						double xmin = xmax - xran;
						xAxis.setRange(xmin, xmax);
					} else {
						Date xmax = secFmt.parse(xMaxInput.getText());
						Date xmin = secFmt.parse(xMinInput.getText());
						xAxis.setRange(xmin.getTime(), xmax.getTime());
					}
				} catch(Exception ex) {
					ex.printStackTrace();
				}
			}
		});

		createChart();

		setXControl(Config.getBool("chart-x-scroll"));
	}

	private void loadMotes() {
		String filter = moteFilter.getText();
		List<Mote> motes = MoteManager.getInstance().getMotes();
		moteCombo.removeAllItems();
		for (Mote m : motes) {
			if (m.getDev()[0] != WVDS.VD)
				continue;
			if (((filter != null) && !filter.matches("^[ \t]*$")) && !m.toString().matches(String.format("^.*%s.*", filter)))
				continue;
			moteCombo.addItem(m);
		}
	}

	private void createChart() {
		dataset = new TimeSeriesCollection();
		chart = ChartFactory.createTimeSeriesChart(
				"",
				"Time",
				"Magnetic",
				dataset,
				true,
				true,
				false
				);
		plot = chart.getXYPlot();
		plot.setBackgroundPaint(Color.white);
		plot.setDomainGridlinePaint(Color.lightGray);
		plot.setRangeGridlinePaint(Color.lightGray);

		renderer = (XYLineAndShapeRenderer) plot.getRenderer(0);
		renderer.setBaseShapesVisible(Config.getBool("chart-marker"));
		renderer.setBaseShapesFilled(Config.getBool("chart-marker-fill"));

		xAxis = plot.getDomainAxis();
		xAxis.setAutoRange(true);
		xAxis.setFixedAutoRange(Config.getInt("chart-x-range")*1000.0);

		yAxis = plot.getRangeAxis();
		yAxis.setRange(Config.getDouble("chart-y-min"), Config.getDouble("chart-y-max"));

		series = new HashMap<String,TimeSeries>();
		for (int i = 0; i < SERIES.length; i++) {
			TimeSeries ts = new TimeSeries(SERIES[i]);
			series.put(SERIES[i], ts);
			dataset.addSeries(ts);
		}

		mainPane.setViewportView(new ChartPanel(chart));
	}

	private String strState(int state) {
		String str = null;
		switch(state) {
		case 0: str = "无车"; break;
		case 1: str = "有车"; break;
		case 2: str = "待激活"; break;
		case 3: str = "标定中"; break;
		default: str = "未知"; break;
		}
		return str;
	}

	private Color bgColor(int state) {
		Color c = null;
		switch(state) {
		case 0: c = Color.green; break;
		case 1: c = Color.red; break;
		case 3: c = Color.yellow; break;
		default: c = null; break;
		}
		return c;
	}

	private Color fgColor(int state) {
		Color c = null;
		switch(state) {
		case 0: c = Color.black; break;
		case 1: c = Color.white; break;
		case 3: c = Color.black; break;
		default: c = Color.black; break;
		}
		return c;
	}

	public void addPacket(Packet p, Date rtime) {
		if (p == null)
			return;
		if (!("park test".equals(p.getName())
				|| "park evt".equals(p.getName())
				|| "mag data".equals(p.getName())
				|| "vd hbeat".equals(p.getName())))
			return;

		if ("park evt".equals(p.getName()) || "vd hbeat".equals(p.getName())) {
			byte[] dev = p.getField("dev").value;
			byte[] mac = new byte[8];
			System.arraycopy(dev, 0, mac, 2, 6);
			Mote m = MoteManager.getInstance().getMoteByDev(dev);
			if (m == null) {
				m = new Mote();
				m.setDev(dev);
				m.setMac(mac);
				m.setSaddr(p.getField("esender").toInt());
				MoteManager.getInstance().addMote(m);
			}

			MagRecord mr = new MagRecord();
			mr.t = rtime.getTime();
			mr.x = p.getField("magX").toInt();
			mr.y = p.getField("magY").toInt();
			mr.z = p.getField("magZ").toInt();
			mr.s = p.getField("status").toInt();
			addMagRecord(m, mr);
			stateMap.put(m, mr.s);
		}
		else if ("park test".equals(p.getName())) {
			int nid = p.getField("from").toInt();
			int id = p.getField("id").toInt();
			Mote m = MoteManager.getInstance().getMote(nid);
			if (m == null) {
				byte[] dev = new byte[6];
				byte[] mac = new byte[8];
				dev[0] = (byte)WVDS.VD; dev[5] = (byte)nid;
				mac[2] = (byte)WVDS.VD; mac[7] = (byte)nid;
				m = new Mote();
				m.setSaddr(nid);
				m.setDev(dev);
				m.setMac(mac);
				m.setName(String.format("test%d", nid));
				MoteManager.getInstance().addMote(m);
			}

			MagRecord mr = new MagRecord();
			mr.t = rtime.getTime();
			mr.x = p.getField("x2").toInt();
			mr.y = p.getField("y2").toInt();
			mr.z = p.getField("z2").toInt();
			mr.s = p.getField("state").toInt();
			addMagRecord(m, mr);
			stateMap.put(m, mr.s);
		}
		else if ("mag data".equals(p.getName())) {
			byte[] dev = p.getField("dev").value;
			byte[] mac = new byte[8];
			System.arraycopy(dev, 0, mac, 2, 6);
			Mote m = MoteManager.getInstance().getMoteByDev(dev);
			if (m == null) {
				m = new Mote();
				m.setDev(dev);
				m.setMac(mac);
				m.setSaddr(p.getField("esender").toInt());
				MoteManager.getInstance().addMote(m);
			}

			byte[] xyz = p.getField("magXYZ").value;
			for (int i = 0; i < 10; i++) {
				MagRecord mr = new MagRecord();
				mr.t = rtime.getTime() + (100 * i);
				mr.x = BytesUtil.getBEInt16(xyz, 6*i+0);
				mr.y = BytesUtil.getBEInt16(xyz, 6*i+2);
				mr.z = BytesUtil.getBEInt16(xyz, 6*i+4);
				mr.s = p.getField("status").toInt();
				if ((mr.x != 0) || (mr.y != 0) || (mr.z != 0)) {
					addMagRecord(m, mr);
					stateMap.put(m, mr.s);
				}
			}
		}
	}

	private void addMagRecord(Mote m, MagRecord r) {
		log.debug("addMagRecord: [" + m + "], " + r);
		Queue<MagRecord> q = dataMap.get(m);
		if (q == null) {
			int size = Config.getInt("chart-points-num");
			q = new CircularFifoQueue<MagRecord>(size);
			dataMap.put(m, q);
		}
		q.add(r);

		Mote sel = (Mote) moteCombo.getSelectedItem();
		if (m == sel) {
			addMagPoint(r);
		}
	}

	private void addMagPoint(MagRecord mr) {
		log.debug("addMagPoint: " + mr);
		for (int i = 0; i < SERIES.length; i++) {
			String s = SERIES[i];
			TimeSeries ts = series.get(s);
			if (ts != null) {
				Date t = new Date(mr.t);
				Millisecond ms = new Millisecond(t);
				ts.addOrUpdate(ms, getVal(mr, s));
			}
		}
		updState(mr.s);

		Range r = xAxis.getRange();
		double xmax = r.getUpperBound();
		double xmin = r.getLowerBound();
		double step = r.getLength() / 10.0;
		if (mr.t > (xmax - step)) {
			xmax = mr.t + step;
			xmin = xmax - r.getLength();
			xAxis.setRange(xmin, xmax);
		}
	}

	private void updState(int s) {
		stateLbl.setOpaque(true);
		stateLbl.setText(strState(s));
		stateLbl.setBackground(bgColor(s));
		stateLbl.setForeground(fgColor(s));
	}

	private double getVal(MagRecord rec, String s) {
		double v = 0.0;
		if (s.equals("X")) { v = rec.x; }
		if (s.equals("Y")) { v = rec.y; }
		if (s.equals("Z")) { v = rec.z; }
		//if (s.equals("X1")) { v = rec.mag1x; }
		//if (s.equals("Y1")) { v = rec.mag1y; }
		//if (s.equals("Z1")) { v = rec.mag1z; }
		//if (s.equals("X2")) { v = rec.mag2x; }
		//if (s.equals("Y2")) { v = rec.mag2y; }
		//if (s.equals("Z2")) { v = rec.mag2z; }
		return v;
	}

	private void updateYMin() {
		String in = yMinInput.getText();
		if (in != null && in.matches("^[-]?[0-9.]+$")) {
			double v = Double.parseDouble(in);
			Range r = yAxis.getRange();
			if (v < r.getUpperBound()) {
				yAxis.setRange(v, r.getUpperBound());
				Config.set("chart-y-min", String.valueOf(v), true);
			}
		}
	}

	private void updateYMax() {
		String in = yMaxInput.getText();
		if (in != null && in.matches("^[-]?[0-9.]+$")) {
			double v = Double.parseDouble(in);
			Range r = yAxis.getRange();
			if (v > r.getLowerBound()) {
				yAxis.setRange(r.getLowerBound(), v);
				Config.set("chart-y-max", String.valueOf(v), true);
			}
		}
	}

	private void setXControl(boolean scroll) {
		xRangeInput.setEnabled(scroll);
		xMinInput.setEnabled(!scroll);
		xMaxInput.setEnabled(!scroll);

		if (scroll) {
			long now = System.currentTimeMillis();
			int r = Integer.parseInt(xRangeInput.getText()) * 1000;
			xAxis.setAutoRange(true);
			xAxis.setFixedAutoRange(r);
			xAxis.setRange(now - r/2, now + r/2);
		} else {
			try {
				Date beg = secFmt.parse(xMinInput.getText());
				Date end = secFmt.parse(xMaxInput.getText());
				xAxis.setAutoRange(false);
				xAxis.setRange(beg.getTime(), end.getTime());
			} catch (ParseException e1) {
				e1.printStackTrace();
			}
		}
	}

	private void updateXRange() {
		String in = xRangeInput.getText();
		if (in != null && in.matches("^[0-9]+$")) {
			int v = Integer.parseInt(in);
			if (v > 0) {
				Config.set("chart-x-range", String.valueOf(v), true);
				v = v * 1000;
				long now = System.currentTimeMillis();
				xAxis.setFixedAutoRange(v);
				xAxis.setRange(now - v/2, now + v/2);
			}
		}
	}

	private void updateXMinMax() {
		String min = xMinInput.getText();
		String max = xMaxInput.getText();
		if (min == null || !min.matches("^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}$")) {
			return;
		}
		if (max == null || !max.matches("^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}$")) {
			return;
		}
		try {
			Date minT = secFmt.parse(min);
			Date maxT = secFmt.parse(max);
			if (minT.getTime() >= maxT.getTime())
				return;
			xAxis.setRange(minT.getTime(), maxT.getTime());
		} catch (ParseException e) {
			e.printStackTrace();
		}
	}

	private int indexOf(String str, String[] arr) {
		for (int i = 0; i < arr.length; i++)
			if (str.equals(arr[i]))
				return i;
		return -1;
	}

	private void loadChart(Mote m) {
		if (m == null)
			return;

		Iterator<Entry<String, TimeSeries>> it = series.entrySet().iterator();
		while (it.hasNext()) {
			Entry<String, TimeSeries> et = it.next();
			et.getValue().clear();
		}

		Queue<MagRecord> q = dataMap.get(m);
		if (q != null) {
			for (MagRecord mr : q)
				addMagPoint(mr);
		}
	}

	@Override
	public void dataAdded(Object o) {
		if (o instanceof Mote) {
			Mote m = (Mote)o;
			if (m.getDev()[0] == WVDS.VD)
				moteCombo.addItem(m);
		}
	}

	@Override
	public void dataRemoved(Object o) {
		if (o instanceof Mote) {
			moteCombo.removeItem((Mote)o);
		}
	}

	@Override
	public void dataChanged(Object o) {

	}

}
