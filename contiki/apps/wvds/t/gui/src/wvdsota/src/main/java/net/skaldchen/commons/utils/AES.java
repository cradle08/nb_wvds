package net.skaldchen.commons.utils;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

public class AES
{
    public static byte[] encrypt(byte[] dataBytes, String key, String iv) throws Exception {
        return encrypt(dataBytes, key.getBytes(), iv.getBytes());
    }

    public static byte[] encrypt(byte[] dataBytes, byte[] key, byte[] iv) throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        int blockSize = cipher.getBlockSize();

        int plaintextLength = dataBytes.length;
        if (plaintextLength % blockSize != 0) {
            plaintextLength = plaintextLength + (blockSize - (plaintextLength % blockSize));
        } else {
            plaintextLength += blockSize;
        }

        byte[] plaintext = new byte[plaintextLength];
        System.arraycopy(dataBytes, 0, plaintext, 0, dataBytes.length);
        for (int i = dataBytes.length; i < plaintext.length; i++)
            plaintext[i] = (byte) (plaintext.length - dataBytes.length);

        SecretKeySpec keyspec = new SecretKeySpec(key, "AES");
        IvParameterSpec ivspec = new IvParameterSpec(iv);

        cipher.init(Cipher.ENCRYPT_MODE, keyspec, ivspec);
        byte[] encrypted = cipher.doFinal(plaintext);

        return encrypted;
    }

    public static byte[] decrypt(byte[] encrypted, String key, String iv) throws Exception {
        return decrypt(encrypted, key.getBytes(), iv.getBytes());
    }

    public static byte[] decrypt(byte[] encrypted, byte[] key, byte[] iv) throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");

        SecretKeySpec keyspec = new SecretKeySpec(key, "AES");
        IvParameterSpec ivspec = new IvParameterSpec(iv);

        cipher.init(Cipher.DECRYPT_MODE, keyspec, ivspec);

        byte[] original = cipher.doFinal(encrypted);
        return original;
    }

}
