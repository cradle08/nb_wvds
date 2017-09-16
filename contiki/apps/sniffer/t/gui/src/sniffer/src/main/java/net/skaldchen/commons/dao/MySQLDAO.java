package net.skaldchen.commons.dao;


public class MySQLDAO extends BaseDAO {

	private final static String DRIVER = "com.mysql.jdbc.Driver";
	private final static String URL = "jdbc:mysql://<dbhost>/<dbname>?useUnicode=true&characterEncoding=UTF8";

	public MySQLDAO(String dbhost, String dbname, String user, String passwd) {
		super(DRIVER, URL.replaceAll("<dbhost>", dbhost).replaceAll("<dbname>", dbname), dbhost, user, passwd);
	}

}
