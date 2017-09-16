package com.cadre.wvds.wvdsota;

import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JPanel;
import java.awt.BorderLayout;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JTextField;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.border.BevelBorder;

@SuppressWarnings("serial")
public class DlgSubnet extends JDialog {

	private JTextField cityTextField;
	private JTextField districtTextField;
	private JTextField streetTextField;
	private JComboBox<Integer> chanCombo;

	public DlgSubnet(JFrame parent) {
		super(parent, true);
		setTitle("子网管理");
		setSize(320, 240);
		setResizable(false);
		setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);

		JPanel panel = new JPanel();
		panel.setBorder(new BevelBorder(BevelBorder.RAISED, null, null, null, null));
		getContentPane().add(panel, BorderLayout.CENTER);
		panel.setLayout(null);

		JLabel cityLabel = new JLabel("城市:");
		cityLabel.setBounds(30, 21, 45, 18);
		panel.add(cityLabel);

		cityTextField = new JTextField();
		cityTextField.setBounds(80, 20, 90, 20);
		panel.add(cityTextField);
		cityTextField.setColumns(10);

		JLabel districtLabel = new JLabel("区县:");
		districtLabel.setBounds(30, 56, 45, 18);
		panel.add(districtLabel);

		districtTextField = new JTextField();
		districtTextField.setBounds(80, 55, 90, 20);
		panel.add(districtTextField);
		districtTextField.setColumns(10);

		JLabel streetLabel = new JLabel("街道:");
		streetLabel.setBounds(30, 91, 45, 18);
		panel.add(streetLabel);

		streetTextField = new JTextField();
		streetTextField.setBounds(80, 90, 180, 20);
		panel.add(streetTextField);
		streetTextField.setColumns(10);

		JLabel chanLabel = new JLabel("信道:");
		chanLabel.setBounds(30, 126, 45, 18);
		panel.add(chanLabel);

		final int[] CHANs = new int[]{1,2,3,4,5,6,7,8,9,10};
		chanCombo = new JComboBox<Integer>();
		chanCombo.setBounds(80, 125, 50, 20);
		panel.add(chanCombo);
		for (int chan : CHANs)
			chanCombo.addItem(Integer.valueOf(chan));
		chanCombo.setSelectedItem(Integer.valueOf(6));

		JPanel panel_1 = new JPanel();
		getContentPane().add(panel_1, BorderLayout.SOUTH);

		JButton addButton = new JButton("添加");
		addButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				Subnet net = new Subnet();
				net.city = cityTextField.getText();
				net.district = districtTextField.getText();
				net.street = streetTextField.getText();
				net.rfchan = (Integer)chanCombo.getSelectedItem();
				net.rfpower = 24;
				net.vdVer = 0; net.vdFile = "VD.txt"; net.vdSize = 0;
				net.rpVer = 0; net.rpFile = "RP.txt"; net.rpSize = 0;
				net.apVer = 0; net.apFile = "AP.txt"; net.apSize = 0;
				SubnetManager.getInstance().addSubnet(net);
			}
		});
		panel_1.add(addButton);

		JButton quitButton = new JButton("关闭");
		quitButton.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent e) {
				dispose();
			}
		});
		panel_1.add(quitButton);
	}
}
