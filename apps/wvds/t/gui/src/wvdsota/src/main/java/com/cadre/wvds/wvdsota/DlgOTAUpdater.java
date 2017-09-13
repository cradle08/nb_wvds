package com.cadre.wvds.wvdsota;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.Point;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenuBar;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.RowFilter;
import javax.swing.SwingUtilities;
import javax.swing.Timer;
import javax.swing.border.BevelBorder;
import javax.swing.border.TitledBorder;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.filechooser.FileFilter;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

import org.apache.commons.io.FileUtils;
import org.apache.commons.io.LineIterator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.comm.PacketListener;
import net.skaldchen.commons.utils.BytesUtil;
import net.skaldchen.commons.utils.CRC16;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.ExtensionFileFilter;
import net.skaldchen.commons.utils.I18N;
import net.skaldchen.commons.utils.JTableUtil;
import net.skaldchen.commons.utils.MenuUtil;

@SuppressWarnings("serial")
public class DlgOTAUpdater extends JFrame implements PacketListener, MVCView, ActionListener {

	private static Logger log = LoggerFactory.getLogger(DlgOTAUpdater.class);

	private JComboBox<Subnet> netCombo;
	private JComboBox<Integer> chanCombo;
	private JComboBox<Integer> powerCombo;
	private JLabel nodeHintLabel;
	private Timer nodeHintTimer;
	private ActionListener radioListener;
	private JComboBox<String> typeCombo;
	private JComboBox<String> modeCombo;
	private JCheckBox autoCBox;
	private JTextField verTextField;
	private JTextField fileTextField;
	private JProgressBar progressBar;
	private JLabel percentLabel;
	private JTextField filterInput;
	private JButton refrBtn;
	private JButton execButton;

	private final int MAX_FAIL  = Config.getInt("max-fail");
	private final int MAX_NOACK = Config.getInt("max-noack");
	private boolean running = false;
	private boolean restart = false;
	private int failcnt = 0;
	private int waitack = 0;
	private long deadline = 0;
	private int noackn = 0;

	private ArrayList<byte[]> fdata;
	private ArrayList<Segment> segs;
	private int segi;

	private final int COL_NODEMAC  = 0;
	private final int COL_VERSION  = 1;
	private final int COL_PERCENT  = 2;
	private final int COL_PROGRESS = 3;
	private final int COL_REMARK   = 4;

	private final String[] tableH = new String[]{"�豸MAC��ַ", "�汾", "%", "��������", "״̬"};
	private final int[] tableW = new int[]{150,35,25,200,90};
	private DefaultTableModel tableM;
	private JTable table;
	private JScrollPane tableScrollPane;
	private TableRowSorter<TableModel> rowSorter;

	private Subnet curNet = null;

	private String pendingOp = null;
	//private String execAct = null;
	private int execType = 0;
	private int execMode = 0;
	private int execVer = 0;
	private int execTotal = 0;
	private int execCount = 0;
	private int execAuto = 0;
	private byte[] execCommit = null;
	private byte[] execTarget = null;
	private List<byte[]> execTargets = null;
	//private int execFails = 0;
	private Timer timeout;

	private Integer newChan = null;
	private Integer newPower = null;

    // menu item: {name,type,listener,[enabled,mnemonic,accelerator,selected,icon]}
	public Object[][] fileMenu = {
			{ "file", new Character('F') },
			{ "exit", MenuUtil.mi, this, MenuUtil.bT, new Character('x') }
	};
	public Object[][] optionMenu = {
			{ "option", new Character('O') },
			{ "freset", MenuUtil.mi, this, MenuUtil.bT, new Character('F'), null, Boolean.FALSE }
	};
	public Object[][] helpMenu = {
			{ "help", new Character('H') },
			{ "help", MenuUtil.mi, this, MenuUtil.bT, new Character('H') },
			{ null },
			{ "about", MenuUtil.mi, this, MenuUtil.bT, new Character('A') },
	};
	private Object[] menuBarData = { fileMenu, optionMenu, helpMenu};

	public DlgOTAUpdater() {
		setTitle("WVDS OTA����");
		setSize(640,580);
		setResizable(false);
		setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);

		addWindowListener(new WindowAdapter() {
			@Override
			public void windowClosing(WindowEvent e) {
				exit();
			}
		});

		JMenuBar menuBar = MenuUtil.createMenuBar(menuBarData);
		setJMenuBar(menuBar);
		menuBar.setBorder(new BevelBorder(BevelBorder.RAISED));

		JPanel mainPane = new JPanel();
		mainPane.setBorder(new BevelBorder(BevelBorder.RAISED, null, null, null, null));
		getContentPane().add(mainPane, BorderLayout.CENTER);
		mainPane.setLayout(null);

		int x = 20, y = 15;

		JLabel netLabel = new JLabel("Ŀ������");
		mainPane.add(netLabel);
		netLabel.setBounds(x, y+3, 48, 15);

		netCombo = new JComboBox<Subnet>();
		mainPane.add(netCombo);
		netCombo.setBounds(x+60, y, 300, 21);

		SubnetManager mgr = SubnetManager.getInstance();
		mgr.addView(this);
		List<Subnet> nets = mgr.getSubnets();
		for (Subnet net : nets)
			netCombo.addItem(net);

