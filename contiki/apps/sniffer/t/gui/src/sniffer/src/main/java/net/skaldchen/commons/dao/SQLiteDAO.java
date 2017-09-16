package net.skaldchen.commons.dao;

import java.sql.DriverManager;
import java.sql.SQLException;

/**
 * Created with IntelliJ IDEA.
 * User: admin
 * Date: 13-4-22
 * Time: 下午10:20
 * To change this template use File | Settings | File Templates.
 */
public class SQLiteDAO extends BaseDAO {

    private String dbfile = null;

    public SQLiteDAO(String dbfile) {
        super("org.sqlite.JDBC");
        this.dbfile = dbfile;
    }

    @Override
    protected void doConnect() throws SQLException {
        conn = DriverManager.getConnection("jdbc:sqlite:" + dbfile);
        stmt = conn.createStatement();
    }

}
