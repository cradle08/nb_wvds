package com.cadre.wvds.sniffer;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.border.BevelBorder;

import org.apache.commons.io.FileUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.alibaba.fastjson.JSON;

@SuppressWarnings("serial")
public class AlgoDialog extends JDialog implements ActionListener {

	private static final Logger log = LoggerFactory.getLogger(AlgoDialog.class);

	private List<Param> params;
	private JLabel[] labels;
	private JTextField[] inputs;

	private final String fname = "dat/algo.json";
	private final String encoding = "UTF-8";

	public AlgoDialog(JFrame parent) {
		super(parent, true);
		initUI();
	}

	private void initUI() {
		setTitle("默认算法参数");
		setSize(240,400);
		setResizable(false);
		setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);

		getContentPane().setLayout(new BorderLayout());

		JPanel mainPane = new JPanel();
		mainPane.setLayout(null);
		getContentPane().add(mainPane, BorderLayout.CENTER);
		mainPane.setBorder(BorderFactory.createBevelBorder(BevelBorder.RAISED));

		String text = "[]";
		try {
			text = FileUtils.readFileToString(new File(fname), encoding);
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
				mainPane.add(labels[i]);
				labels[i].setBounds(30, 25+30*i, 100, 20);

				inputs[i] = new JTextField();
				mainPane.add(inputs[i]);
				inputs[i].setColumns(8);
				inputs[i].setBounds(130, 25+30*i, 50, 20);
				inputs[i].setText(p.getValue());
				//if (i > 8) inputs[i].setEnabled(false);
				++i;
			}
		}

		JPanel botPane = new JPanel();
		getContentPane().add(botPane, BorderLayout.SOUTH);

		JButton saveBtn = new JButton("保存");
		botPane.add(saveBtn);
		saveBtn.setActionCommand("save");
		saveBtn.addActionListener(this);

		JButton closeBtn = new JButton("关闭");
		botPane.add(closeBtn);
		closeBtn.setActionCommand("close");
		closeBtn.addActionListener(this);
	}

	private boolean save() {
		for (int i = 0; i < params.size(); i++) {
			String str = inputs[i].getText();
			if (str == null || !str.matches("^[-]?[0-9]+$")) {
				String message = String.format("\"%s\"的输入应为整数", params.get(i).getName());
				JOptionPane.showMessageDialog(this, message);
				return false;
			}
			params.get(i).setValue(str);
		}

		String text = JSON.toJSONString(params);
		try {
			FileUtils.writeStringToFile(new File(fname), text, encoding);
			JOptionPane.showMessageDialog(this, "保存成功");
		} catch (IOException e) {
			log.warn("fail write " + fname, e);
			JOptionPane.showMessageDialog(this, "保存失败");
		}
		return true;
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		String act = e.getActionCommand();

		if ("save".equals(act)) {
			if (save())
				dispose();
		}
		else if ("close".equals(act)) {
			dispose();
		}
	}
}
