package net.skaldchen.echart;

import java.io.File;

import javax.swing.JFrame;
import javax.swing.UIManager;

import org.apache.commons.io.FileUtils;

import net.skaldchen.commons.utils.Config;
import net.skaldchen.commons.utils.PID;

public class Main {

	private static JFrame frame;
	private static EnergyChartPane chart;

	public static void main(String[] args) {
		Config.init("echart.ini");

		initTheme();

		frame = new JFrame();
		frame.setTitle("EnergyChart");
		frame.setSize(1000,600);
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		chart = new EnergyChartPane();
		frame.getContentPane().add(chart);

		try {
			String pid = PID.getPID();
			FileUtils.writeStringToFile(new File("echart.pid"), pid);
		} catch(Exception e) {
			e.printStackTrace();
		}

		frame.setLocationRelativeTo(null);
		frame.setVisible(true);
	}

	private static void initTheme() {
		String lnf = null;
		//lnf = "com.sun.java.swing.plaf.windows.WindowsLookAndFeel";
		lnf = "com.jtattoo.plaf.acryl.AcrylLookAndFeel";
		try {
			UIManager.setLookAndFeel(lnf);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
}
