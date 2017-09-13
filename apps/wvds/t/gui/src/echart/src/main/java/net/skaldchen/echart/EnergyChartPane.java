package net.skaldchen.echart;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.text.DecimalFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.border.BevelBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.filechooser.FileFilter;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.LineIterator;
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

import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.ExtensionFileFilter;

public class EnergyChartPane extends JPanel {

	private JTextField xMinInput;
	private JTextField xMaxInput;
	private JTextField yMinInput;
	private JTextField yMaxInput;
	private JTextField xRangeInput;

	private TimeSeriesCollection dataset;
	private JFreeChart chart;
	private XYPlot plot;
	private ValueAxis xAxis;
	private ValueAxis yAxis;
	private JScrollPane mainPane;

	private final SimpleDateFormat secFmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
	private final SimpleDateFormat msecFmt = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
	private XYLineAndShapeRenderer renderer;
	private JCheckBox markerBox;
	private JCheckBox markerFillBox;
	private JTextField fileInput;
	private JButton chooseBtn;
	private TimeSeries energys;

	private Date begT = null;
	private Date endT = null;
	private double minY = 0;//Double.MAX_VALUE;
	private double maxY = 0.0000001;//-Double.MAX_VALUE;
	private double YMin = 0.0;
	private double YMax = 0.0000001;
	private final DecimalFormat yFmt = new DecimalFormat("0.000");
	private JButton resetBtn;

