package net.skaldchen.commons.dao;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SQLiteDAO extends BaseDAO {

	private Logger log = LoggerFactory.getLogger(SQLiteDAO.class);

	private String dburl;
	private Connection conn;
	private Statement stmt;

	public SQLiteDAO(String dbfile) {
		this.dburl = "jdbc:sqlite:" + dbfile;
	}

	public void connect() throws SQLException {
		try {
			Class.forName("org.sqlite.JDBC");
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
		}
		conn = getConn();
		conn.close();
	}

	private Connection getConn() throws SQLException {
		conn = DriverManager.getConnection(dburl);
		return conn;
	}

	public List<Map<String,Object>> queryAsList(String sql) {
		ResultSet rs = null;
		ResultSetMetaData rsmd = null;
		log.debug("query: " + sql);

		List<Map<String,Object>> res = new ArrayList<Map<String,Object>>();
		try {
			conn = getConn();
			stmt = conn.createStatement();
			rs = stmt.executeQuery(sql);
			rsmd = rs.getMetaData();
			while (rs.next()) {
				Map<String,Object> e = new HashMap<String,Object>();
				for (int i = 1; i <= rsmd.getColumnCount(); i++) {
					String name = rsmd.getColumnName(i);
					Object obj = rs.getObject(i);
					e.put(name, obj);
				}
				res.add(e);
			}
			rs.close();
			stmt.close();
			conn.close();
		} catch (SQLException e) {
			log.warn("fail query", e);
		}

		return res;
	}

	public ResultSet query(String sql) {
		ResultSet rs = null;
		try {
			conn = getConn();
			stmt = conn.createStatement();
			rs = stmt.executeQuery(sql);
			//stmt.close();
			//conn.close();
		} catch (SQLException e) {
			log.warn("query fail", e);
		}
		return rs;
	}

	public boolean execute(String sql) {
		try {
			log.debug("execute: " + sql);
			conn = getConn();
			stmt = conn.createStatement();
			int count = stmt.executeUpdate(sql);
			stmt.close();
			conn.close();
			return (count > 0);
		} catch (SQLException e) {
			e.printStackTrace();
		}
		return false;
	}

	public boolean isExisted(String sql) {
		boolean success = false;
		try {
			conn = getConn();
			stmt = conn.createStatement();

			ResultSet rs = stmt.executeQuery(sql);
			if(rs.next()) {
				success = true;
			}

			rs.close();
			stmt.close();
			conn.close();
		} catch (SQLException e) {
			log.warn("isExisted fail", e);
		}
		return success;
	}

}
