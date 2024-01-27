package org.chromium.chrome.browser.vdl.crypto;

import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigInteger;
import java.net.URL;
import java.net.URLConnection;
import java.security.Security;

public class Crypto {
    private Cipher cipher;
    private String baseUrl;
    private String keyUrl;
    private byte[] key;
    private byte[] iv;

    boolean forceKey = false;

    public Crypto(String baseUrl) {
        this(baseUrl, null);
    }

    public Crypto(String baseUrl, String key) {
        this.baseUrl = baseUrl;

        if (key != null) {
            this.key = unpackHexString(key);
            forceKey = true;
        }
    }

    public CipherInputStream wrapInputStream(InputStream in) {
        return new CipherInputStream(in, cipher);
    }

    public boolean hasKey() {
        return (key != null);
    }

    public String getCurrentKey() {
        return arrayToString(key);
    }

    public String getCurrentIV() {
        return arrayToString(iv);
    }

    public void updateKeyString(String keyString) throws CryptoException {
        String[] parts = keyString.split(",");

        String newKeyUrl = parts[1].split("\"")[1];

        if (parts.length > 2) {
            String ivString = parts[2].split("0x")[1];
            iv = unpackHexString(ivString);
        } else {
            iv = new byte[16];
        }

        try {
            if (!forceKey && !newKeyUrl.equals(keyUrl)) {
                if (newKeyUrl.startsWith("http")) {
                    keyUrl = newKeyUrl;
                } else {
                    keyUrl = baseUrl + newKeyUrl;
                }

                fetchKey();
            }
        } catch (IOException e) {
            throw new CryptoException(e);
        }

        initCipher();
    }

    private void fetchKey() throws IOException {
        this.key = new byte[16];

        URLConnection con = new URL(this.keyUrl).openConnection();
        InputStream in = new BufferedInputStream(con.getInputStream());

        int read = 0;
        int offset = 0;

        while (offset < 16) {
            if ((read = in.read(this.key, offset, key.length - offset)) == -1)
                break;

            offset += read;
        }

        in.close();
    }

    private void initCipher() throws CryptoException {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());

        try {
            SecretKey secretKey = new SecretKeySpec(key, "AES");

            cipher = Cipher.getInstance("AES/CBC/PKCS7Padding", "BC");
            cipher.init(Cipher.DECRYPT_MODE, secretKey, new IvParameterSpec(iv));
        } catch (Exception e) {
            throw new CryptoException(e);
        }
    }

    private byte[] unpackHexString(String hexString) {
        byte[] array = new byte[16];

        BigInteger bigInt = new BigInteger(hexString, 16);
        BigInteger andValue = new BigInteger("ff", 16);

        for (int i = 0, index = 15; i < 16; i++, index--) {
            BigInteger shift = bigInt.shiftRight(i * 8);
            shift = shift.and(andValue);
            array[index] = shift.byteValue();
        }

        return array;
    }

    private String arrayToString(byte[] array) {
        StringBuilder sb = new StringBuilder();

        if (array != null) {
            for (byte b : array)
                sb.append(String.format("%02x", b));
        }

        return sb.toString();
    }

    @Override
    public String toString() {
        return "net.chrislongo.hls.Crypto{" +
                "cipher=" + cipher +
                ", keyUrl='" + keyUrl + '\'' +
                ", key=" + arrayToString(key) +
                ", iv=" + arrayToString(iv) +
                '}';
    }
}
