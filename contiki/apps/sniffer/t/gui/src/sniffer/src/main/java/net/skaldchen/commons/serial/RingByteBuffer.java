package net.skaldchen.commons.serial;

import java.io.IOException;
import java.io.InputStream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RingByteBuffer {

    private final Logger log = LoggerFactory.getLogger(RingByteBuffer.class);

	private final int size;
	private final byte[] buf;
	private int length;
	private int nextGet;
	private int nextPut;

	public RingByteBuffer(int size) {
		this.size = size;
		buf = new byte[size];
	}

	public int size() {
		return size;
	}

	public int length() {
		return length;
	}

	public int nextPut() {
		return nextPut;
	}

	public int nextGet() {
		return nextGet;
	}

	public void clear() {
		length = 0;
		nextGet = 0;
		nextPut = 0;
	}

	public byte get(int idx) {
		if (idx >= length)
			throw new RuntimeException(String.format("index %d out of bounds, ringbuf has %d bytes", idx, length));

		int i = nextGet + idx;
		if (i >= size)
			i -= size;
		return buf[i];
	}

	public byte[] get(int ofs, int len) {
		if (len < 1 || len > length)
			return new byte[0];

		byte[] bytes = new byte[len];
		int j = nextGet + ofs;
		if (j >= size)
			j -= size;
		for (int i = 0; i < len; i++) {
			bytes[i] = buf[j];
			if (++j >=size)
				j = 0;
		}
		return bytes;
	}

	public boolean put(byte b) {
		if (length < size) {
			length++;
			buf[nextPut++] = b;
			if (nextPut >= size)
				nextPut = 0;
			return true;
		}
		return false;
	}

	public byte[] pop(int len) {
		if (len < 1 || len > length)
			return new byte[0];

		debug(String.format("pop %d bytes, len %d, get %d, put %d", len, length, nextGet, nextPut));
		byte[] bytes = get(0, len);
		if (bytes.length > 0) {
			len = bytes.length;
			length -= len;
			nextGet += len;
			if (nextGet >= size)
				nextGet -= size;
			debug(String.format("after pop, len %d, get %d, put %d", length, nextGet, nextPut));
			if (length > 0)
				debug("  " + toString());
		}
		return bytes;
	}

	public int putFrom(InputStream in) throws IOException {
		return putFrom(in, size);
	}

	public int putFrom(InputStream in, int maxlen) throws IOException {
		int len = 0;
		int max_read = 0;
		int tot_read = 0;

		while (true) {
			if (in.available() == 0)
				break;

			if (nextGet + length == nextPut) {
				max_read = size - nextPut;
			} else {
				max_read = nextGet - nextPut;
			}
			max_read = Math.min(max_read, in.available());
			if (max_read == 0) // buffer full
				break;

			debug(String.format("  before read get %d, put %d, max %d, %d avail", nextGet, nextPut, max_read, in.available()));
			len = in.read(buf, nextPut, max_read);

			nextPut += len;
			if (nextPut >= size)
				nextPut -= size;
			length += len;
			maxlen -= len;
			tot_read += len;
			debug(String.format("  after read %d bytes, put %d, left %d", len, nextPut, maxlen));
		}
		debug(String.format("read %d bytes, get %d, put %d, totlen %d", tot_read, nextGet, nextPut, length));

		return tot_read;
	}

	private String strBytes(byte[] bytes, int ofs, int len) {
		StringBuilder sb = new StringBuilder();
		for (int i = 0; i < len; i++)
			sb.append(String.format("%02X ", bytes[ofs + i]));
		return (sb.length() > 1) ? sb.substring(0, sb.length() - 1) : "";
	}

	public String toString() {
		StringBuilder sb = new StringBuilder();
		for (int i = nextGet; i != nextPut; ) {
			sb.append(String.format("%02X ", buf[i]));
			if (++i >= size) i = 0;
		}
		return (sb.length() > 1) ? sb.substring(0, sb.length()-1) : "";
	}

	private void debug(String s) {
		log.debug(s);
	}
}
