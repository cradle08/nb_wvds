package net.skaldchen.commons.utils;

public class Uptime {

	private int second = 0;
	private int minute = 0;
	private int hour = 0;
	private int day = 0;

	public Uptime() {
	}

	public void inc() {
		second += 1;
		if (second == 60) {
			second = 0;
			minute += 1;
			if (minute == 60) {
				minute = 0;
				hour += 1;
				if (hour == 24) {
					hour = 0;
					day += 1;
				}
			}
		}
	}

	public String toString() {
		return String.format("%dd %02d:%02d:%02d", day, hour, minute, second);
	}

}
