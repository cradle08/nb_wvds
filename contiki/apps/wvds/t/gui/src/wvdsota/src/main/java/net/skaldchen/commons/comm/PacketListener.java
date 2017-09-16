package net.skaldchen.commons.comm;

/**
 * Created with IntelliJ IDEA.
 * User: Administrator
 * Date: 13-11-12
 * Time: 上午9:38
 * To change this template use File | Settings | File Templates.
 */
public interface PacketListener {

    public void packetReceived(byte[] packet);
}
