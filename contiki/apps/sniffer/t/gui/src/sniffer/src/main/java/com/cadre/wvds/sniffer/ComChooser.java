package com.cadre.wvds.sniffer;

import gnu.io.CommPortIdentifier;
import net.skaldchen.commons.utils.ComUtil;
import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.I18N;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Enumeration;

import javax.swing.BorderFactory;
import javax.swing.ComboBoxModel;
import javax.swing.DefaultComboBoxModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingConstants;
import javax.swing.border.BevelBorder;

/**
* This code was edited or generated using CloudGarden's Jigloo
* SWT/Swing GUI Builder, which is free for non-commercial
* use. If Jigloo is being used commercially (ie, by a corporation,
* company or business for any purpose whatever) then you
* should purchase a license for each developer using Jigloo.
* Please visit www.cloudgarden.com for details.
* Use of Jigloo implies acceptance of these licensing terms.
* A COMMERCIAL LICENSE HAS NOT BEEN PURCHASED FOR
* THIS MACHINE, SO JIGLOO OR THIS CODE CANNOT BE USED
* LEGALLY FOR ANY CORPORATE OR COMMERCIAL PURPOSE.
*/
@SuppressWarnings("serial")
public class ComChooser extends javax.swing.JDialog implements ActionListener {

	private final int WIDTH  = 280;
	private final int HEIGHT = 210;

	private JPanel mainPanel;
	private JButton okButton;
	private JLabel hintLabel;
	private JComboBox<String> baudComboBox;
	private JLabel baudLabel;
	private JComboBox<String> nameComboBox;
	private JLabel nameLabel;
	private JButton quitButton;
	private JPanel botPanel;

	public ComChooser() {
		initGUI();
		load();
	}

	private void initGUI() {
		try {
			{
				botPanel = new JPanel();
				getContentPane().add(botPanel, BorderLayout.SOUTH);
				{
					okButton = new JButton();
					botPanel.add(okButton);
					okButton.setText(I18N.get("button.ok"));
					okButton.setPreferredSize(new Dimension(60,22));
					okButton.setActionCommand("ok");
					okButton.addActionListener(this);
				}
				{
					quitButton = new JButton();
					botPanel.add(quitButton);
					quitButton.setText(I18N.get("button.quit"));
					quitButton.setPreferredSize(new Dimension(60,22));
					quitButton.setActionCommand("quit");
					quitButton.addActionListener(this);
				}
			}
			{
				mainPanel = new JPanel();
				getContentPane().add(mainPanel, BorderLayout.CENTER);
				mainPanel.setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));
				mainPanel.setLayout(null);
				{
					nameLabel = new JLabel();
					mainPanel.add(nameLabel);
					nameLabel.setText(I18N.get("com.name"));
					nameLabel.setBounds(25, 54, 70, 15);
					nameLabel.setHorizontalAlignment(SwingConstants.TRAILING);
				}
				{
					ComboBoxModel<String> jComboBox1Model =
							new DefaultComboBoxModel<String>(
									new String[] { "Item One", "Item Two" });
					nameComboBox = new JComboBox<String>();
					mainPanel.add(nameComboBox);
					nameComboBox.setModel(jComboBox1Model);
					nameComboBox.setBounds(100, 51, 90, 22);
				}
				{
					baudLabel = new JLabel();
					mainPanel.add(baudLabel);
					baudLabel.setText(I18N.get("com.baud"));
					baudLabel.setBounds(25, 89, 70, 15);
					baudLabel.setHorizontalAlignment(SwingConstants.TRAILING);
				}
				{
					String[] BAUDs = { "115200", "57600", "38400", "19200", "9600" };
					ComboBoxModel<String> jComboBox2Model =
							new DefaultComboBoxModel<String>(BAUDs);
					baudComboBox = new JComboBox<String>();
					mainPanel.add(baudComboBox);
					baudComboBox.setModel(jComboBox2Model);
					baudComboBox.setBounds(100, 86, 90, 22);
				}
				{
					hintLabel = new JLabel();
					mainPanel.add(hintLabel);
					hintLabel.setText(I18N.get("com.hint"));
					hintLabel.setBounds(14, 22, 204, 15);
				}
			}

			setTitle(I18N.get("com.title"));
			setSize(WIDTH, HEIGHT);
			setAlwaysOnTop(true);
			setResizable(false);
			setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private void load() {
		Enumeration<?> e = CommPortIdentifier.getPortIdentifiers();
		nameComboBox.removeAllItems();
		while (e.hasMoreElements()) {
			CommPortIdentifier com = (CommPortIdentifier) e.nextElement();
			if (com.getPortType() == CommPortIdentifier.PORT_SERIAL)
				nameComboBox.addItem(com.getName());
		}

		nameComboBox.setSelectedItem(Config.get("com-name"));
		baudComboBox.setSelectedItem(Config.get("com-baud"));
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		String action = e.getActionCommand();
		if (action.equals("ok")) {
			String com = (String) nameComboBox.getSelectedItem();
			String baud = (String) baudComboBox.getSelectedItem();

			if (ComUtil.avail(com)) {
				Config.set("com-name", com);
				Config.set("com-baud", baud);
				Config.save();

				Main.com_done = true;
				dispose();
			}
			else {
				String message = "串口" + com + "已被占用，可能原因及措施：\n"
						+ "1) 其他程序已使用该串口，请关闭该程序；\n"
						+ "2) 选择了错误的串口，请选择正确串口。";
				JOptionPane.showMessageDialog(this, message);
			}
		}
		else if (action.equals("quit")) {
			dispose();
			System.exit(0);
		}
	}

}
