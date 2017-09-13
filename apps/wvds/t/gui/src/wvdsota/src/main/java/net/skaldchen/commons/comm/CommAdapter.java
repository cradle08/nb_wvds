package net.skaldchen.commons.comm;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.skaldchen.commons.utils.BytesUtil;

public class CommAdapter implements Runnable {

    private final Logger log = LoggerFactory.getLogger(CommAdapter.class);

    protected List<byte[]> inQueue;

    private List<PacketFilter>   filters;
    private int filterResult = PacketFilter.NOMATCH;

    private List<PacketListener> listeners;

    private boolean running = false;

    public CommAdapter() {
        inQueue = new ArrayList<byte[]>();

        filters = new ArrayList<PacketFilter>();
        listeners = new ArrayList<PacketListener>();
    }

    public void open() throws Exception {
        this.running = true;
        new Thread(this).start();
    }

    public void close() throws Exception {
        this.running = false;
    }

    public void send(byte[] packet) throws IOException {

    }

    public void receive(byte[] packet) {
        log.debug(String.format("rcvd: (%2d) %s", packet.length, BytesUtil.strBytes(packet)));
        inQueue.add(packet);
    }

    public void addPacketFilter(PacketFilter filter) {
        filters.add(filter);
    }

    public void addPacketListener(PacketListener l) {
        listeners.add(l);
    }

    private void notifyListeners(byte[] pkt) {
        for (PacketListener l : listeners)
            l.packetReceived(pkt);
    }

    @Override
    public void run() {
        while (running) {
            try {
                while (inQueue.size() > 0) {
                    byte[] pkt = inQueue.get(0);

                    for (PacketFilter f : filters) {
                        int r = f.filter(pkt);
                        if (r == PacketFilter.MATCHED) {
                            byte[] packet = f.getPacket();
                            if (packet != null) {
                                log.debug(String.format("handle as %s packet", f.getName()));
                                log.debug(String.format("rcvd: (%2d) %s", packet.length, BytesUtil.strBytes(packet)));
                                notifyListeners(packet);
                            }

                            inQueue.remove(0);
                            filterResult = PacketFilter.MATCHED;
                            break; // no need for other filters to check
                        }
                        else if (r == PacketFilter.WAIT) {
                            log.debug(String.format("wait %s packet complete", f.getName()));
                            filterResult = PacketFilter.WAIT;
                            // let next filter also check it
                        }
                        else if (r == PacketFilter.NOMATCH) {
                            // Pending filters should have been reset by itself.
                            // nothing need to do, let next filter check it
                        }
                        else {
                            log.warn(String.format("invalid ret value %d", r));
                        }
                    }

                    if (filterResult == PacketFilter.NOMATCH) {
                        log.debug(String.format("handle as raw packet"));
                        inQueue.remove(0);
                        notifyListeners(pkt);
                    }
                    else if (filterResult == PacketFilter.MATCHED) {
                        // prepare for next operation
                        filterResult = PacketFilter.NOMATCH;
                        for (PacketFilter f : filters)
                            f.reset();
                    }
                    else if (filterResult == PacketFilter.WAIT) {
                        inQueue.remove(0);
                    }
                }

                Thread.sleep(1);
            } catch (Exception e) {
                log.warn("fail process", e);
            }
        }
        log.error("thread stopped");
    }

}