		netCombo.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String type = WVDS.NAME[WVDS.TYPE[typeCombo.getSelectedIndex()]];
				curNet = (Subnet)netCombo.getSelectedItem();
				if (curNet != null) {
					selectChan(curNet.rfchan);
					selectPower(curNet.rfpower);
					verTextField.setText(WVDS.ver2str(curNet.getFwVer(type)));
					fileTextField.setText(curNet.getFwFile(type));
					refrBtn.setEnabled(typeCombo.getSelectedIndex() < 2);
					loadMotes();

					int sel = netCombo.getSelectedIndex();
					Config.set("last-net", String.valueOf(sel), true);
				}
			}
		});

		JButton netAddBtn = new JButton("+");
		mainPane.add(netAddBtn);
		netAddBtn.setBounds(x+364, y, 20, 20);
		netAddBtn.setToolTipText("����µ�����");
		netAddBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				DlgSubnet dlg = new DlgSubnet(Main.gui);
				dlg.setLocationRelativeTo(Main.gui);
				dlg.setVisible(true);
			}
		});

		JButton netDelBtn = new JButton("-");
		mainPane.add(netDelBtn);
		netDelBtn.setBounds(x+388, y, 20, 20);
		netDelBtn.setToolTipText("ɾ����ǰѡ�е�����");
		netDelBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String message = "ȷ��ɾ�������������������?";
				int r = JOptionPane.showConfirmDialog(Main.gui, message, "ȷ��",0,3);
				if (r != JOptionPane.YES_OPTION) {
					return;
				}

				MoteManager.getInstance().removeMote(curNet.id);
				SubnetManager.getInstance().delSubnet(curNet);
			}
		});

		y += 30;
		JLabel chanLabel = new JLabel("�����ŵ�");
		chanLabel.setBounds(x, y+3, 48, 15);
		mainPane.add(chanLabel);

		chanCombo = new JComboBox<Integer>();
		chanCombo.setBounds(x+60, y, 48, 21);
		mainPane.add(chanCombo);
		for (int i = 1; i <= 10; i++)
			chanCombo.addItem(Integer.valueOf(i));

		JLabel powerLabel = new JLabel("��Ƶ����");
		mainPane.add(powerLabel);
		powerLabel.setBounds(x+120, y+3, 48, 15);

		powerCombo = new JComboBox<Integer>();
		mainPane.add(powerCombo);
		powerCombo.setBounds(x+173, y, 48, 21);
		for (int i = 17; i <= 28; i++)
			powerCombo.addItem(Integer.valueOf(i));

		nodeHintLabel = new JLabel();
		mainPane.add(nodeHintLabel);
		nodeHintLabel.setBounds(x+228, y, 300, 18);
		nodeHintLabel.setForeground(Color.RED);

		y += 30;
		JLabel typeLabel = new JLabel("Ŀ������");
		typeLabel.setBounds(x, y+3, 48, 15);
		mainPane.add(typeLabel);

		String[] TYPEs = new String[]{"AP","RP","VD"};
		typeCombo = new JComboBox<String>();
		typeCombo.setBounds(x+60, y, 48, 21);
		mainPane.add(typeCombo);
		for (String type : TYPEs)
			typeCombo.addItem(type);

		typeCombo.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String type = WVDS.NAME[WVDS.TYPE[typeCombo.getSelectedIndex()]];
				verTextField.setText(WVDS.ver2str(curNet.getFwVer(type)));
				fileTextField.setText(curNet.getFwFile(type));
				refrBtn.setEnabled(typeCombo.getSelectedIndex() < 2);
				loadMotes();
			}
		});

		y += 30;
		JLabel modeLabel = new JLabel("����ģʽ");
		modeLabel.setBounds(x, y+3, 54, 15);
		mainPane.add(modeLabel);

		modeCombo = new JComboBox<String>();
		modeCombo.setBounds(x+60, y, 80, 21);
		mainPane.add(modeCombo);
		String[] MODEs = new String[]{"","�����ڵ�","һ������","���нڵ�"};
		for (String mode : MODEs)
			modeCombo.addItem(mode);
		modeCombo.setSelectedIndex(1);
		modeCombo.setEnabled(false);

		autoCBox = new JCheckBox("�Զ�����");
		mainPane.add(autoCBox);
		autoCBox.setBounds(x+145, y+2, 80, 18);

		y += 30;
		JLabel versionLabel = new JLabel("�̼��汾");
		versionLabel.setBounds(x, y+3, 54, 15);
		mainPane.add(versionLabel);

		verTextField = new JTextField();
		verTextField.setBounds(x+60, y, 36, 21);
		mainPane.add(verTextField);
		verTextField.setColumns(10);
		verTextField.setText("1.0");

		JLabel verHintLabel = new JLabel(" ");
		verHintLabel.setBounds(x+100, y+3, 162, 15);
		mainPane.add(verHintLabel);
		verHintLabel.setForeground(Color.RED);

		y += 30;
		JLabel lblNewLabel = new JLabel("�̼��ļ�");
		lblNewLabel.setBounds(x, y+3, 54, 15);
		mainPane.add(lblNewLabel);

		fileTextField = new JTextField();
		fileTextField.setBounds(x+60, y, 490, 21);
		mainPane.add(fileTextField);

		JButton chooseBtn = new JButton("...");
		chooseBtn.setBounds(x+555, y, 25, 20);
		mainPane.add(chooseBtn);

		chooseBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				chooseFirmware();
			}
		});

		y += 30;
		JLabel progLabel = new JLabel("���ؽ���");
		progLabel.setBounds(x, y+3, 54, 15);
		mainPane.add(progLabel);

		progressBar = new JProgressBar();
		progressBar.setBounds(x+60, y, 490, 20);
		mainPane.add(progressBar);
		//progressBar.setVisible(false);

		percentLabel = new JLabel("");
		percentLabel.setBounds(x+555, y, 75, 18);
		mainPane.add(percentLabel);

		y += 30;
		JPanel tableP = new JPanel();
		mainPane.add(tableP);
		tableP.setBounds(x-3, y, 600, 250);
		tableP.setBorder(new TitledBorder("�豸�б�"));
		tableP.setLayout(null);

		tableM = new DefaultTableModel(tableH, 0);
		table = new JTable(tableM);

		tableScrollPane = new JScrollPane();
		tableScrollPane.setViewportView(table);
		tableP.add(tableScrollPane);
		tableScrollPane.setBounds(10, 20, 500, 220);
		JTableUtil.fitTableColumns(table, tableW);

		TableColumn progCol = table.getColumnModel().getColumn(COL_PROGRESS);
		progCol.setCellRenderer(new ProgressRenderer());

		rowSorter = new TableRowSorter<TableModel>(tableM);
		table.setRowSorter(rowSorter);

		int xa = 518, ya = 18, dy = 27;
		JLabel filterLabel = new JLabel("����:");
		tableP.add(filterLabel);
		filterLabel.setBounds(xa, ya+4, 70, 20);

		ya += dy;
		filterInput = new JTextField();
		tableP.add(filterInput);
		filterInput.setBounds(xa, ya, 70, 22);

		filterInput.getDocument().addDocumentListener(new DocumentListener() {
			public void changedUpdate(DocumentEvent e) {
			}

			public void insertUpdate(DocumentEvent e) {
				String text = filterInput.getText().trim();
				rowSorter.setRowFilter(RowFilter.regexFilter(text));
			}

			public void removeUpdate(DocumentEvent e) {
				String text = filterInput.getText().trim();
				rowSorter.setRowFilter(RowFilter.regexFilter(text));
			}
		});

		ya += dy;
		refrBtn = new JButton("ˢ��");
		tableP.add(refrBtn);
		refrBtn.setBounds(xa, ya, 72, 22);
		refrBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				byte[] dest = new byte[8];
				int type = WVDS.TYPE[typeCombo.getSelectedIndex()];
				Main.network.getInfo(dest, type);
			}
		});

		ya += dy;
		JButton addBtn = new JButton("����");
		tableP.add(addBtn);
		addBtn.setBounds(xa, ya, 72, 22);
		addBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String message = "������ڵ�MAC��ַ(�Կո�ָ�):";
				String input = JOptionPane.showInputDialog(DlgOTAUpdater.this, message);
				if (input != null && input.matches("^[0-9a-fA-F ]+$")) {
					String sMac = input.replaceAll(" ", "");
					if (sMac.length() == 16) {
						byte[] mac = new byte[8];
						for (int i = 0; i < 8; i++) {
							mac[i] = (byte)Integer.parseInt(sMac.substring(2*i,2*i+2), 16);
						}

						int type = WVDS.TYPE[typeCombo.getSelectedIndex()];
						if (mac[2] == type) {
							int ver = 0;
							addMote(mac, type, ver, curNet.id);
							addMoteRow(mac, ver, 0, "������");
						}
						else {
							message = "����Ľڵ�MAC��ַ�뵱ǰĿ�����Ͳ���";
							JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
						}
					}
					else {
						message = "����Ľڵ�MAC��ַ�Ƿ�";
						JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
					}
				}
			}
		});

		ya += dy;
		JButton delBtn = new JButton("ɾ��");
		tableP.add(delBtn);
		delBtn.setBounds(xa, ya, 72, 22);
		delBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				int[] rows = table.getSelectedRows();
				if (rows.length == 0) {
					String message = "��ѡ�����е�һ�������豸������";
					JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
					return;
				}

				for (int i = rows.length-1; i >= 0; i--) {
					String sMac = (String)table.getValueAt(rows[i], COL_NODEMAC);
					MoteManager.getInstance().removeMote(sMac);
					tableM.removeRow(rows[i]);
				}
			}
		});

		ya += dy;
		JButton rstBtn = new JButton("����");
		tableP.add(rstBtn);
		rstBtn.setBounds(xa, ya, 72, 22);
		rstBtn.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				int[] rows = table.getSelectedRows();
				if (rows.length > 1) {
					String message = "ֻ��ѡ��һ���豸���в���";
					JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
					return;
				}

				String sMac = (String)table.getValueAt(rows[0], COL_NODEMAC);
				byte[] mac = Mote.toMAC(sMac);
				Main.network.resetMote(mac);
			}
		});

		JPanel botPane = new JPanel();
		getContentPane().add(botPane, BorderLayout.SOUTH);

		JButton downButton = new JButton("����");
		botPane.add(downButton);
		downButton.setPreferredSize(new Dimension(50,24));
		downButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String sVer = verTextField.getText();
				if (sVer == null || !sVer.matches("^[0-9]\\.[0-9]$")) {
					verHintLabel.setText("�������[0-9].[0-9]����1.0");
					return;
				}

				String execFile = fileTextField.getText();
				execFile = execFile.substring(execFile.lastIndexOf(File.separatorChar) + 1);
				Pattern p = Pattern.compile(Config.get("fname-patt"));
				Matcher m = p.matcher(execFile);
				if (!m.find()) {
					String message = "�̼��ļ��������Ϲ淶";
					JOptionPane.showMessageDialog(Main.gui, message);
					return;
				}

				String firmware = fileTextField.getText();
				File f = new File(firmware);
				if (f != null && f.exists()) {
					int type = WVDS.TYPE[typeCombo.getSelectedIndex()];
					int ver = Integer.parseInt(verTextField.getText().replaceAll("\\.", ""), 16);

					curNet.setFwVer(WVDS.NAME[type], ver);
					curNet.setFwFile(WVDS.NAME[type], fileTextField.getText());
					SubnetManager.getInstance().updSubnet(curNet);

					progressBar.setVisible(true);
					downloadFirmware(f, type, ver);
				}
			}
		});

		execButton = new JButton("����");
		botPane.add(execButton);
		execButton.setActionCommand("exec");
		execButton.setPreferredSize(new Dimension(50,24));
		execButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				String act = e.getActionCommand();
				if ("exec".equals(act)) {
					String ver = verTextField.getText();
					if (ver == null || !ver.matches("^[0-9]\\.[0-9]$")) {
						verHintLabel.setText("�������[0-9].[0-9]����1.0");
						return;
					}

					int[] rows = table.getSelectedRows();
					if ((rows.length != 1) && !autoCBox.isSelected()) {
						String message = "��ѡ���豸�б��е�һ�л�ѡ���Զ����¸�ѡ���������";
						JOptionPane.showMessageDialog(Main.gui, message);
						return;
					}

					String execFile = fileTextField.getText();
					execFile = execFile.substring(execFile.lastIndexOf(File.separatorChar) + 1);
					Pattern p = Pattern.compile(Config.get("fname-patt"));
					Matcher m = p.matcher(execFile);
					if (m.find()) {
						String commit = m.group(Config.getInt("group-commit"));
						execCommit = new byte[8];
						for (int i = 0; i < commit.length(); i++)
							execCommit[i] = (byte)commit.charAt(i);
					} else {
						String message = "�̼��ļ��������Ϲ淶���޷�ȷ����git�汾��";
						JOptionPane.showMessageDialog(Main.gui, message);
						return;
					}

					execType = WVDS.TYPE[typeCombo.getSelectedIndex()];
					execMode = modeCombo.getSelectedIndex();
					execVer = Integer.parseInt(verTextField.getText().replaceAll("\\.", ""), 16);
					execTotal = curNet.getFwSize(WVDS.NAME[execType]);
					execCount = 1;
					execAuto = (autoCBox.isSelected() ? 1 : 0);

					if (execTargets == null) {
						execTargets = new ArrayList<byte[]>();
					}
					execTargets.clear();
					if (autoCBox.isSelected()) {
						byte[] target = new byte[8];
						target[2] = (byte)execType;
						execTargets.add(target);

					} else {
						for (int r : rows) {
							String str = (String) table.getValueAt(r, COL_NODEMAC);
							byte[] target = Mote.toMAC(str);
							execTargets.add(target);
							if (!"������".equals(table.getValueAt(r, COL_REMARK)))
								table.setValueAt("�ȴ�����", r, COL_REMARK);
						}
					}

					log.info(String.format("start upgrade %s nodes", execTargets.size()));
					executeUpgrade();
				}
				else if ("stop".equals(act)) {
					byte[] target = new byte[8];
					byte[] commit = new byte[8];
					Main.network.execOTA(0, 0, target, 0, 0, 0, commit);
				}
			}
		});

		JButton closeButton = new JButton("�˳�");
		botPane.add(closeButton);
		closeButton.setPreferredSize(new Dimension(50,24));
		closeButton.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				exit();
			}
		});

		netCombo.setSelectedIndex(Config.getInt("last-net"));
		typeCombo.setSelectedIndex(2);
		chanCombo.setSelectedItem(curNet.rfchan);
		powerCombo.setSelectedItem(curNet.rfpower);

		radioListener = new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				newPower = (Integer)powerCombo.getSelectedItem();
				newChan = (Integer)chanCombo.getSelectedItem();
				Main.network.setRadio(newChan.intValue(), newPower.intValue());
			}
		};
		chanCombo.addActionListener(radioListener);
		powerCombo.addActionListener(radioListener);
	}

	private void exit() {
		boolean quit = true;
		if (Config.getBool("quit-confirm")) {
			String message = "ȷ���˳�������?";
			int r = JOptionPane.showConfirmDialog(this, message, "ȷ��",0,3);
			quit = (r == JOptionPane.YES_OPTION);
		}
		if (quit) {
			Point p = this.getLocation();
			Config.set("frame-x", String.valueOf(p.x));
			Config.set("frame-y", String.valueOf(p.y));
			Config.save();
			FileUtils.deleteQuietly(new File(Main.pidfile));
			dispose();
			System.exit(0);
		}
	}

	private int rowIndex(byte[] mac) {
		String sMac = Mote.strMAC(mac);
		for (int i = 0; i < table.getRowCount(); i++) {
			if (sMac.equals((String)table.getValueAt(i, COL_NODEMAC))) {
				return i;
			}
		}
		return -1;
	}

	private void executeUpgrade() {
		if (execTargets.size() == 0)
			return;

		execTarget = execTargets.get(0);
		int r = rowIndex(execTarget);
		if (r != -1) {
			table.setValueAt(Integer.valueOf(0), r, COL_PERCENT);
			table.setValueAt(Integer.valueOf(0), r, COL_PROGRESS);
		}
		log.info(String.format("exec OTA to %s", Mote.strMAC(execTarget)));
		Main.network.execOTA(execType, execVer, execTarget, execMode, execCount, execAuto, execCommit);

		//int period = Config.getInt("cmd-timeout");
		//timeout = new Timer(period*1000, new ActionListener() {
		//	@Override
		//	public void actionPerformed(ActionEvent e) {
		//		if (++execFails < Config.getInt("cmd-maxfail")) {
		//			log.debug(String.format("resend exec command to %s", Mote.strMAC(execTarget)));
		//			Main.network.execOTA(execType, execVer, execTarget, execMode, execCount, execAuto, execCommit);
		//		}
		//		else {
		//			log.warn(String.format("fail exec OTA to %s", Mote.strMAC(execTarget)));
		//			timeout.stop();
		//			timeout = null;

		//			int r = rowIndex(execTarget);
		//			if (r != -1) {
		//				table.setValueAt("����ʧ��", r, COL_REMARK);
		//			}

		//			execTargets.remove(execTarget);
		//			if (execTargets.size() > 0) {
		//				log.debug("exec OTA to next node at prev max failure");
		//				execFails = 0;
		//				executeUpgrade();
		//			}
		//		}
		//	}
		//});
		//timeout.start(); log.debug("start timeout " + timeout);
		//log.debug(String.format("start timeout for upgrade %s", Mote.strMAC(execTarget)));
	}

	private void chooseFirmware() {
		File file1 = new File(Config.get("last-dir"));
		JFileChooser chooser = new JFileChooser(file1);
		//FileFilter filter1 = new ExtensionFileFilter("Firmware file (*.hex,*.txt)", new String[] { "HEX", "TXT" });
		FileFilter filter1 = new ExtensionFileFilter("msp430-txt file (*.txt)", new String[] { "TXT" });
		chooser.setFileFilter(filter1);
		int returnVal = chooser.showOpenDialog(null);
		if (returnVal == JFileChooser.APPROVE_OPTION) {
			String fname = chooser.getSelectedFile().getPath();
			File file = new File(fname);

			if (fname.contains("_boot")) {
				String message = "Ӧѡ���ļ�������_boot�Ĺ̼��ļ�";
				JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
				return;
			}

			String type = (String)typeCombo.getSelectedItem();
			if (!file.getName().startsWith(type)) {
				String message = "ѡ��Ĺ̼��ļ���Ŀ�����Ͳ���";
				JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
				return;
			}

			fileTextField.setText(fname);
			Config.set("last-dir", file.getParent(), true);
		}
	}

	private void downloadFirmware(File firmware, int role, int ver) {
		final int LEN = 128; // ÿ����֡����128�ֽ���Ч�ĳ�������

		String fname = firmware.getName();
		fdata = new ArrayList<byte[]>();

		segs = new ArrayList<Segment>();
		segs.add(new Segment(0x5C00, 0xEE00));
		segs.add(new Segment(0xEE00, 0xF000));
		segs.add(new Segment(0x10000, 0x26C00));
		segi = 0;

		if (fname.endsWith(".hex")) {
			try {
				LineIterator it = FileUtils.lineIterator(firmware);
				int len = 0;
				int ofs = 0;
				int addr = 0;
				int type = 0;
				int num = 0;
				byte[] pkt = null;
				int i = 0;
				int expt = segs.get(segi).beg;

				while (it.hasNext()) {
					String line = it.next(); // ���.hex��ÿ�д���
					len = Integer.parseInt(line.substring(1,3), 16); // ���������ֽڸ���
					addr = Integer.parseInt(line.substring(3,7), 16) + ofs; // �������ݶ�Ӧ��ʼ��ַ��ע��Ҫ����ƫ�Ƶ�ַ
					type = Integer.parseInt(line.substring(7,9), 16); // ���е�����

					if (type == 0x00) { // ������
						if (expt < addr) { // Ԥ�ڵĵ�ַС�ڵ�ǰ�е���ʼ��ַ���м䲿����Ҫ���0xFF�ֽ�
							log.debug(String.format("need fill [0x%04X, 0x%04X)", expt, addr));
							for ( ; expt < addr; ) {
								if (i == 0) { // ��ʼһ��������֡
									pkt = new byte[18+LEN];
									i += 6; // tstamp
									pkt[i++] = (byte)role; // Ŀ����½ڵ�����: VD/RP/AP
									pkt[i++] = (byte)ver; // �̼��汾��
									pkt[i++] = (byte)(num == 0 ? 0x01 : 0x00); // ����ǵ�1�����ݰ�����ֵ0x01���м����ݰ�����ֵ0x00�����һ�����ݰ�����ֵ0x80
									pkt[i++] = 0x00; // ����(������)
									BytesUtil.putBEUInt32(expt, pkt, i); i += 4; // 4�ֽڵ�ַ���ɼ������ݰ����к�
									BytesUtil.putBEUInt16(LEN, pkt, i); i += 2; // 2�ֽ����ݳ���
								}

								pkt[i++] = (byte)0xFF; // ���0xFF�ֽ�
								expt += 1;

								if (i == 10+LEN) { // ��ǰ����֡������
									int crc = CRC16.calc(0, pkt, 0, i); // ����ӽڵ����͵���Ч���ݵ�CRC
									BytesUtil.putBEUInt16(crc, pkt, i); i += 2; // ���CRC
									//log.debug(String.format("enq %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
									fdata.add(pkt); // ���뷢�Ͷ��У��Ժ���
									num += 1;
									i = 0; // ��Ҫ��ʼһ��������֡
								}
							}
						}

						for (int j = 0; j < len; j++) { // ������ǰ������
							if (i == 0) { // ��ʼһ��������֡
								pkt = new byte[18+LEN];
								i += 6; // tstamp
								pkt[i++] = (byte)role;  // Ŀ����½ڵ�����: VD/RP/AP
								pkt[i++] = (byte)ver;  // �̼��汾��
								pkt[i++] = (byte)(num == 0 ? 0x01 : 0x00); // ����ǵ�1�����ݰ�����ֵ0x01���м����ݰ�����ֵ0x00�����һ�����ݰ�����ֵ0x80
								pkt[i++] = 0x00; // ����(������)
								BytesUtil.putBEUInt32(expt, pkt, i); i += 4; // 4�ֽڵ�ַ���ɼ������ݰ����к�
								BytesUtil.putBEUInt16(LEN, pkt, i); i += 2; // 2�ֽ����ݳ���
							}

							pkt[i++] = (byte)Integer.parseInt(line.substring(9+j*2,9+j*2+2), 16); // hex�ļ������ֽ�
							expt += 1;

							if (i == 16+LEN) { // ��ǰ����֡������
								int crc = CRC16.calc(0, pkt, 6, i-6); // ����ӽڵ����͵���Ч���ݵ�CRC
								BytesUtil.putBEUInt16(crc, pkt, i); i += 2; // ���CRC
								//log.debug(String.format("enq %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
								fdata.add(pkt); // ���뷢�Ͷ��У��Ժ���
								num += 1;
								i = 0; // ��Ҫ��ʼһ��������֡
							}
						}
					}
					else if (type == 0x02) { // ��ַ������
						ofs = Integer.parseInt(line.substring(9, 13), 16) * 16; // ��ַƫ��ֵ����
					}
				}

				if (i > 0) { // �ļ��Ѷ��꣬�����һ������֡������
					pkt[8] = (byte)0x80; // ��������һ������֡
					for (; i < 16+LEN; )
						pkt[i++] = (byte)0xFF; // ���0xFF�ֽ�
					int crc = CRC16.calc(0, pkt, 6, i-6);
					BytesUtil.putBEUInt16(crc, pkt, i); i += 2;
					//log.debug(String.format("enq %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
					fdata.add(pkt); // ���뷢�Ͷ��У��Ժ���
					num += 1;
					i = 0;
				} else { // ���һ�����ݰ���������
					pkt = fdata.get(fdata.size()-1);
					pkt[8] = (byte)0x80; // ��������һ������֡
					int crc = CRC16.calc(0, pkt, 0, 16+LEN); // ���¼���CRC
					BytesUtil.putBEUInt16(crc, pkt, 16+LEN);
				}

			} catch (IOException e) {
				log.warn("fail load " + firmware.getAbsolutePath());
			}
		}
		else if (fname.endsWith(".txt")) {
			try {
				LineIterator it = FileUtils.lineIterator(firmware);
				int addr = 0;
				int expt = segs.get(segi).beg;
				int segaddr = 0;
				byte[] pkt = null;
				int num = 0;
				int i = 0;

				while (it.hasNext()) {
					String line = it.next();
					if (line.charAt(0) == '@') {
						addr = Integer.parseInt(line.substring(1), 16);
					}
					else if (line.charAt(0) == 'q') {
						// file end
					}
					else {
						while (expt < addr) {
							int fill = 0;
							if (addr > segs.get(segi).end) {
								fill = segaddr + LEN;
							} else {
								fill = addr;
							}
							log.debug(String.format("need fill [0x%04X, 0x%04X)", expt, fill));
							for ( ; expt < fill; ) {
								if (i == 0) { // ��ʼһ��������֡
									pkt = new byte[18+LEN];
									i += 6; // ʱ���������ʱ���
									pkt[i++] = (byte)role; // Ŀ����½ڵ�����: VD/RP/AP
									pkt[i++] = (byte)ver; // �̼��汾��
									pkt[i++] = (byte)(num == 0 ? 0x01 : 0x00); // ����ǵ�1�����ݰ�����ֵ0x01���м����ݰ�����ֵ0x00�����һ�����ݰ�����ֵ0x80
									pkt[i++] = 0x00; // ����(������)
									BytesUtil.putBEUInt32(expt, pkt, i); i += 4; // 4�ֽڵ�ַ���ɼ������ݰ����к�
									BytesUtil.putBEUInt16(LEN, pkt, i); i += 2; // 2�ֽ����ݳ���
									segaddr = expt;
								}

								pkt[i++] = (byte)0xFF; // ���0xFF�ֽ�
								expt += 1;

								if (i == 16+LEN) { // ��ǰ����֡������
									int crc = CRC16.calc(0, pkt, 6, i-6); // ����ӽڵ����͵���Ч���ݵ�CRC
									BytesUtil.putBEUInt16(crc, pkt, i); i += 2; // ���CRC
									//log.debug(String.format("enq %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
									fdata.add(pkt); // ���뷢�Ͷ��У��Ժ���
									num += 1;
									i = 0; // ��Ҫ��ʼһ��������֡
								}
							}
							if (addr > segs.get(segi).end) {
								segi += 1;
								expt = segs.get(segi).beg;
								log.debug(String.format("expect %X of seg %d start after fill", expt, segi));
							}
						}

						String[] subs = line.split(" ");
						byte[] data = new byte[subs.length];
						for (int j = 0; j < data.length; j++) {
							data[j] = (byte)Integer.parseInt(subs[j], 16);
						}

						for (int j = 0; j < data.length; j++) { // ������ǰ������
							if (i == 0) { // ��ʼһ��������֡
								pkt = new byte[18+LEN];
								i += 6; // ʱ���������ʱ���
								pkt[i++] = (byte)role;  // Ŀ����½ڵ�����: VD/RP/AP
								pkt[i++] = (byte)ver;  // �̼��汾��
								pkt[i++] = (byte)(num == 0 ? 0x01 : 0x00); // ����ǵ�1�����ݰ�����ֵ0x01���м����ݰ�����ֵ0x00�����һ�����ݰ�����ֵ0x80
								pkt[i++] = 0x00; // ����(������)
								BytesUtil.putBEUInt32(expt, pkt, i); i += 4; // 4�ֽڵ�ַ���ɼ������ݰ����к�
								BytesUtil.putBEUInt16(LEN, pkt, i); i += 2; // 2�ֽ����ݳ���
								segaddr = expt;
							}

							pkt[i++] = data[j]; // txt�ļ������ֽ�
							expt += 1;
							if (expt >= segs.get(segi).end) {
								segi += 1;
								expt = segs.get(segi).beg;
								log.debug(String.format("expect %X of seg %d start", expt, segi));
							}

							if (i == 16+LEN) { // ��ǰ����֡������
								int crc = CRC16.calc(0, pkt, 6, i-6); // ����ӽڵ����͵���Ч���ݵ�CRC
								BytesUtil.putBEUInt16(crc, pkt, i); i += 2; // ���CRC
								//log.debug(String.format("enq %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
								fdata.add(pkt); // ���뷢�Ͷ��У��Ժ���
								num += 1;
								i = 0; // ��Ҫ��ʼһ��������֡
							}
						}

					}
				}

				if (i > 0) { // �ļ��Ѷ��꣬�����һ������֡������
					pkt[8] = (byte)0x80; // ��������һ������֡
					for (; i < 16+LEN; )
						pkt[i++] = (byte)0xFF; // ���0xFF�ֽ�
					int crc = CRC16.calc(0, pkt, 6, i-6);
					BytesUtil.putBEUInt16(crc, pkt, i); i += 2;
					//log.debug(String.format("enq %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
					fdata.add(pkt); // ���뷢�Ͷ��У��Ժ���
				} else { // ���һ�����ݰ���������
					pkt = fdata.get(fdata.size()-1);
					pkt[8] = (byte)0x80; // ��������һ������֡
					int crc = CRC16.calc(0, pkt, 6, 10+LEN); // ���¼���CRC
					BytesUtil.putBEUInt16(crc, pkt, 16+LEN);

					pkt= fdata.get(fdata.size()-1);
					log.debug(String.format("last %d: %s", pkt.length, BytesUtil.strBytes(pkt)));
				}
			} catch(Exception e) {
				log.warn("fail", e);
			}
		}
		else {
			log.warn("not support this format of firmware");
		}

		int npage = (fdata.size() + 1) / 2;
		curNet.setFwSize(WVDS.NAME[role], npage);
		SubnetManager.getInstance().updSubnet(curNet);

		Runnable r = new Runnable() {
			@Override
			public void run() {
				while (running) {
					//Mote BS = MainClass.motedb.getMote(Mote.ROOT);
					int total = fdata.size();
					int done = 0;
					byte[] mac = new byte[8];

					log.info("ota download start");
					while (running && done < fdata.size()) {
						byte[] pkt = fdata.get(done);
						System.arraycopy(WVDS.getTS(), 0, pkt, 0, 6); // attach timestamp

						waitack = BytesUtil.getBEUInt32(pkt, 10);
						log.info(String.format("send ota data at 0x%06X", waitack));
						log.info(String.format("send: (%d) %s", pkt.length, BytesUtil.strBytes(pkt)));
						Main.network.send(mac, (WVDS.PKT_OTADATA | 0x80), pkt);
						//Main.com.send(pkt);

						deadline = System.currentTimeMillis() + 1000;
						while (waitack != 0 && System.currentTimeMillis() < deadline);

						if (waitack == 0) {
							log.debug(String.format("next ota data"));
							//fdata.remove(0);
							done += 1;
							progressBar.setValue((done * 100) / total);
							percentLabel.setText(String.format("%d/%d", done, total));
							noackn = 0;
						} else {
							log.warn(String.format("noack for ota data 0x%06X", waitack));
							if (++noackn >= MAX_NOACK) {
								running = false;
								log.warn(String.format("ota data %d/0x%06X reach max noack", done, waitack));
								String message = String.format("��%d����ַ0x%06X�����ݶ���ش�ʧ�ܣ���ֹ����", done, waitack);
								JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
							}
							if (restart) {
								done = 0;
								restart = false;
							}
						}
					}
					if (done == fdata.size()) {
						log.info("ota download complete");
						running = false;
						String message = "�������";
						JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
					}
				}
				log.info("ota download quit");
			}
		};
		running = true;
		new Thread(r).start();
	}

	private void loadMotes() {
		if (tableM != null) {
			int type = WVDS.TYPE[typeCombo.getSelectedIndex()];
			List<Mote> motes = MoteManager.getInstance().getMotes(curNet.id, type);
			tableM.setRowCount(0);
			for (Mote m : motes)
				addMoteRow(m);
		}
	}

	private void addMoteRow(Mote m) {
		addMoteRow(m.getMac(), m.getVer(), 0, "������");
	}

	private void addMoteRow(String mac, String ver, int prog, String status) {
		Object[] rowData = new Object[tableH.length];
		int i = 0;
		rowData[i++] = mac;
		rowData[i++] = ver;
		rowData[i++] = Integer.valueOf(prog);
		rowData[i++] = Integer.valueOf(prog);
		rowData[i++] = status;
		tableM.addRow(rowData);
	}

	private void addMoteRow(byte[] mac, int ver, int prog, String status) {
		addMoteRow(Mote.strMAC(mac), WVDS.strVer(ver), prog, status);
	}

	private void selectChan(int chan) {
		chanCombo.removeActionListener(radioListener);
		chanCombo.setSelectedItem(chan);
		chanCombo.addActionListener(radioListener);
	}

	private void selectPower(int power) {
		powerCombo.removeActionListener(radioListener);
		powerCombo.setSelectedItem(power);
		powerCombo.addActionListener(radioListener);
	}

	private void showHint(String message) {
		nodeHintLabel.setText(message);
		if (nodeHintTimer == null) {
			int time = Config.getInt("hint-time");
			nodeHintTimer = new Timer(time, new ActionListener() {
				@Override
				public void actionPerformed(ActionEvent e) {
					nodeHintLabel.setText("");
				}
			});
		}
		nodeHintTimer.restart();
	}

	private Mote addMote(byte[] mac, int type, int ver, int net) {
		Mote m = new Mote();
		MoteManager mgr = MoteManager.getInstance();
		m.setMac(mac);
		m.setType(type);
		m.setVer(ver);
		m.setNetId(net);
		mgr.addMote(m);
		return m;
	}

	private Mote getMote(byte[] mac) {
		MoteManager mgr = MoteManager.getInstance();
		return mgr.getMote(mac);
	}

	private Mote getOrAddMote(byte[] mac, int type, int ver, int net) {
		Mote m = null;
		m = getMote(mac);
		if (m == null) {
			m = addMote(mac, type, ver, net);
		}
		return m;
	}

	@Override
	public void packetReceived(byte[] packet) {
		if (packet == null || packet.length < 2)
			return;

		int last = packet.length - 1;
		if ((packet[last-1] == 0x0D) && (packet[last] == 0x0A)) {
			String str = BytesUtil.txtBytes(packet, 0, packet.length);
			log.info(String.format("rcvd: %s", str));

			if ("freset".equals(pendingOp)) {
				pendingOp = null;
				if (str.startsWith("AT+OK")) {
					String message = "�����ڵ�ָ����������ѳɹ�";
					JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
				}
			}
			return;
		}

		if ((packet[0] != (byte)0xAA) || (packet[last] != (byte)0xFF)) {
			log.warn("invalid WVDS frame: " + BytesUtil.strBytes(packet));
			return;
		}

		byte[] payload = null;
		try {
			payload = WVDS.decrypt(packet, WVDS.aesKey, WVDS.aesIV);
		} catch (Exception e) {
			log.warn("invalid WVDS frame: " + BytesUtil.strBytes(packet), e);
			return;
		}

		try {
			int op = (payload[6] & WVDS.OP_MASK);
			log.info(String.format("rcvd: (%d) %s", payload.length, BytesUtil.strBytes(payload)));

			if (op == WVDS.PKT_OTADATA) {
				int res = BytesUtil.getUInt8(payload, 13);
				int addr = BytesUtil.getBEUInt32(payload, 14);
				log.info(String.format("rcvd otadata resp %d at 0x%05X", res, addr));

				if (res == 0) {
					if (addr == waitack) {
						log.info(String.format("ota data 0x%06X acked", waitack));
						waitack = 0;
					} else {
						log.warn(String.format("ota data ack mismatch, rcvd 0x%06X != wait 0x%06X", addr, waitack));
					}
				}
				else if (res == 1) { // ���ݰ�CRC����
					int rcrc = BytesUtil.getBEUInt16(payload, 18);
					int ccrc = BytesUtil.getBEUInt16(payload, 20);
					log.warn(String.format("crc error, rcrc 0x%04X, ccrc 0x%04X", rcrc, ccrc));
				}
				else if (res == 2) { // �޷����ļ�д
					if (++failcnt >= MAX_FAIL) {
						running = false; // no need download
						SwingUtilities.invokeLater(new Runnable() {
							@Override
							public void run() {
								String message = String.format("�޷����ļ�д��");
								JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
							}
						});
					}
				}
				else if (res == 3) { // �ļ�δ��
					restart = true;
					log.warn("file not open to write");
				}
				else if (res == 4) { // ���ݰ�д��ʧ��
					if (++failcnt >= MAX_FAIL) {
						running = false; // no need download
						SwingUtilities.invokeLater(new Runnable() {
							@Override
							public void run() {
								String message = String.format("��ַ0x%05X�����ݶ��д��ʧ��", addr);
								JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
							}
						});
					}
				}
				else if (res == 5) { // �ظ����ݰ�
					if (addr == waitack) {
						waitack = 0;
					}
				}
				else if (res == 6) { // ���ذ汾�Ź���
					int ver = BytesUtil.getUInt8(payload, 18);
					running = false; // no need download
					SwingUtilities.invokeLater(new Runnable() {
						@Override
						public void run() {
							String message = String.format("�����ع̼��汾��%s����ȷ�������ذ汾", WVDS.strVer(ver));
							JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
						}
					});
				}
				else if (res == 7) { // ���ذ汾����ͬ����������
					int ver = BytesUtil.getUInt8(payload, 18);
					running = false; // no need download
					SwingUtilities.invokeLater(new Runnable() {
						@Override
						public void run() {
							String message = String.format("�����ع̼��汾��%s����������", WVDS.strVer(ver));;
							JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
						}
					});
				}
				else {
					log.warn(String.format("ota data 0x%06X acked error %d", waitack, res));
				}
			}
			else if (op == WVDS.PKT_OTAEXEC) {
				int res = BytesUtil.getUInt8(payload, 13);
				int ver = ((payload.length > 14) ?  BytesUtil.getUInt8(payload, 14) : 0);
				log.info(String.format("rcvd otaexec resp %d for ver %s", res, WVDS.strVer(ver)));

				String message = null;
				switch (res) {
				case 0:
					if ("exec".equals(execButton.getActionCommand())) {
						if (autoCBox.isSelected()) { showHint(String.format("�Զ��������������ȴ�VD����")); }
						else { showHint(String.format("��%s�ĸ���������", Mote.strMAC(execTarget, ""))); }

						execButton.setActionCommand("stop");
						execButton.setText("��ֹ");

						if (execTarget != null) {
							int r = rowIndex(execTarget);
							if (r != -1) {
								table.setValueAt("����������", r, COL_REMARK);
							}
						}
					}
					else if ("stop".equals(execButton.getActionCommand())) {
						showHint("�����ڵ��ѳɹ���ֹ����");

						execButton.setActionCommand("exec");
						execButton.setText("����");
					}
					break;
				case 1:
					message = "�ڵ�û������֧��"; break;
				case 2:
					message = String.format("�����ع̼��汾�Ƚڵ㵱ǰ�汾%s��", WVDS.strVer(ver)); break;
				case 3:
					message = String.format("�°汾�̼���δ���أ������ذ汾��%s", WVDS.strVer(ver)); break;
				case 4:
					message = String.format("�����汾���������ع̼��汾%s����", WVDS.strVer(ver)); break;
				case 5:
					message = String.format("�����ڸ��豸��״̬δ֪����ˢ�»򼤻�VD");
					if ("exec".equals(execButton.getActionCommand())) {
						execButton.setActionCommand("stop");
						execButton.setText("��ֹ");
					}
					break;
				case 6:
					message = String.format("��֧�ָø���ģʽ"); break;
				}
				if (message != null)
					JOptionPane.showMessageDialog(DlgOTAUpdater.this, message);
			}
			else if (op == WVDS.PKT_OTAPROG) { // OTA progress
				byte[] mac = BytesUtil.getBytes(payload, 7, 8);
				int ver = BytesUtil.getUInt8(payload, 15);
				int done = BytesUtil.getBEUInt16(payload, 16);
				int perc = (done * 100) / (execTotal - 1);

				if (done == 0xFFFF) {
					log.info(String.format("rebooted at update %s/%s not done", WVDS.strMac(mac), WVDS.strVer(ver)));
					showHint("�ϴθ���δ��ɣ��Զ�����");

					if ("exec".equals(execButton.getActionCommand())) {
						execButton.setActionCommand("stop");
						execButton.setText("��ֹ");

						Subnet net = (Subnet) netCombo.getSelectedItem();
						String devtype = WVDS.NAME[mac[2]];
						typeCombo.setSelectedItem(devtype);

						if (ver == net.getFwVer(devtype)) {
							verTextField.setText(WVDS.ver2str(ver));
							fileTextField.setText(net.getFwFile(devtype));
							execTotal = net.getFwSize(devtype);
						} else {
							log.warn(String.format("not identical with database"));
						}

						if (mac[3]==0 && mac[4]==0) {
							autoCBox.setSelected(true);
						}
					}
				}
				else {
					log.info(String.format("rcvd ota progress %d/%d, %d%%", done, execTotal, perc));
					if (timeout != null) {
						timeout.stop(); log.debug("stop timeout " + timeout);
						timeout = null;
					}

					Mote m = getMote(mac);
					if (m == null) {
						m = addMote(mac, mac[2], ver-1, curNet.id);
					}
					m.otaPending = true;

					int r = rowIndex(mac);
					if (r == -1) {
						addMoteRow(m);
						r = rowIndex(mac);
					}
					if (r != -1) {
						table.setValueAt(Integer.valueOf(perc), r, COL_PERCENT);
						table.setValueAt(Integer.valueOf(perc), r, COL_PROGRESS);
						table.setValueAt(((perc >= 100) ? "�̼��������" : "���ڽ��չ̼�"), r, COL_REMARK);
					}
				}
			}
			else if (op == WVDS.PKT_VDCONN) {
				byte[] mac = BytesUtil.getBytes(payload, 7, 8);
				int ver = BytesUtil.getUInt8(payload, 15);
				//int nid = BytesUtil.getBEUInt16(payload, 16);
				String sMac = Mote.strMAC(mac);
				String sVer = WVDS.strVer(ver);
				log.info(String.format("rcvd version %s from %s", sVer, sMac));

				Mote m = getOrAddMote(mac, mac[2], ver, curNet.id);

				if (!m.otaPending) {
					int type = WVDS.TYPE[typeCombo.getSelectedIndex()];
					if (mac[2] == (byte)type) {
						boolean exist = false;
						for (int i = 0; i < table.getRowCount(); i++) {
							if (sMac.equals((String)table.getValueAt(i, COL_NODEMAC))) {
								if (!sVer.equals((String)table.getValueAt(i, COL_VERSION))) {
									table.setValueAt(sVer, i, COL_VERSION);
								}
								table.setValueAt("����", i, COL_REMARK);
								exist = true; break;
							}
						}
						if (!exist) {
							log.info(String.format("add new node %s, ver %s", sMac, sVer));
							addMoteRow(sMac, sVer, 0, "����");
						}
					}
				}

				if (ver != m.getVer()) {
					m.setVer(ver);
					MoteManager.getInstance().updateMote(m);
				}

				if (ver == execVer) {
					int r = rowIndex(mac);
					if (r != -1) {
						table.setValueAt(sVer, r, COL_VERSION);
						table.setValueAt("�����ɹ�", r, COL_REMARK);
						m.otaPending = false;

						Iterator<byte[]> it = execTargets.iterator();
						while (it.hasNext()) {
							byte[] tmp = it.next();
							if (Arrays.equals(tmp, mac)) {
								it.remove();
								log.info(String.format("node %s update succeed", Mote.strMAC(mac)));
							}
						}
						//if (execTargets.size() > 0) {
						//	log.debug("exec OTA to next node at prev succeed");
						//	execFails = 0;
						//	executeUpgrade();
						//}

						if (execMode == WVDS.OTA_ONENODE && !autoCBox.isSelected()) {
							execButton.setActionCommand("exec");
							execButton.setText("����");
						}
					}
				}
			}
			else if (op == WVDS.PKT_RADIO) {
				int subop = BytesUtil.getUInt8(payload, 13);
				int res = BytesUtil.getUInt8(payload, 14);
				if (subop == WVDS.GET) {
					if (res == 0) {
						int chan = BytesUtil.getUInt8(payload, 15);
						int power = BytesUtil.getUInt8(payload, 20);
						log.info(String.format("rcvd radio channel %d power %d", chan, power));
						showHint("�����ڵ�������");
						selectChan(chan);
						selectPower(power);

						curNet.rfchan = chan;
						curNet.rfpower = power;
						SubnetManager.getInstance().updSubnet(curNet);
					}
				}
				else if (subop == WVDS.SET) {
					if (res == 0) {
						log.info(String.format("set radio config succeed"));
						showHint("�����ڵ�Ĺ����ŵ�����Ƶ���ʸ��ĳɹ�");
						curNet.rfchan = newChan;
						curNet.rfpower = newPower;
						SubnetManager.getInstance().updSubnet(curNet);
						//JOptionPane.showMessageDialog(Main.gui, "�ŵ����óɹ�");
					} else {
						log.warn(String.format("set radio config failed"));
						selectChan(curNet.rfchan);
						selectPower(curNet.rfpower);
						JOptionPane.showMessageDialog(Main.gui, "�ŵ�����ʧ��");
					}
				}
			}
			else {
				log.warn(String.format("ignore WVDS message 0x%02X", op));
			}
		} catch(Exception e) {
			log.warn("", e);
		}
	}

	@Override
	public void dataAdded(Object o) {
		if (o instanceof Subnet) {
			netCombo.addItem((Subnet)o);
		}
	}

	@Override
	public void dataRemoved(Object o) {
		if (o instanceof Subnet) {
			netCombo.removeItem((Subnet)o);
		}
	}

	@Override
	public void dataChanged(Object o) {
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		String act = e.getActionCommand();

		if ("menu.exit".equals(act)) {
			exit();
		}
		else if ("menu.freset".equals(act)) {
			pendingOp = "freset";
			Main.network.freset();
		}
		else if ("menu.help".equals(act)) {
			try {
				String cmd = "cmd /c start %s";
				cmd = String.format(cmd, Config.get("help-file"));
				Runtime.getRuntime().exec(cmd);
			} catch(Exception ex) {
				ex.printStackTrace();
			}
		}
		else if ("menu.about".equals(act)) {
			String message = I18N.get("text.about");
			message += I18N.get("text.version")
					.replaceAll("<version>", Version.version.toString())
					.replaceAll("<build>", Version.build)
					.replaceAll("<commit>", Version.commit);
			JOptionPane.showMessageDialog(DlgOTAUpdater.this, message, "����",1);
		}
		else {
			System.err.println("not support " + act);
		}
	}
}

@SuppressWarnings("serial")
class ProgressRenderer extends DefaultTableCellRenderer {

	private final JProgressBar b = new JProgressBar(0, 100);

	public ProgressRenderer() {
		super();
		setOpaque(true);
		b.setBorder(BorderFactory.createEmptyBorder(1, 1, 1, 1));
	}

	@Override
	public Component getTableCellRendererComponent(JTable table, Object value,
			boolean isSelected, boolean hasFocus, int row, int column) {
		Integer i = (Integer) value;
		String text = "Completed";
		if (i < 0) {
			text = "Error";
		} else if (i <= 100) {
			b.setValue(i);
			return b;
		}
		super.getTableCellRendererComponent(table, text, isSelected, hasFocus, row, column);
		return this;
	}
}
