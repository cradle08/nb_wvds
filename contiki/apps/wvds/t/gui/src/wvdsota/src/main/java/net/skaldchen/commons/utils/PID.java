package net.skaldchen.commons.utils;

//import java.io.File;
//import java.io.FileInputStream;
//import java.io.FileNotFoundException;
//import java.io.FileOutputStream;
//import java.io.IOException;
//import java.io.InputStream;
import java.lang.management.ManagementFactory;
import java.lang.management.RuntimeMXBean;

//import org.slf4j.Logger;
//import org.slf4j.LoggerFactory;

/**
 * Process ID in Java
 *
 * @author lichengwu
 * @created 2012-1-18
 *
 * @version 1.0
 */
public final class PID {

    //private static final Logger log = LoggerFactory.getLogger(PID.class);

    /**
     * ˽�й��췽��
     */
    private PID() {
        super();
    }

    /**
     * ���java����id
     *
     * @author lichengwu
     * @created 2012-1-18
     *
     * @return java����id
     */
    public static final String getPID() {
        String pid = System.getProperty("pid");
        if (pid == null) {
            RuntimeMXBean runtimeMXBean = ManagementFactory.getRuntimeMXBean();
            String processName = runtimeMXBean.getName();
            if (processName.indexOf('@') != -1) {
                pid = processName.substring(0, processName.indexOf('@'));
            } else {
                //pid = getPIDFromOS();
            }
            System.setProperty("pid", pid);
        }
        return pid;
    }

    /**
     * �Ӳ���ϵͳ���pid
     * <p>
     * ����windows����ο�:http://www.scheibli.com/projects/getpids/index.html
     *
     * @author lichengwu
     * @created 2012-1-18
     *
     * @return
     */
    //private static String getPIDFromOS() {
    //    String pid = null;
    //    String[] cmd = null;
    //    File tempFile = null;

    //    String osName = ParameterUtil.getParameter(Parameter.OS_NAME);
    //    // ����windows
    //    if (osName.toLowerCase().contains("windows")) {
    //        FileInputStream fis = null;
    //        FileOutputStream fos = null;

    //        try {
    //            // ������ʱgetpids.exe�ļ�
    //            tempFile = File.createTempFile("getpids", ".exe");
    //            File getpids = new File(ParameterUtil.getResourcePath("getpids.exe"));
    //            fis = new FileInputStream(getpids);
    //            fos = new FileOutputStream(tempFile);
    //            byte[] buf = new byte[1024];
    //            while (fis.read(buf) != -1) {
    //                fos.write(buf);
    //            }
    //            // �����ʱgetpids.exe�ļ�·����Ϊ����
    //            cmd = new String[] { tempFile.getAbsolutePath() };
    //        } catch (FileNotFoundException e) {
    //            log.equals(e);
    //        } catch (IOException e) {
    //            log.equals(e);
    //        } finally {
    //            if (tempFile != null) {
    //                tempFile.deleteOnExit();
    //            }
    //            Closer.close(fis, fos);
    //        }
    //    }

    //    // �����windows
    //    else {
    //        cmd = new String[] { "/bin/sh", "-c", "echo $ $PPID" };
    //    }
    //    InputStream is = null;
    //    ByteArrayOutputStream baos = null;
    //    try {
    //        byte[] buf = new byte[1024];
    //        Process exec = Runtime.getRuntime().exec(cmd);
    //        is = exec.getInputStream();
    //        baos = new ByteArrayOutputStream();
    //        while (is.read(buf) != -1) {
    //            baos.write(buf);
    //        }
    //        String ppids = baos.toString();
    //        // ����windows�ο���http://www.scheibli.com/projects/getpids/index.html
    //        pid = ppids.split(" ")[1];
    //    } catch (Exception e) {
    //        log.error(e);
    //    } finally {
    //        if (tempFile != null) {
    //            tempFile.deleteOnExit();
    //        }
    //        Closer.close(is, baos);
    //    }
    //    return pid;
    //}
}