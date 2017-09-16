package net.skaldchen.commons.dao;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.List;
import java.util.Map;

public class BaseDAO {

    public BaseDAO() { }

    public BaseDAO(String dbhost, String dbname, String user, String passwd) { }

    public void connect() throws SQLException {

    }

    public List<Map<String,Object>> queryAsList(String sql) {
        return null;
    }

    public ResultSet query(String sql) {
        return null;
    }

    public boolean execute(String sql) {
        return false;
    }

    public boolean isExisted(String sql) {
        return false;
    }

}
