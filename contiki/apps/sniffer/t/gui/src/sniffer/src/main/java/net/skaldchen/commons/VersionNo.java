package net.skaldchen.commons;

public class VersionNo {

	public int major;
	public int minor;
	public int revision;
	public String build;
	public String state;

	public VersionNo() {}

	public VersionNo(int major, int minor, int rev) {
		this(major, minor, rev, "");
	}

	public VersionNo(int major, int minor, int rev, String state) {
		this.major = major;
		this.minor = minor;
		this.revision = rev;
		this.state = state;
	}

	// return true if a is newer than b
	public static boolean newer(VersionNo a, VersionNo b) {
		return (a.major > b.major || a.minor > b.minor || a.revision > b.revision);
	}

	// must like [0-9]+.[0-9]+.[0-9]+[-]*[a-zA-Z]*
	public static VersionNo parse(String version) {
		final String pattern = "^[0-9]+.[0-9]+.[0-9]+[-]*[a-zA-Z]*$";
		if (!version.matches(pattern)) {
			System.err.println("wrong version number " + version);
			return null;
		}
		VersionNo ver = new VersionNo();
		String[] nums = version.split("[.]");
		for (int i = 0; i < 3; i++) {
			ver.major = Integer.parseInt(nums[0]);
			ver.minor = Integer.parseInt(nums[1]);
			if (nums[2].indexOf("-") == -1) {
				ver.revision = Integer.parseInt(nums[2]);
			} else {
				String[] strs = nums[2].split("-");
				ver.revision = Integer.parseInt(strs[0]);
				ver.state = strs[1];
			}
		}
		return ver;
	}

	// Return the string form of linux kernal style version number
	// For example: 2.6.32 -> (2<<16) + (6<<8) + 32 -> 132640
	public static String parseVerno(long verno) {
		StringBuilder sb = new StringBuilder();
		sb.append((verno >> 16) & 0xff);
		sb.append(".");
		sb.append((verno >> 8) & 0xff);
		sb.append(".");
		sb.append(verno & 0xff);
		return sb.toString();
	}

	public String toString() {
		return "" + major + "." + minor + "." + revision
				+ ((state.length() > 0) ? "-" + state : "");
	}
}
