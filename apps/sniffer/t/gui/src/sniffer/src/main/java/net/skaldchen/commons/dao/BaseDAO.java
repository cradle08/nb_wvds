package net.skaldchen.commons.dao;

//import cn.ac.siat.ietcwsn.common.utils.I18N;
//import cn.ac.siat.ietcwsn.wsnamr.*;
//import cn.ac.siat.ietcwsn.wsnamr.client.view.StateListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.FileInputStream;
import java.io.InputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Vector;


public class BaseDAO implements Runnable {

    private static Logger log = LoggerFactory.getLogger(BaseDAO.class);

    protected String driver;
    protected String dburl;
    protected String dbhost;
    protected String user;
    protected String passwd;

    protected Connection conn = null;
    protected Statement stmt = null;
//    private Vector<StateListener> listeners = new Vector<StateListener>();

    private Vector<String> sqlQueue;
    private Thread thread = null;
    private boolean running = false;

    private final int THREAD_PERIOD = 1000;
    private final int ERRMSG_LEN = 80;

    public BaseDAO(String driver) {
        // load the database driver
        try {
            Class.forName(driver);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    public BaseDAO(String driver, String url, String host, String user, String passwd) {
        debug("BaseDAO: init <" + url + ">, " + user + ":" + passwd);

        this.driver = driver;
        this.dburl = url;
        this.dbhost = host;
        this.user = user;
        this.passwd = passwd;

        //this.listeners = new Vector<StateListener>();
        this.sqlQueue = new Vector<String>();

        // load the database driver
        try {
            Class.forName(driver);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }
    }

    public void connect() throws SQLException {
        debug("BaseDAO: connect");
        running = true;

        doConnect();
        if (stmt != null) {
            debug("BaseDAO: stmt ready");
//            MainClass.database_ok = true;
            //notifyListeners(Constants.EVENT_DATABASE_OK);
        }

        startT();
    }

    private void startT() {
        if (thread == null || !thread.isAlive()) {
            thread = new Thread(this);
            thread.start();
            debug("BaseDAO: thread start");
        }
    }

    protected void doConnect() throws SQLException {
        if (stmt == null) {
            if (conn == null || conn.isClosed()) {
                debug("BaseDAO: get conn");
                conn = DriverManager.getConnection(dburl, user, passwd);
                if (conn != null) {
                    debug("BaseDAO: conn ready");
                }
            }
            debug("BaseDAO: get stmt");
            stmt = conn.createStatement();
        }
    }

    public void disconnect() {
        debug("BaseDAO: disconnect");
        running = false;
        if (thread != null)
            thread.interrupt();
    }

    private void doDisconnect() throws SQLException {
        debug("BaseDAO: do disconnect");
        if (stmt != null) {
            stmt.close();
            stmt = null;
        }
        conn = null;
    }

    public boolean isConnected() {
        boolean rval = false;
        try {
            rval = (stmt != null && !conn.isClosed());
        } catch (SQLException e) { e.printStackTrace(); }
        return rval;
    }

    public void waitConn() {
        debug("BaseDAO: wait conn");
        while (!isConnected())
            ;
    }

    public String getServerIP() {
        return dbhost;
    }

//    public void addStateListener(StateListener l) {
//        listeners.add(l);
//    }

//    private void notifyListeners(String desc) {
//        for (StateListener l : listeners) {
//            l.stateChanged(desc);
//        }
//    }

    private void failed(SQLException e) {
        stmt = null;
        log.error("ErrorCode " + e.getErrorCode()
                + ", SQLState " + e.getSQLState()
                + ", Message <" + strLimit(e.getMessage(), ERRMSG_LEN) + ">", e);

        // log the event if cannot connect database
//        if (e.getMessage().startsWith("Communications link failure")) {
//            MainClass.database_ok = false;
//            //notifyListeners(Constants.EVENT_DATABASE_ERR);
//            MainClass.getLogMgr().write(I18N.get("msg.db_conn_down"));
//        }
    }

    public boolean update(String sql) {
        try {
            if (stmt != null) {
                log.debug(sql);
                int count = stmt.executeUpdate(sql);
                return (count > 0);
            } else {
                log.warn("stmt is null when update");
            }
        } catch (SQLException e) {
            failed(e);
        }
        return false;
    }

    public List<Object> query(String sql, Class<?> c) {
        List<Object> list = new ArrayList<Object>();
        Object obj = null;
        Constructor<?> con = null;

        try {
            con = c.getConstructor(); // get the constructor function
            if (stmt != null) {
                log.debug(sql + ", " + c.getName());
                ResultSet rs = stmt.executeQuery(sql);
                ResultSetMetaData rsmd = rs.getMetaData();

                if (rs != null) {
                    while (rs.next()) {
                        obj = con.newInstance();
                        for (int i = 1; i <= rsmd.getColumnCount(); i++) {
                            String name = rsmd.getColumnName(i);
                            String val = rs.getString(i);

                            String func = "set"	+ name.substring(0,1).toUpperCase()
                                + name.substring(1);
                            Method m = c.getMethod(func, String.class);
                            if (m != null && val != null) {
                                m.invoke(obj, val);
                            }
                        }
                        list.add(obj);
                    }
                }
            } else {
                log.warn(String.format("stmt is null when query(%s, %s)", sql, c.getName()));
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return list;
    }

    public ResultSet query(String sql) {
        try {
            if (stmt != null) {
                log.debug(sql);
                return stmt.executeQuery(sql);
            } else {
                log.warn(String.format("stmt is null when query(%s)", sql));
            }
        } catch (SQLException e) {
            failed(e);
        }
        return null;
    }

    public boolean execute(String sql) {
        boolean success = false;
        try {
            //waitConn();
            if (stmt != null) {
                log.debug(sql);
                stmt.execute(sql);
                success = true;
            } else {
                log.warn("stmt is null when execute");
            }
        } catch (SQLException e) {
            failed(e);
        }
        return success;
    }

    public boolean executeFile(String sqlFile) throws Exception {
        List<String> sqlList = loadFile(sqlFile);
        boolean success = false;

        if (stmt != null) {
            debug("load " + sqlFile);
            for (String sql : sqlList) {
                stmt.addBatch(sql);
            }
            stmt.executeBatch();
            success = true;
        }
        return success;
    }

    private List<String> loadFile(String sqlFile) throws Exception {
        List<String> sqlList = new ArrayList<String>();

        try {
            String absSqlFile = sqlFile;
            InputStream sqlFileIn = new FileInputStream(absSqlFile);

            StringBuffer sqlSb = new StringBuffer();
            byte[] buff = new byte[1024];
            int byteRead = 0;
            while ((byteRead = sqlFileIn.read(buff)) != -1) {
                sqlSb.append(new String(buff, 0, byteRead));
            }

            // Windows 下换行是 \r\n, Linux 下是 \n
            String[] sqlArr = sqlSb.toString().split("(;\\s*\\r\\n)|(;\\s*\\n)");
            for (int i = 0; i < sqlArr.length; i++) {
                String sql = sqlArr[i].replaceAll("--.*", "").trim();
                if (!sql.equals("")) {
                    sqlList.add(sql);
                }
            }
            return sqlList;
        } catch (Exception ex) {
            throw new Exception(ex.getMessage());
        }
    }

    private void debug(String s) {
        log.debug(s);
        //System.out.println(df.format(new Date()) + s);
    }

    @Override
    public void run() {
        while (true) {
            //debug("BaseDAO: thread run");
            if (running) {
                try {
                    doConnect();
                } catch (SQLException e) {
                    log.warn("fail connect", e);
                }

                try {
                    Thread.sleep(THREAD_PERIOD);
                } catch (InterruptedException e) { }

            } else {
                // shutdown the database connection
                try {
                    doDisconnect();
                    //notifyListeners(Constants.EVENT_DATABASE_ERR);
                } catch(SQLException e) {
                    log.warn("fail disconnect", e);
                }

                break;
            }
        }

        debug("BaseDAO: thread stopped");
    }

    private static String strLimit(String str, int len) {
        return str.substring(0, (len < str.length() ? len-1 : str.length()-1));
    }

    private final SimpleDateFormat df = new SimpleDateFormat("[HH:mm:ss,SSS] ");
}
