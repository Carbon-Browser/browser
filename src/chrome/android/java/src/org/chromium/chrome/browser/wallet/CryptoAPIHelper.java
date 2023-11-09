package org.chromium.chrome.browser.wallet;

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

public class CryptoAPIHelper {

    private static final String ETHERSCAN_API_KEY = "EGB6VW4Y4CNTQD3FP4TGA3MAWS3M9TTUKZ";
    private static final String BSCSCAN_API_KEY = "UVMB2DE897HHNS5UX5U5X4U438N54F73IG";

    private static final String INFURNA_API_KEY = "ed0c2c977c8b4f69b8d2b11a0b7c05e7";

    private static WalletInterface mWalletInterface;

    public CryptoAPIHelper(WalletInterface walletInterface) {
        mWalletInterface = walletInterface;
    }

    public void sendAPIRequestBody(String url, final Web3Enum type, String callType, final String tokenSymbol, String body, TransactionCallback callback) {
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
                    conn.setRequestMethod(callType);
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
                    switch(type) {
                        case TRANSACTION:
                            if (callback != null) callback.onTransactionResult(result);
                          break;
                        case BEP_PRICE:
                        case ERC_PRICE:
                            JSONArray resultArr = new JSONArray(result);
                            for (int i = 0; i != resultArr.length(); i++) {
                                JSONObject item = resultArr.getJSONObject(i);
                                mWalletInterface.onReceivedCustomTokenPrice(tokenSymbol, item.getDouble("priceUsd"));
                            }
                            break;
                        default:
                            break;
                    }
                } catch (Exception e) {}
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void sendAPIRequest(String url, final Web3Enum type, String callType, final String tokenSymbol, final TransactionCallback callback, final ConfigureTrxCallback gasCallback) {
        new AsyncTask<String>() {
            @Override
            protected String doInBackground() {
                HttpURLConnection conn = null;
                StringBuffer response = new StringBuffer();
                try {
                    URL mUrl = new URL(url);

                    conn = (HttpURLConnection) mUrl.openConnection();
                    conn.setDoOutput(false);
                    conn.setConnectTimeout(20000);
                    conn.setDoInput(true);
                    conn.setUseCaches(false);
                    conn.setRequestMethod(callType);
                    conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=UTF-8");

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
                switch(type) {
                    case TRX_GAS_ESTIMATE:
                        if (gasCallback != null) gasCallback.onGasEstimateReceived(result);
                        break;
                    case TOKEN_NONCE:
                        try {
                            JSONObject jsonResult = new JSONObject(result);
                            String nonce = jsonResult.getString("result");

                            if (nonce.startsWith("0x")) {
                                nonce = nonce.substring(2);
                            }

                            mWalletInterface.onReceivedNonce(tokenSymbol, nonce);
                        } catch (Exception e) {}
                        break;
                    case BSC_GAS_CHECK:
                    case ETH_GAS_CHECK:
                        if (gasCallback != null) gasCallback.onCheckGasResult(result);
                        break;
                    case TRANSACTION:
                        if (callback != null) callback.onTransactionResult(result);
                        break;
                    case GET_BEP_TRX:
                    case GET_ETH_TRX:
                        try {
                            JSONObject jsonresult = new JSONObject(result);
                            JSONArray resultArray = jsonresult.getJSONArray("result");
                            ArrayList<TransactionObj> mTransactionArray = new ArrayList<TransactionObj>();

                            for (int i = 0; i != resultArray.length(); i++) {
                                JSONObject trxItem = resultArray.getJSONObject(i);

                                BigDecimal trxAmount = new BigDecimal(trxItem.getString("value"));
                                BigDecimal wei = new BigDecimal(1000000000000000000l);
                                MathContext mc = new MathContext(6);
                                trxAmount = trxAmount.divide(wei, mc);

                                BigDecimal gasPrice = new BigDecimal(trxItem.getString("gasPrice"));
                                BigDecimal gasUsed = new BigDecimal(trxItem.getString("gasUsed"));
                                BigDecimal gas = gasUsed.multiply(gasPrice);
                                gas = gas.divide(wei, mc);

                                // android.icu.math.BigDecimal does not have toPlainString or any other kind of built in method
                                java.math.BigDecimal trxAmountAlt = new java.math.BigDecimal(trxAmount.toString());

                                TransactionObj trxObj = new TransactionObj(trxItem.getString("timeStamp"), trxAmountAlt.toPlainString(), trxItem.getString("hash"), trxItem.getString("to"), trxItem.getString("from"),
                                          trxItem.getString("contractAddress"), tokenSymbol, gas.toString());
                                mTransactionArray.add(trxObj);
                            }

                            mWalletInterface.onReceivedTransactions(mTransactionArray);
                        } catch (Exception ignore) {

                        }
                        break;
                    case ERC_BALANCE:
                    case BEP_BALANCE:
                    case BNB_BALANCE:
                    case ETH_BALANCE:
                        try {
                            JSONObject jsonObjectBalance = new JSONObject(result);
                            String balance = jsonObjectBalance.getString("result");
                            BigDecimal balanceB = new BigDecimal(balance);
                            BigDecimal wei = new BigDecimal(1000000000000000000l);

                            MathContext mc = new MathContext(6);

                            balanceB = balanceB.divide(wei, mc);

                            // divide by 1000000000000000000 because api returns Wei

                            balanceB = balanceB.setScale(6, BigDecimal.ROUND_HALF_DOWN);
                            balance = balanceB.toString();
                            mWalletInterface.onUpdatedBalance(tokenSymbol, balance);
                        } catch (Exception ignore) { }
                        break;
                    case BTC_BALANCE:
                        try {
                            JSONObject jsonObjectBalance = new JSONObject(result);
                            String balance = jsonObjectBalance.getString("final_balance");
                            BigDecimal balanceB = new BigDecimal(balance);
                            BigDecimal wei = new BigDecimal(100000000l);

                            MathContext mc = new MathContext(6);

                            balanceB = balanceB.divide(wei, mc);

                            // divide by 100000000 because api response format

                            balanceB = balanceB.setScale(6, BigDecimal.ROUND_HALF_DOWN);
                            balance = balanceB.toString();

                            mWalletInterface.onUpdatedBalance("BTC", balance);
                        } catch (Exception ignore) { }
                        break;
                    case BEP_PRICE:
                    case ERC_PRICE:
                    case BNB_PRICE:
                    case ETH_PRICE:
                    case BTC_PRICE:
                        try {
                            JSONObject jsonObject = new JSONObject(result);
                            double d = (double)jsonObject.getJSONObject("data").getJSONObject("rates").getDouble("USD");

                            mWalletInterface.onReceiveTokenPrice(tokenSymbol, d);
                        } catch (Exception ignore) { }
                        break;
                    default:
                        break;
                }
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    public void getEthBalance(String address) {
        String url = "https://api.etherscan.io/api?module=account&action=balance&address=" + address
        + "&tag=latest&apikey=" + ETHERSCAN_API_KEY;

        sendAPIRequest(url, Web3Enum.ETH_BALANCE, "POST", "ETH", null, null);
    }
    public void getBNBBalance(String address) {
        String url = "https://api.bscscan.com/api?module=account&action=balance&address=" + address
        + "&apikey=" + BSCSCAN_API_KEY;

        sendAPIRequest(url, Web3Enum.BNB_BALANCE, "POST", "BSC", null, null);
    }

    public void getBEPBalance(String contractAddress, String address, String tokenSymbol) {
        String url = "https://api.bscscan.com/api?module=account&action=tokenbalance&contractaddress="
        + contractAddress
        + "&address=" + address
        + "&tag=latest&apikey=" + BSCSCAN_API_KEY;

        sendAPIRequest(url, Web3Enum.BEP_BALANCE, "POST", tokenSymbol, null, null);
    }

    public void getERCBalance(String contractaddress, String address, String tokenSymbol) {
        String url = "https://api.etherscan.io/api?module=account&action=tokennfttx&contractaddress="
        + contractaddress
        + "&address=" + address
        + "&sort=desc&apikey=" + ETHERSCAN_API_KEY;

        sendAPIRequest(url, Web3Enum.ERC_BALANCE, "POST", tokenSymbol, null, null);
    }

    public void getERCPrice(String tokenSymbol, String contractAddress) {
        // ERC ID = 1
        String url = "https://api.coinbrain.com/public/coin-info";
        String body = "{"
          + "\"1\": ["
          + "\"" + contractAddress + "\""
          + "]"
          + "}";

        sendAPIRequestBody(url, Web3Enum.ERC_PRICE, "POST", tokenSymbol, body, null);
    }

    public void getBEPPrice(String tokenSymbol, String contractAddress) {
        // BEP ID = 56
        String url = "https://api.coinbrain.com/public/coin-info";
        String body = "{"
          + "\"56\": ["
          + "\"" + contractAddress + "\""
          + "]"
          + "}";

        sendAPIRequestBody(url, Web3Enum.BEP_PRICE, "POST", tokenSymbol, body, null);
    }

    public void getBTCBalance(String address) {
        String url = "https://api.blockcypher.com/v1/btc/main/addrs/" + address + "/balance";

        sendAPIRequest(url, Web3Enum.BTC_BALANCE, "GET", "BTC", null, null);
    }

    public void getEthPrice() {
        String url = "https://api.coinbase.com/v2/exchange-rates?currency=ETH";

        sendAPIRequest(url, Web3Enum.ETH_PRICE, "GET", "ETH", null, null);
    }

    public void getBNBPrice() {
        String url = "https://api.coinbase.com/v2/exchange-rates?currency=BNB";

        sendAPIRequest(url, Web3Enum.BNB_PRICE, "GET", "BSC", null, null);
    }

    public void getBTCPrice() {
        String url = "https://api.coinbase.com/v2/exchange-rates?currency=BTC";

        sendAPIRequest(url, Web3Enum.BTC_PRICE, "GET", "BTC", null, null);
    }

    public void getTokenNonce(String tokenType, String address, String tokenSymbol) {
        String baseUrl;
        String apiKey;

        if (tokenType.equals("BEP20")) {
            baseUrl = "https://api.bscscan.com/api";
            apiKey = BSCSCAN_API_KEY;
        } else {
            baseUrl = "https://api.etherscan.io/api";
            apiKey = ETHERSCAN_API_KEY;
        }

        String url = baseUrl
                    + "?module=proxy&action=eth_getTransactionCount&address="
                    + address
                    + "&tag=latest&apikey="
                    + apiKey;

        sendAPIRequest(url, Web3Enum.TOKEN_NONCE, "POST", tokenSymbol, null, null);
    }


    public void getERCTrx(String address, String contractAddress, String ticker) {
        String url = "https://api.etherscan.io/api?module=account&action=tokentx&contractaddress="
        + contractAddress
        + "&address="
        + address
        + "&page=1&offset=20&sort=desc&apikey="
        + ETHERSCAN_API_KEY;

        sendAPIRequest(url, Web3Enum.GET_ETH_TRX, "POST", ticker, null, null);
    }

    public void getBEPTrx(String address, String contractAddress, String ticker) {
        String url = "https://api.bscscan.com/api?module=account&action=tokentx&contractaddress="
        + contractAddress
        + "&address="
        + address
        + "&page=1&offset=20&apikey="
        + BSCSCAN_API_KEY;

        sendAPIRequest(url, Web3Enum.GET_BEP_TRX, "POST", ticker, null, null);
    }

    public void getBSCETHTrx(String address, boolean isEth, String ticker) {
        String url = (isEth ? "https://api.etherscan.io" : "https://api.bscscan.com")
        + "/api?module=account&action=txlist&address="
        + address
        + "&page=1&offset=20&sort=desc&apikey="
        + (isEth ? ETHERSCAN_API_KEY : BSCSCAN_API_KEY);

        sendAPIRequest(url, (isEth ? Web3Enum.GET_ETH_TRX : Web3Enum.GET_BEP_TRX), "POST", ticker, null, null);
    }

    public void getTokenGasEstimate(String tokenType, String data, String recipient, String amount, ConfigureTrxCallback callback) {
        String url = (tokenType.equals("BEP20") ? "https://api.bscscan.com/api" : "https://api.etherscan.io/api")
          + "?module=proxy&action=eth_estimateGas"
          + "&data=" + data
          + "&to=" + recipient
          + "&value=" + amount
          + "&apikey=" + (tokenType.equals("BEP20") ? BSCSCAN_API_KEY : ETHERSCAN_API_KEY);

        sendAPIRequest(url, Web3Enum.TRX_GAS_ESTIMATE, "POST", "", null, callback);
    }

    public void checkGasPrice(String gas, ConfigureTrxCallback callback, CoinType coinType) {
        String url = coinType == CoinType.ETHEREUM ? "https://api.etherscan.io" : "api.bscscan.com"
        + "/api?module=gastracker&action=gasestimate&gasprice="
        + gas
        + "&apikey="
        + (coinType == CoinType.ETHEREUM ? ETHERSCAN_API_KEY : BSCSCAN_API_KEY);

        sendAPIRequest(url, Web3Enum.ETH_GAS_CHECK, "POST", "", null, callback);
    }

    public void sendTransaction(String chainType, String hexString, String tokenSymbol, TransactionCallback callback) {
        String url = "";
        if (chainType.equals("BEP20")) {
            url = "https://go.getblock.io/5855ef8cc8c04dfd808817090d98dd7c";
        } else {
            url = "https://go.getblock.io/56d1ba70818d40a19f936b8a0bffcece";
        }

        String body = "{"
            + "\"jsonrpc\": \"2.0\","
            + "\"method\": \"eth_sendRawTransaction\","
            + "\"params\": ["
            + "\"" + hexString + "\""
            + "],"
            + "\"id\": \"getblock.io\""
            + "}";

        sendAPIRequestBody(url, Web3Enum.TRANSACTION, "POST", tokenSymbol, body, callback);
    }
}
