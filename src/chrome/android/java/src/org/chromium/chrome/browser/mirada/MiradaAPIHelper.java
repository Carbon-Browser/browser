package org.chromium.chrome.browser.mirada;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.SocketTimeoutException;
import org.chromium.base.task.AsyncTask;

import org.json.JSONObject;
import org.json.JSONArray;
import android.icu.math.BigDecimal;
import java.lang.Integer;
import java.lang.Long;
import java.lang.Double;
import java.lang.Float;

import wallet.core.jni.CoinType;

import java.math.RoundingMode;
import android.icu.math.MathContext;

import java.util.ArrayList;

import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

public class MiradaAPIHelper {

    public static final String MIRADA_API_KEY = "";

    public void sendAPIRequestBody(String url, String body) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(url);

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setDoOutput(true);
                    conn.setConnectTimeout(20000);
                    conn.setDoInput(true);
                    conn.setUseCaches(false);
                    conn.setRequestMethod("POST");
                    conn.setRequestProperty("Content-Type", "application/json");

                    byte[] outputInBytes = body.getBytes("UTF-8");
                    OutputStream os = conn.getOutputStream();
                    os.write( outputInBytes );
                    os.close();

                    // handle the response
                    int status = conn.getResponseCode();
                    if (status != 200) {
                        throw new IOException("Post failed with error code " + status);
                    } else {
                        BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                        String inputLine;
                        while ((inputLine = in.readLine()) != null) {
                            response.append(inputLine);
                        }
                        in.close();
                    }
                } catch (SocketTimeoutException timeout) {
                  timeout.printStackTrace();
                    // Time out, don't set a background - lazy
                } catch (Exception e) {

                } finally {
                    if (conn != null)
                        conn.disconnect();
                }

                return response.toString();
            }

            @Override
            protected void onPostExecute(String result) {
                try {

                } catch (Exception e) {}
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    // public void sendTransaction(String chainType, String hexString, String tokenSymbol, TransactionCallback callback) {
    //     String url = "";
    //     if (chainType.equals("BEP20")) {
    //         url = "https://go.getblock.io/5855ef8cc8c04dfd808817090d98dd7c";
    //     } else {
    //         url = "https://go.getblock.io/56d1ba70818d40a19f936b8a0bffcece";
    //     }
    //
    //     String body = "{"
    //         + "\"jsonrpc\": \"2.0\","
    //         + "\"method\": \"eth_sendRawTransaction\","
    //         + "\"params\": ["
    //         + "\"" + hexString + "\""
    //         + "],"
    //         + "\"id\": \"getblock.io\""
    //         + "}";
    //
    //     sendAPIRequestBody(url, Web3Enum.TRANSACTION, "POST", tokenSymbol, body, callback);
    // }
}
