package com.cadre.wvds.sniffer;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;
import javax.swing.RowFilter;
import javax.swing.border.BevelBorder;
import javax.swing.border.TitledBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.alibaba.fastjson.JSON;

import net.skaldchen.commons.packet.Packet;
import net.skaldchen.commons.serial.PacketListener;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.JTableUtil;

public class MoteCtrlDialog extends JDialog implements PacketListener, MVCView {

	private static final Logger log = LoggerFactory.getLogger(MoteCtrlDialog.class);

	private JRadioButton modeDisabledRB;
	private JRadioButton modeActivateRB;
	private JRadioButton modeMaintainRB;

	private JTextField chanInput;

	private List<Param> params;
	private JLabel[] labels;
	private JTextField[] inputs;

	private int W = 600;
	private int H = 420;

	private final String[] HEADER = new String[]{"车位号","设备号","P","C","状态"};
	private final int[] WIDTHs = new int[]{60,90,20,20,120};
	private final int COL_NAME   = 0;
	private final int COL_DEVNO  = 1;
	private final int COL_PARK   = 2;
	private final int COL_CONN   = 3;
	private final int COL_STATUS = 4;

	private DefaultTableModel tableM;
	private JTable table;
	private ActionListener tableAL;
	private JTextField filterText;
	private TableRowSorter<TableModel> rowSorter;

	private String pendingOp = null;
	private int pendingRow = -1;

	public MoteCtrlDialog(JFrame parent) {
		super(parent, false);
		setTitle("设备控制");
		setSize(W,H);
		setAlwaysOnTop(true);
		setResizable(false);
		setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);

		addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent e) {
				exit();
			}
		});

		JPanel mainPane = new JPanel();
		getContentPane().add(mainPane, BorderLayout.CENTER);
		mainPane.setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));
		mainPane.setLayout(new BorderLayout(0, 0));

		JPanel listPane = new JPanel();
		listPane.setBorder(new TitledBorder(null, "设备列表", TitledBorder.LEADING, TitledBorder.TOP, null, null));
		mainPane.add(listPane, BorderLayout.CENTER);
		listPane.setPreferredSize(new Dimension(W-177,H-40));
		listPane.setLayout(null);

		tableM = new DefaultTableModel(HEADER, 0);
		table = new JTable(tableM);
		table.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

		JScrollPane scrollPane = new JScrollPane();
		listPane.add(scrollPane);
		scrollPane.setViewportView(table);
		scrollPane.setBounds(10,23,W-273,H-100);

		JTableUtil.fitTableColumns(table, WIDTHs);

		rowSorter = new TableRowSorter<TableModel>(tableM);
		table.setRowSorter(rowSorter);

		TableColumn parkCol = table.getColumnModel().getColumn(COL_PARK);
		parkCol.setCellRenderer(new VDStateCellRenderer());

		TableColumn connCol = table.getColumnModel().getColumn(COL_CONN);
		connCol.setCellRenderer(new VDConnCellRenderer());

		JPanel ctrlPane = new JPanel();
		listPane.add(ctrlPane);
		ctrlPane.setBounds(W-260, 23, 70, H-100);
		ctrlPane.setLayout(null);

		JLabel filterPre = new JLabel("过滤:");
		ctrlPane.add(filterPre);
		filterPre.setBounds(5, 5, 58, 20);

		filterText = new JTextField();
		ctrlPane.add(filterText);
		filterText.setBounds(5, 26, 58, 20);

		filterText.getDocument().addDocumentListener(new DocumentListener() {
			@Override
			public void removeUpdate(DocumentEvent e) {
				setFilter(filterText.getText());
			}

			@Override
			public void insertUpdate(DocumentEvent e) {
				setFilter(filterText.getText());
			}

			@Override
			public void changedUpdate(DocumentEvent e) {
				setFilter(filterText.getText());
			}
		});

		tableAL = new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				int[] rows = table.getSelectedRows();
				if (rows.length != 1) {
					String message = "请选中列表中的一个设备后再点击操作";
					JOptionPane.showMessageDialog(MoteCtrlDialog.this, message);
					return;
				}

				String act = e.getActionCommand();
				byte[] dev = Mote.toDev((String)table.getValueAt(rows[0], COL_DEVNO));
				WVDSNetwork nwk = (WVDSNetwork)Main.nwk;
				Mote m = MoteManager.getInstance().getMoteByDev(dev);
				int to = m.getSaddr();
				if ((to == 65534) || (to == -1)) {
					String message = "该设备的通讯短地址未知，请激活后再操作";
					JOptionPane.showMessageDialog(MoteCtrlDialog.this, message);
					return;
				}

				pendingOp = act;
				pendingRow = rows[0];
				if ("reset".equals(act)) {
					table.setValueAt("正在重启...", rows[0], COL_STATUS);
					nwk.reset(to, dev);
				}
				else if ("freset".equals(act)) {
					table.setValueAt("正在恢复出厂...", rows[0], COL_STATUS);
					nwk.freset(to, dev);
				}
				else if ("reinit".equals(act)) {
					table.setValueAt("正在标定中...", rows[0], COL_STATUS);
					table.setValueAt(WVDS.strState(3), rows[0], COL_PARK);
					nwk.reinit(to, dev);
				}
				else if ("algoGet".equals(act)) {
					table.setValueAt("正在查询参数...", rows[0], COL_STATUS);
					for (JTextField t : inputs)
						t.setText("");
					nwk.algoGet(to, dev);
				}
				else if ("algoSet".equals(act)) {
					int num = inputs.length;
					for (int j = 0; j < num; j++) {
						String str = inputs[j].getText();
						if (str == null || !str.matches("^[-]?[0-9]+$")) {
							String message = String.format("\"%s\"的输入不合法，应为数字", labels[j].getText());
							JOptionPane.showMessageDialog(MoteCtrlDialog.this, message);
							return;
						}
					}

					table.setValueAt("正在设置参数...", rows[0], COL_STATUS);
					AlgoParam algo = new AlgoParam();
					int i = 0;
					algo.normalT = Integer.parseInt(inputs[i++].getText());
					algo.flunctT = Integer.parseInt(inputs[i++].getText());
					algo.bigOccThr = Integer.parseInt(inputs[i++].getText());
					algo.midOccThr = Integer.parseInt(inputs[i++].getText());
					algo.litOccThr = Integer.parseInt(inputs[i++].getText());
					algo.unOccThr = Integer.parseInt(inputs[i++].getText());
					algo.baseLineX = Integer.parseInt(inputs[i++].getText());
					algo.baseLineY = Integer.parseInt(inputs[i++].getText());
					algo.baseLineZ = Integer.parseInt(inputs[i++].getText());
					algo.sensorGain = Integer.parseInt(inputs[i++].getText());
					nwk.algoSet(to, dev, algo);
				}
				else if ("disconn".equals(act)) {
					table.setValueAt("正在断开...", rows[0], COL_STATUS);
					String str = chanInput.getText();
					if ((str == null) || !str.matches("^[0-9]+$")) {
						String message = "输入的信道值非法，应为整数";
						JOptionPane.showMessageDialog(MoteCtrlDialog.this, message);
						return;
					}

					int chan = Integer.parseInt(str);
					int min = Config.getInt("channel-min");
					int max = Config.getInt("channel-max");
					if ((chan < min) || (chan > max)) {
						String message = String.format("输入的信道值非法，范围应是[%d,%d]", min, max);
						JOptionPane.showMessageDialog(MoteCtrlDialog.this, message);
						return;
					}

					nwk.disconn(to, dev, chan);
				}
			}
		};

		modeDisabledRB = new JRadioButton("禁用");
		modeActivateRB = new JRadioButton("激活");
		modeMaintainRB = new JRadioButton("维护");
		ButtonGroup modeBG = new ButtonGroup();
		modeBG.add(modeDisabledRB);
		modeBG.add(modeActivateRB);
		modeBG.add(modeMaintainRB);
		modeDisabledRB.setSelected(true);

		ctrlPane.add(modeDisabledRB); modeDisabledRB.setBounds(3, 59, 50, 18);
		ctrlPane.add(modeActivateRB); modeActivateRB.setBounds(3, 77, 50, 18);
		ctrlPane.add(modeMaintainRB); modeMaintainRB.setBounds(3, 95, 50, 18);

		JButton algoGetBtn = new JButton("参数查询");
		algoGetBtn.setMargin(new Insets(0,0,0,0));
		algoGetBtn.setBounds(5, 120, 60, 24);
		algoGetBtn.setActionCommand("algoGet");
		algoGetBtn.addActionListener(tableAL);
		ctrlPane.add(algoGetBtn);

		JButton algoSetBtn = new JButton("参数配置");
		algoSetBtn.setMargin(new Insets(0,0,0,0));
		algoSetBtn.setBounds(5, 150, 60, 24);
		algoSetBtn.setActionCommand("algoSet");
		algoSetBtn.addActionListener(tableAL);
		ctrlPane.add(algoSetBtn);

		JButton reinitBtn = new JButton("重新标定");
		reinitBtn.setMargin(new Insets(0,0,0,0));
		reinitBtn.setBounds(5, 180, 60, 24);
		reinitBtn.setActionCommand("reinit");
		reinitBtn.addActionListener(tableAL);
		ctrlPane.add(reinitBtn);

		JButton resetBtn = new JButton("重新启动");
		resetBtn.setMargin(new Insets(0,0,0,0));
		resetBtn.setBounds(5, 210, 60, 24);
		resetBtn.setActionCommand("reset");
		resetBtn.addActionListener(tableAL);
		ctrlPane.add(resetBtn);

		JButton fresetBtn = new JButton("恢复出厂");
		fresetBtn.setMargin(new Insets(0,0,0,0));
		fresetBtn.setBounds(5, 240, 60, 24);
		fresetBtn.setActionCommand("freset");
		fresetBtn.addActionListener(tableAL);
		ctrlPane.add(fresetBtn);

		JButton activateBtn = new JButton("断开连接");
		activateBtn.setMargin(new Insets(0,0,0,0));
		activateBtn.setBounds(5, 270, 60, 24);
		activateBtn.setActionCommand("disconn");
		activateBtn.addActionListener(tableAL);
		ctrlPane.add(activateBtn);

		JLabel chanPre = new JLabel("信道");
		ctrlPane.add(chanPre);
		chanPre.setBounds(5, 300, 30, 18);

		chanInput = new JTextField();
		chanInput.setText("6");
		ctrlPane.add(chanInput);
		chanInput.setBounds(35, 300, 30, 20);

		JPanel algoPane = new JPanel();
		algoPane.setBorder(new TitledBorder(null, "算法参数", TitledBorder.LEADING, TitledBorder.TOP, null, null));
		mainPane.add(algoPane, BorderLayout.EAST);
		algoPane.setPreferredSize(new Dimension(177,H-40));
		algoPane.setLayout(null);

		String text = "[]";
		try {
			text = FileUtils.readFileToString(new File("dat/algo.json"), "UTF-8");
		} catch (IOException e) {
			e.printStackTrace();
		}
		params = JSON.parseArray(text, Param.class);

		if (params.size() > 0) {
			labels = new JLabel[params.size()];
			inputs = new JTextField[params.size()];
			int i = 0;
			for (Param p : params) {
				labels[i] = new JLabel(p.getName());
				algoPane.add(labels[i]);
				labels[i].setBounds(15, 25+30*i, 100, 20);

				inputs[i] = new JTextField();
				algoPane.add(inputs[i]);
				inputs[i].setColumns(8);
				inputs[i].setBounds(115, 25+30*i, 50, 20);
				inputs[i].setText(p.getValue());
				//if (i > 8) inputs[i].setEnabled(false);
				++i;
			}
		}

		JButton loadAlgoBtn = new JButton("加载默认");
		loadAlgoBtn.setMargin(new Insets(0, 0, 0, 0));
		algoPane.add(loadAlgoBtn);
		loadAlgoBtn.setBounds(106, 322, 60, 24);
		loadAlgoBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				int i = 0;
				for (Param p : params) {
					if (6 <= i && i <= 8)
						continue;
					inputs[i++].setText(p.getValue());
				}
			}
		});

		JPanel botPane = new JPanel();
		getContentPane().add(botPane, BorderLayout.SOUTH);

		JButton closeBtn = new JButton("关闭");
		botPane.add(closeBtn);
		closeBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				exit();
			}
		});

		load();
	}

	private void exit() {
		Main.comm.removePacketListener(this);
		MoteManager.getInstance().removeView(this);
		dispose();
	}

	private void load() {
		MoteManager motemgr = MoteManager.getInstance();
		List<Mote> motes = motemgr.getMotes();
		for (Mote m : motes) {
			if (m.getDev()[0] == WVDS.VD)
				addMoteRow(m);
		}
	}

	private void addMoteRow(Mote m) {
		Object[] rowData = new Object[HEADER.length];
		int i = 0;
		rowData[i++] = m.getName();
		rowData[i++] = Mote.strDev(m.getDev());
		rowData[i++] = "未知";
		rowData[i++] = "未连接";
		rowData[i++] = "";
		tableM.addRow(rowData);
	}

	private void removeMoteRow(Mote m) {
		String dev = Mote.strDev(m.getDev());
		for (int i = 0; i < table.getRowCount(); i++) {
			if (dev.equals(table.getValueAt(i, COL_DEVNO)))
				tableM.removeRow(i);
		}
	}

	private Mote rowToMote(int r) {
		Mote m = null;
		if (r != -1) {
			String str = (String)table.getValueAt(r, COL_DEVNO);
			byte[] dev = Mote.toDev(str);
			m = MoteManager.getInstance().getMoteByDev(dev);
		}
		return m;
	}

	private int rowOfMote(byte[] dev) {
		String str = Mote.strDev(dev);
		for (int i = 0; i < table.getRowCount(); i++) {
			if (str.equals(table.getValueAt(i, COL_DEVNO)))
				return i;
		}
		return -1;
	}

	private void setFilter(String filter) {
		if (filter == null)
			return;
		rowSorter.setRowFilter(RowFilter.regexFilter(filter));
	}

	@Override
	public void packetReceived(byte[] pkt) {
		log.info(String.format("rcvd: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
		if ("hdlc".equals(Config.get("serial-proto"))) {
			if (pkt[0] == Const.SNIFFER_MSG) {
				byte[] packet = BytesUtil.getBytes(pkt, 5, pkt.length-6);
				Packet p = Main.parser.parse(packet);
				if (p == null)
					return;

				MoteManager mgr = MoteManager.getInstance();
				WVDSNetwork nwk = (WVDSNetwork)Main.nwk;
				if ("vd act req".equals(p.getName())) {
					if (modeActivateRB.isSelected()) {
						byte[] dev = p.getField("dev").value;
						int nid = p.getField("from").toInt();
						Mote m = mgr.getMoteByDev(dev);
						if (m != null) {
							if (nid != m.getSaddr())
								m.setSaddr(nid);

							if (!m.conned) {
								pendingOp = "conn";
								pendingRow = rowOfMote(dev);
								nwk.actack(m.getSaddr(), dev);
							}
						}
					}
				}
				else if ("netcmd req".equals(p.getName())) {
					if (modeMaintainRB.isSelected()) {
						int nid = p.getField("from").toInt();
						byte[] mac = p.getField("mac").value;
						byte[] dev = new byte[6];
						System.arraycopy(mac, 2, dev, 0, 6);

						Mote m = mgr.getMoteByDev(dev);
						if (m != null) {
							if (nid != m.getSaddr())
								m.setSaddr(nid);

							if (!m.conned) {
								pendingOp = "conn";
								pendingRow = rowOfMote(dev);
								nwk.connack(m.getSaddr(), dev);
							}
						}
					}
				}
				else if ("netcmd ack".equals(p.getName())) {
					int r = pendingRow;
					if (r != -1) {
						if ("reinit".equals(pendingOp)) {
							table.setValueAt(WVDS.strState(3), r, COL_PARK);
							focusAndSelMoteRow(r);
							pendingOp = null;
						}
						else if ("conn".equals(pendingOp)) {
							table.setValueAt("已连接", r, COL_CONN);
							table.setValueAt("已连接", r, COL_STATUS);
							focusAndSelMoteRow(r);
							pendingOp = null;
							Mote m = rowToMote(r);
							m.conned = true;
						}
						else if ("disconn".equals(pendingOp)) {
							table.setValueAt("已断开", r, COL_CONN);
							focusMoteRow(r);
							pendingOp = null;
							Mote m = rowToMote(r);
							m.conned = false;
						}
					}
				}
				else if ("connect".equals(p.getName())) {
					int nid = p.getField("from").toInt();
					byte[] dev = p.getField("dev").value;

					if (modeMaintainRB.isSelected()) {
						int[] rows = table.getSelectedRows();
						if (rows.length == 1) {
							byte[] sel = Mote.toDev((String)table.getValueAt(rows[0], COL_DEVNO));
							if (Arrays.equals(dev, sel)) {
								Mote m = mgr.getMoteByDev(dev);
								if (m != null && !m.conned) {
									pendingOp = "conn";
									pendingRow = rowOfMote(dev);
									nwk.connack(nid, dev);
								}
							}
						}
					}
				}
				else if ("park evt".equals(p.getName()) || "vd hbeat".equals(p.getName())) {
					int nid = p.getField("from").toInt();
					byte[] dev = p.getField("dev").value;
					int s = p.getField("status").toInt();

					int r = rowOfMote(dev);
					if (r != -1) {
						table.setValueAt(WVDS.strState(s), r, COL_PARK);
						focusMoteRow(r);
					}

					if (modeMaintainRB.isSelected()) {
						int[] rows = table.getSelectedRows();
						if (rows.length == 1) {
							byte[] sel = Mote.toDev((String)table.getValueAt(rows[0], COL_DEVNO));
							if (Arrays.equals(dev, sel)) {
								Mote m = mgr.getMoteByDev(dev);
								if (m != null && !m.conned) {
									pendingOp = "conn";
									pendingRow = rowOfMote(dev);
									nwk.connack(nid, dev);
								}
							}
						}
					}
				}
				else if ("fae algo resp".equals(p.getName())) {
					int subop = p.getField("subop").toInt();
					int ok = p.getField("res").toInt();

					if (subop == 1) { // 查询应答
						byte[] dev = p.getField("dev").value;
						int r = rowOfMote(dev);
						if (r != -1) {
							if (ok == 0) {
								table.setValueAt("参数查询成功", r, COL_STATUS);
							} else {
								table.setValueAt("参数查询失败", r, COL_STATUS);
							}
							focusAndSelMoteRow(r);

							int i = 0;
							inputs[i++].setText(String.valueOf(p.getField("normalT").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("flunctT").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("bigThr").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("midThr").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("litThr").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("unThr").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("baseX").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("baseY").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("baseZ").toInt()));
							inputs[i++].setText(String.valueOf(p.getField("gain").toInt()));
						}
					}
					else if (subop == 2) { // 配置应答
						byte[] dev = p.getField("dev").value;
						int r = rowOfMote(dev);
						if (r != -1) {
							if (ok == 0) {
								table.setValueAt("参数配置成功", r, COL_STATUS);
							} else {
								table.setValueAt("参数配置失败", r, COL_STATUS);
							}
							focusAndSelMoteRow(r);
						}
					}
				}
				else if ("fae reinit resp".equals(p.getName())) {
					byte[] dev = p.getField("dev").value;
					int r = rowOfMote(dev);
					if (r != -1) {
						int ok = p.getField("res").toInt();
						if (ok == 0) {
							table.setValueAt("标定成功", r, COL_STATUS);
							table.setValueAt(WVDS.strState(0), r, COL_PARK);
						} else {
							table.setValueAt("标定失败,错误" + ok, r, COL_STATUS);
						}
						focusAndSelMoteRow(r);
					}
				}
				else if ("fae disconn ack".equals(p.getName()) || "fae disconn req".equals(p.getName())) {
					byte[] dev = p.getField("dev").value;
					int r = rowOfMote(dev);
					if (r != -1) {
						table.setValueAt("已断开", r, COL_CONN);
						table.setValueAt("已断开", r, COL_STATUS);
						focusMoteRow(r);
					}
				}
				else if ("log info".equals(p.getName())) {
					int code = p.getField("code").toInt();
					if (code == 0x82) {
						if ("freset".equals(pendingOp)) {
							table.setValueAt(WVDS.strState(2), pendingRow, COL_PARK);
							table.setValueAt("已断开", pendingRow, COL_CONN);
							table.setValueAt("已恢复出厂", pendingRow, COL_STATUS);
							focusMoteRow(pendingRow);
							Mote m = rowToMote(pendingRow);
							m.conned = false;
							pendingOp = null;
							pendingRow = -1;
						}
					}
				}
				else if ("reboot".equals(p.getName())) {
					byte[] mac = p.getField("mac").value;
					byte[] dev = new byte[6];
					System.arraycopy(mac, 2, dev, 0, 6);
					int r = rowOfMote(dev);
					if (r != -1) {
						table.setValueAt("已重启", r, COL_STATUS);
						focusMoteRow(r);
						Mote m = rowToMote(r);
						m.conned = false;
					}
				}
			}
		}
	}

	private void focusMoteRow(int r) {
		Rectangle rect = table.getCellRect(r, 0, true);
		table.scrollRectToVisible(rect);
		this.repaint();
	}

	private void focusMoteRow(Mote m) {
		int r = rowOfMote(m.getDev());
		if (r != -1)
			focusMoteRow(r);
	}

	private void focusAndSelMoteRow(int r) {
		Rectangle rect = table.getCellRect(r, 0, true);
		table.scrollRectToVisible(rect);
		table.getSelectionModel().setSelectionInterval(r, r);
		this.repaint();
	}

	class VDConnCellRenderer extends DefaultTableCellRenderer
	{
		public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column)
		{
			Component c = super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
			if (value instanceof String) {
				if (((String)value).equals("已连接")) {
					setBackground(Color.green);
					setForeground(Color.black);
				}
				else if (((String)value).equals("已断开")) {
					setBackground(new Color(224,224,224));
					setForeground(Color.black);
				}
				else { // 未知
					setBackground(null);
					setForeground(Color.black);
				}
			}
			return c;
		}
	}

	@Override
	public void dataAdded(Object o) {
		if (o instanceof Mote) {
			Mote m = (Mote)o;
			addMoteRow(m);
			focusMoteRow(m);
		}
	}

	@Override
	public void dataRemoved(Object o) {
		if (o instanceof Mote) {
			Mote m = (Mote)o;
			removeMoteRow(m);
		}
	}

	@Override
	public void dataChanged(Object o) {
		// TODO Auto-generated method stub

	}
}