	public EnergyChartPane() {
		setLayout(new BorderLayout(0, 0));

		JPanel topPane = new JPanel();
		add(topPane, BorderLayout.NORTH);
		topPane.setBorder(new BevelBorder(BevelBorder.RAISED));

		fileInput = new JTextField();
		topPane.add(fileInput);
		fileInput.setColumns(80);

		chooseBtn = new JButton("Choose");
		topPane.add(chooseBtn);

		chooseBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				File file1 = new File(Config.get("data-dir"));
				JFileChooser chooser = new JFileChooser(file1);
				FileFilter filter1 = new ExtensionFileFilter("CSV file (*.csv)", new String[] { "CSV" });
				chooser.setFileFilter(filter1);
				int returnVal = chooser.showOpenDialog(null);
				if (returnVal == JFileChooser.APPROVE_OPTION) {
					String fname = chooser.getSelectedFile().getPath();
					fileInput.setText(fname);
					SwingUtilities.invokeLater(new Runnable() {
						@Override
						public void run() {
							File f = new File(fname);
							if (f.getParent() != null && !f.getParent().equals(Config.get("data-dir")))
								Config.set("data-dir", f.getParent(), true);
							loadFile(fname);
						}
					});
				}
			}
		});

		mainPane = new JScrollPane();
		add(mainPane);
		mainPane.setBorder(new BevelBorder(BevelBorder.RAISED));

		JPanel botPane = new JPanel();
		add(botPane, BorderLayout.SOUTH);
		botPane.setBorder(new BevelBorder(BevelBorder.RAISED));

		markerBox = new JCheckBox("Marker");
		markerBox.setSelected(false);
		botPane.add(markerBox);
		markerBox.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean vis = markerBox.isSelected();
				//Config.set("chart-marker", String.valueOf(vis), true);
				renderer.setBaseShapesVisible(vis);
			}
		});

		markerFillBox = new JCheckBox("Fill");
		markerFillBox.setSelected(true);
		botPane.add(markerFillBox);
		markerFillBox.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				boolean fill = markerFillBox.isSelected();
				//Config.set("chart-marker-fill", String.valueOf(fill), true);
				renderer.setBaseShapesFilled(fill);
			}
		});

		JLabel sep1 = new JLabel(" ");
		botPane.add(sep1);

		JLabel xMinLabel = new JLabel("MinX");
		botPane.add(xMinLabel);

		Calendar cal = Calendar.getInstance();
		cal.set(Calendar.MILLISECOND, 0);
		cal.set(Calendar.SECOND, 0);
		cal.set(Calendar.MINUTE, 0);
		cal.set(Calendar.HOUR_OF_DAY, 0);

		xMinInput = new JTextField();
		botPane.add(xMinInput);
		xMinInput.setColumns(13);
		xMinInput.setText(msecFmt.format(cal.getTime()));

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

		JLabel xMaxLabel = new JLabel("MaxX");
		botPane.add(xMaxLabel);

		cal.add(Calendar.DAY_OF_YEAR, 1);
		xMaxInput = new JTextField();
		botPane.add(xMaxInput);
		xMaxInput.setColumns(13);
		xMaxInput.setText(msecFmt.format(cal.getTime()));

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

		JLabel yMinLabel = new JLabel("MinY");
		botPane.add(yMinLabel);

		yMinInput = new JTextField();
		botPane.add(yMinInput);
		yMinInput.setColumns(4);
		yMinInput.setText("0");
		//yMinInput.setText(Config.get("chart-y-min"));

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

		JLabel yMaxLabel = new JLabel("MaxY");
		botPane.add(yMaxLabel);

		yMaxInput = new JTextField();
		botPane.add(yMaxInput);
		yMaxInput.setColumns(4);
		yMaxInput.setText("100");
		//yMaxInput.setText(Config.get("chart-y-max"));

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

		JLabel sep3 = new JLabel(" ");
		botPane.add(sep3);
		sep3.setPreferredSize(new Dimension(20, 20));

		resetBtn = new JButton("Reset");
		botPane.add(resetBtn);
		resetBtn.setToolTipText("重置曲线显示所有数据");
		resetBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				xAxis.setRange(begT.getTime(), endT.getTime());
				yAxis.setRange(YMin, YMax);
				xMinInput.setText(msecFmt.format(begT));
				xMaxInput.setText(msecFmt.format(endT));
				yMinInput.setText(yFmt.format(YMin));
				yMaxInput.setText(yFmt.format(YMax));
			}
		});

		createChart();
	}

	private void clearChart() {
		energys.clear();
		begT = null;
		endT = null;
		minY = 0;//Double.MAX_VALUE;
		maxY = 0.0000001;//-Double.MAX_VALUE;
		YMin = 0.0;
		YMax = 0.0000001;
	}

	private void loadFile(String fname) {
		final SimpleDateFormat fmt = new SimpleDateFormat("y-M-d H:m:s");
		final SimpleDateFormat inFmt = new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS");

		File file = new File(fname);
		Calendar cal = Calendar.getInstance();
		boolean indata = false;
		double range = 0.0;
		int total = 0;
		double unit = 1.0;
		long step = 1;
		boolean abstime = false;
		boolean xdone = false;

		clearChart();
		try {
			LineIterator it = FileUtils.lineIterator(file);
			while (it.hasNext()) {
				String line = it.next();
				if (line.startsWith("Start time:")) {
					String[] subs = line.split(",");
					begT = fmt.parse(subs[1]);
					xMinInput.setText(msecFmt.format(begT));
				}
				else if (line.startsWith("Finish time:")) {
					String[] subs = line.split(",");
					endT = fmt.parse(subs[1]);
					xMaxInput.setText(msecFmt.format(endT));
					xAxis.setRange(begT.getTime(), endT.getTime());
					step = (endT.getTime() - begT.getTime()) / total;
					xdone = true;
					indata = true;
				}
				else if (line.startsWith("Measure Range,")) {
					String sRange = line.substring("Measure Range,".length());
					if (sRange.endsWith("mA")) {
						range = Double.parseDouble(sRange.substring(0, sRange.length()-2));
					} else if (sRange.endsWith("A")) {
						range = Double.parseDouble(sRange.substring(0, sRange.length()-1)) * 1000;
					} else {
						throw new RuntimeException("unknown range");
					}
				}
				else if (line.startsWith("Range:,")) {
					String sRange = line.substring("Range:,".length());
					if (sRange.endsWith("mA,")) {
						range = Double.parseDouble(sRange.substring(0, sRange.length()-3));
					} else if (sRange.endsWith("A,")) {
						range = Double.parseDouble(sRange.substring(0, sRange.length()-2)) * 1000;
					} else {
						throw new RuntimeException("unknown range");
					}
				}
				else if (line.startsWith("Data Count,")) {
					total = Integer.parseInt(line.substring("Data Count,".length()));
				}
				else if (line.startsWith("Counter:,")) {
					total = Integer.parseInt(line.substring("Counter:,".length(), line.length()-1));
				}
				else if (line.startsWith("Data Unit,")) {
					String sUnit = line.substring("Data Unit,".length());
					if (sUnit.equals("A")) {
						unit = 1000.0;
					}
				}
				else if (line.startsWith("DataLog Time,")) {
					indata = true;
					abstime = true;
				}
				else if (indata) {
					String[] subs = line.split(",");
					if (subs != null && subs.length > 1) {
						if (abstime) {
							try {
								Date tt = inFmt.parse(subs[0]);
								long t = tt.getTime();
								double val = Double.parseDouble(subs[1]) * unit;
								if (begT == null) {
									begT = tt;
									xMinInput.setText(msecFmt.format(begT));
								}
								endT = tt;
								if (val < range) {
									Millisecond ms = new Millisecond(new Date(t));
									energys.add(ms, val);
									updateY(val);
								} else {
									System.err.println("invalid value at " + msecFmt.format(tt));
								}
							} catch(ParseException e) {
								throw new RuntimeException("unknown time format");
							}
						} else {
							int n = Integer.parseInt(subs[0]);
							long t = begT.getTime() + (n * step);
							double val = Double.parseDouble(subs[1]);
							val *= 1000.0; // in unit of mA
							Millisecond ms = new Millisecond(new Date(t));
							if (val < range) {
								energys.add(ms, val);
								updateY(val);
							} else {
								System.err.println("invalid value at point " + n);
							}
						}
					}
				}
			}
			if (!xdone) {
				xMaxInput.setText(msecFmt.format(endT));
				xAxis.setRange(begT.getTime(), endT.getTime());
			}
		} catch (IOException e) {
			e.printStackTrace();
		} catch (ParseException e) {
			e.printStackTrace();
		}
	}

	private void createChart() {
		dataset = new TimeSeriesCollection();
		chart = ChartFactory.createTimeSeriesChart(
				"",
				"Time",
				"Current (mA)",
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
		renderer.setBaseShapesVisible(markerBox.isSelected());
		renderer.setBaseShapesFilled(markerFillBox.isSelected());

		xAxis = plot.getDomainAxis();
		xAxis.setRange(getXRange());

		yAxis = plot.getRangeAxis();
		yAxis.setRange(getYRange());

		energys = new TimeSeries("current");
		dataset.addSeries(energys);

		mainPane.setViewportView(new ChartPanel(chart));
	}

	private Range getXRange() {
		Range r = null;
		try {
			Date beg = secFmt.parse(xMinInput.getText());
			Date end = secFmt.parse(xMaxInput.getText());
			r = new Range(beg.getTime(), end.getTime());
			return r;
		} catch (ParseException e) {
			e.printStackTrace();
		}

		long now = System.currentTimeMillis();
		r = new Range(now - 1800000, now + 1800000);
		return r;
	}

	private Range getYRange() {
		Range r = null;
		try {
			double beg = Double.parseDouble(yMinInput.getText());
			double end = Double.parseDouble(yMaxInput.getText());
			r = new Range(beg, end);
			return r;
		} catch(Exception e) {
			e.printStackTrace();
		}

		r = new Range(0, 100);
		return r;
	}

	private void updateY(double v) {
		boolean changed = false;
		//if (v < minY) {
		//	minY = v;
		//	changed = true;
		//}
		if (v > maxY) {
			maxY = v;
			changed = true;
		}
		if (changed) {
			if (maxY > minY) {
				double r = maxY - minY;
				YMax = YMin + r * 1.1;
				yAxis.setRange(YMin, YMax);
				yMaxInput.setText(yFmt.format(YMax));
			}
		}
	}

	private void updateYMin() {
		String in = yMinInput.getText();
		if (in != null && in.matches("^[-]?[0-9.]+$")) {
			double v = Double.parseDouble(in);
			Range r = yAxis.getRange();
			if (v < r.getUpperBound()) {
				yAxis.setRange(v, r.getUpperBound());
				//Config.set("chart-y-min", String.valueOf(v), true);
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
				//Config.set("chart-y-max", String.valueOf(v), true);
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

	private void updateXMinMax() {
		String min = xMinInput.getText();
		String max = xMaxInput.getText();
		if (min == null || !min.matches("^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3}$")) {
			return;
		}
		if (max == null || !max.matches("^[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3}$")) {
			return;
		}
		try {
			Date minT = msecFmt.parse(min);
			Date maxT = msecFmt.parse(max);
			if (minT.getTime() >= maxT.getTime())
				return;
			xAxis.setRange(minT.getTime(), maxT.getTime());
		} catch (ParseException e) {
			e.printStackTrace();
		}
	}

}
