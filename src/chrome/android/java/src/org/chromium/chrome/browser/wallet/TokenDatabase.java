package org.chromium.chrome.browser.wallet;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import android.icu.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;

import wallet.core.jni.CoinType;

public class TokenDatabase extends SQLiteOpenHelper {

    private ArrayList<TokenObj> mTokenArray = new ArrayList<TokenObj>() {{
        add(new TokenObj(true, "https://carbon.website/carbon/android-resources/wallet/crypto_logo_carbon.png", "Carbon", "0.00", "0", "CSIX", "0x04756126f044634c9a0f0e985e60c88a51acc206", "BEP20", "18"));
        // add(new TokenObj(CoinType.BITCOIN, R.drawable.crypto_logo_bitcoin, "Bitcoin", "0.00", "0", "BTC", ""));
        add(new TokenObj(CoinType.SMARTCHAIN, "https://carbon.website/carbon/android-resources/wallet/crypto_logo_binance.png", "Binance Smart Chain", "0.00", "0", "BSC", "BEP20"));
        add(new TokenObj(CoinType.ETHEREUM, "https://carbon.website/carbon/android-resources/wallet/crypto_logo_ethereum.png", "Ethereum", "0.00", "0", "ETH", "ERC20"));
        add(new TokenObj(true, "https://carbon.website/carbon/android-resources/wallet/crypto_logo_usdtbep20.png", "Tether USD", "0.00", "0", "USDT", "0x55d398326f99059fF775485246999027B3197955", "BEP20", "18"));
    }};

    // All Static variables
    // Database Version
    // !-- If you change the database schema, must increment the database version --!
    private static final int DATABASE_VERSION = 18;

    // Database Name
    private static final String DATABASE_NAME = "cw_db";

    // Table name
    private final String TABLE_CW = "table_cw";
    private final String TABLE_TRX = "table_trx";

    // SQLiteDB Table Columns names
    private final String KEY_TOKEN_ICON_URL = "tokenUrl";
    private final String KEY_TOKEN_NAME = "tokenName";
    private final String KEY_TOKEN_USD_VALUE = "tokenUSDValue";
    private final String KEY_TOKEN_BALANCE = "tokenBalance";
    private final String KEY_TOKEN_SYMBOL = "tokenSymbol";
    private final String KEY_TOKEN_CONTRACT_ADDRESS = "tokenContractAddress";
    private final String KEY_TOKEN_CHAIN_NAME = "tokenChainName";
    private final String KEY_HIGHEST_LATEST_NONCE = "tokenNonce";
    private final String KEY_DECIMALS = "tokenDecimals";

    private final String KEY_TIMESTAMP = "trxTimestamp";
    private final String KEY_AMOUNT = "trxAmount";
    private final String KEY_HASH = "trxHash";
    private final String KEY_RECIPIENT_ADDRESS = "trxRecipient";
    private final String KEY_SENDER_ADDRESS = "trxSender";
    private final String KEY_CONTRACT_ADDRESS = "trxContractAddress";
    private final String KEY_GAS = "trxGas";

    private static TokenDatabase sInstance;

    public static synchronized TokenDatabase getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new TokenDatabase(context.getApplicationContext());
        }
        return sInstance;
    }

    public TokenDatabase(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    // Creating Tables
    @Override
    public void onCreate(SQLiteDatabase db) {
        String CREATE_WALLET_TABLE = "CREATE TABLE " + TABLE_CW + "("
                + KEY_TOKEN_ICON_URL + " VARCHAR,"
                + KEY_TOKEN_NAME + " VARCHAR,"
                + KEY_TOKEN_USD_VALUE + " VARCHAR,"
                + KEY_TOKEN_BALANCE + " VARCHAR,"
                + KEY_TOKEN_SYMBOL + " VARCHAR PRIMARY KEY,"
                + KEY_TOKEN_CONTRACT_ADDRESS + " VARCHAR,"
                + KEY_TOKEN_CHAIN_NAME + " VARCHAR,"
                + KEY_HIGHEST_LATEST_NONCE + " VARCHAR,"
                + KEY_DECIMALS + " VARCHAR"
                + ")";
        db.execSQL(CREATE_WALLET_TABLE);

        String CREATE_TRANSACTION_TABLE = "CREATE TABLE " + TABLE_TRX + "("
                + KEY_HASH + " VARCHAR PRIMARY KEY,"
                + KEY_TIMESTAMP + " VARCHAR,"
                + KEY_AMOUNT + " VARCHAR,"
                + KEY_RECIPIENT_ADDRESS + " VARCHAR,"
                + KEY_SENDER_ADDRESS + " VARCHAR,"
                + KEY_CONTRACT_ADDRESS + " VARCHAR,"
                + KEY_TOKEN_SYMBOL + " VARCHAR,"
                + KEY_GAS + " VARCHAR"
                + ")";
        db.execSQL(CREATE_TRANSACTION_TABLE);

        for (int i = 0; i != mTokenArray.size(); i++) {
            TokenObj tokenObj = mTokenArray.get(i);

            String COLUMNS = " (" + KEY_TOKEN_ICON_URL + "," + KEY_TOKEN_NAME + "," + KEY_TOKEN_USD_VALUE + "," + KEY_TOKEN_BALANCE + "," + KEY_TOKEN_SYMBOL + "," + KEY_TOKEN_CONTRACT_ADDRESS + "," + KEY_TOKEN_CHAIN_NAME + ","+ KEY_HIGHEST_LATEST_NONCE + "," + KEY_DECIMALS + ")";
            String VALUES = " VALUES('" + tokenObj.iconUrl + "','" + tokenObj.name + "','" + tokenObj.usdValue + "','" + tokenObj.balance + "','" + tokenObj.ticker + "','" + tokenObj.chain + "','" +tokenObj.chainName + "','" + "0" + "','" + tokenObj.decimals + "')";
            String query = "INSERT INTO "
                    + TABLE_CW
                    + COLUMNS
                    + VALUES;

            db.execSQL(query);
        }
    }

    @Override
    public void onConfigure(SQLiteDatabase db) {
        super.onConfigure(db);

        if (!db.isWriteAheadLoggingEnabled()) {
            db.enableWriteAheadLogging();
        }
    }

    // Upgrading database
    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // try deleting the database, re-writing the tokens with the new decimal field.. if error occurs just do total wipe

        try {
          ArrayList<TokenObj> mTempTokenArray = internalGetTokenList(db);

          // Drop older table if existed
          db.execSQL("DROP TABLE IF EXISTS " + TABLE_CW);

          // Create tables again
          String CREATE_WALLET_TABLE = "CREATE TABLE " + TABLE_CW + "("
                  + KEY_TOKEN_ICON_URL + " VARCHAR,"
                  + KEY_TOKEN_NAME + " VARCHAR,"
                  + KEY_TOKEN_USD_VALUE + " VARCHAR,"
                  + KEY_TOKEN_BALANCE + " VARCHAR,"
                  + KEY_TOKEN_SYMBOL + " VARCHAR PRIMARY KEY,"
                  + KEY_TOKEN_CONTRACT_ADDRESS + " VARCHAR,"
                  + KEY_TOKEN_CHAIN_NAME + " VARCHAR,"
                  + KEY_HIGHEST_LATEST_NONCE + " VARCHAR,"
                  + KEY_DECIMALS + " VARCHAR"
                  + ")";
          db.execSQL(CREATE_WALLET_TABLE);

          for (int i = 0; i != mTempTokenArray.size(); i++) {
              TokenObj tokenObj = mTempTokenArray.get(i);

              String COLUMNS = " (" + KEY_TOKEN_ICON_URL + "," + KEY_TOKEN_NAME + "," + KEY_TOKEN_USD_VALUE + "," + KEY_TOKEN_BALANCE + "," + KEY_TOKEN_SYMBOL + "," + KEY_TOKEN_CONTRACT_ADDRESS + "," + KEY_TOKEN_CHAIN_NAME + ","+ KEY_HIGHEST_LATEST_NONCE + "," + KEY_DECIMALS + ")";
              String VALUES = " VALUES('" + tokenObj.iconUrl + "','" + tokenObj.name + "','" + tokenObj.usdValue + "','" + tokenObj.balance + "','" + tokenObj.ticker + "','" + tokenObj.chain + "','" +tokenObj.chainName + "','" + "0" + "','" + tokenObj.decimals + "')";
              String query = "INSERT INTO "
                      + TABLE_CW
                      + COLUMNS
                      + VALUES;

              db.execSQL(query);
          }
        } catch (Exception e) {
          db.execSQL("DROP TABLE IF EXISTS " + TABLE_CW);

          db.execSQL("DROP TABLE IF EXISTS " + TABLE_TRX);

          onCreate(db);
        }
    }

    public void addAndReplaceTrx(ArrayList<TransactionObj> trxArray) {
        if (trxArray.size() == 0) return;

        String tokenSymbol = trxArray.get(0).tokenSymbol;

        SQLiteDatabase db = this.getWritableDatabase();
        String[] whereArgs = new String[] { tokenSymbol };

        db.delete(TABLE_TRX, KEY_TOKEN_SYMBOL+"=?", whereArgs);

        for (int i = 0; i != trxArray.size(); i++) {
            TransactionObj trx = trxArray.get(i);

            String COLUMNS = " (" + KEY_HASH + "," + KEY_TIMESTAMP + "," + KEY_AMOUNT + "," + KEY_RECIPIENT_ADDRESS + "," + KEY_SENDER_ADDRESS + "," + KEY_CONTRACT_ADDRESS + "," + KEY_TOKEN_SYMBOL + "," + KEY_GAS + ")";
            String VALUES = " VALUES('" + trx.hash + "','" + trx.timestamp + "','" + trx.amount + "','" + trx.recipientAddress + "','" + trx.senderAddress + "','" + trx.contractAddress + "','" +trx.tokenSymbol + "','" + trx.gas + "')";
            String query = "INSERT OR REPLACE INTO "
                    + TABLE_TRX
                    + COLUMNS
                    + VALUES;

            db.execSQL(query);
        }
        db.close();
    }

    public ArrayList<TransactionObj> getAllTrx(String bscAddress, String ethAddress) {

        ArrayList<TransactionObj> transactionList = new ArrayList<TransactionObj>();

        synchronized (sInstance) {
            SQLiteDatabase db = getReadableDatabase();

            try {
                // Select All Query
                String selectQuery = "SELECT * FROM " + TABLE_TRX + " ORDER BY " + KEY_TIMESTAMP + " DESC";

                Cursor cursor = db.rawQuery(selectQuery, null);

                // looping through all rows and adding to list
                if (cursor.moveToFirst()) {
                    do {
                        String hash = cursor.getString(0);
                        String timestamp = cursor.getString(1);
                        String amount = cursor.getString(2);
                        String recipientAddress = cursor.getString(3);
                        String senderAddress = cursor.getString(4);
                        String contractAddress = cursor.getString(5);
                        String tokenSymbol = cursor.getString(6);
                        String gas = cursor.getString(7);

                        String tokenIconUrl = "";
                        String tokenPriceUsd = "";
                        String chainType = "";
                        String coinName = "";

                        // Get token icon url
                        String tokenIconUrlQuery = "SELECT * FROM " + TABLE_CW + " WHERE " + KEY_TOKEN_SYMBOL + " = '" + tokenSymbol + "'";
                        Cursor innerCursor = db.rawQuery(tokenIconUrlQuery, null);

                        // looping through all rows and calculate
                        if (innerCursor.moveToFirst()) {
                            do {
                                tokenIconUrl = innerCursor.getString(0);
                                coinName = innerCursor.getString(1);
                                tokenPriceUsd = innerCursor.getString(2);
                                chainType = innerCursor.getString(6);
                            } while (innerCursor.moveToNext());
                        }
                        innerCursor.close();

                        boolean isSender = (bscAddress.equalsIgnoreCase(senderAddress) || ethAddress.equalsIgnoreCase(senderAddress));

                        TransactionObj item = new TransactionObj(timestamp, amount, hash, recipientAddress, senderAddress, contractAddress, tokenSymbol, isSender,
                                tokenIconUrl, chainType, tokenPriceUsd, gas, coinName);

                        // Add title item to list
                        transactionList.add(item);
                    } while (cursor.moveToNext());
                }

                db.close();
                cursor.close();
            } catch (Exception ignore) { }
        }

        return transactionList;
    }

    public String getWalletValue() {
        BigDecimal total = new BigDecimal(0);

        synchronized (sInstance) {
            SQLiteDatabase db = getReadableDatabase();

            try {
                String selectQuery = "SELECT * FROM " + TABLE_CW;

                Cursor cursor = db.rawQuery(selectQuery, null);

                // looping through all rows and calculate
                if (cursor.moveToFirst()) {
                    do {
                        BigDecimal usdValue = new BigDecimal(cursor.getString(2));
                        BigDecimal balance = new BigDecimal(cursor.getString(3));

                        total = total.add((balance.multiply(usdValue)));
                    } while (cursor.moveToNext());
                }

                db.close();
                cursor.close();
            } catch (Exception ignore) { }
        }

        return total.setScale(2, BigDecimal.ROUND_HALF_DOWN)+"";
    }

    public int getNTokens() {
        int count = 0;

        synchronized (sInstance) {
            SQLiteDatabase db = getReadableDatabase();

            try {
                String countQuery = "SELECT  * FROM " + TABLE_CW;
                Cursor cursor = db.rawQuery(countQuery, null);
                count = cursor.getCount();

                db.close();
                cursor.close();
            } catch (Exception ignore) { }
        }

        return count;
    }

    public void addToken(TokenObj tokenObj) {
        synchronized (sInstance) {
            SQLiteDatabase db = getWritableDatabase();

            try {
                String COLUMNS = " (" + KEY_TOKEN_ICON_URL + "," + KEY_TOKEN_NAME + "," + KEY_TOKEN_USD_VALUE + "," + KEY_TOKEN_BALANCE + "," + KEY_TOKEN_SYMBOL + "," + KEY_TOKEN_CONTRACT_ADDRESS + "," + KEY_TOKEN_CHAIN_NAME + "," + KEY_HIGHEST_LATEST_NONCE + "," + KEY_DECIMALS + ")";
                String VALUES = " VALUES('" + tokenObj.iconUrl + "','" + tokenObj.name + "','" + tokenObj.usdValue + "','" + tokenObj.balance + "','" + tokenObj.ticker + "','" + tokenObj.chain + "','" + tokenObj.chainName + "','" + "0" + "','" + tokenObj.decimals + "')";
                String query = "INSERT OR REPLACE INTO "
                        + TABLE_CW
                        + COLUMNS
                        + VALUES;

                db.execSQL(query);
                db.close();
            } catch (Exception ignore) {}
        }
    }

    public ArrayList<TokenObj> getTokenList() {
        ArrayList<TokenObj> tokenList = new ArrayList<>();

        synchronized (sInstance) {
            SQLiteDatabase db = getReadableDatabase();
            try {
                // Select All Query
                String selectQuery = "SELECT * FROM " + TABLE_CW;

                Cursor cursor = db.rawQuery(selectQuery, null);

                // looping through all rows and adding to list
                if (cursor.moveToFirst()) {
                    do {
                        String iconUrl = cursor.getString(0);
                        String name = cursor.getString(1);
                        String usdValue = cursor.getString(2);
                        String balance = cursor.getString(3);
                        String ticker = cursor.getString(4);
                        String chain = cursor.getString(5);
                        String chainName = cursor.getString(6);

                        String decimals = cursor.getString(8);

                        boolean isCustom = !chain.equals("null");

                        TokenObj item = new TokenObj(isCustom, iconUrl, name, usdValue, balance, ticker, chain, chainName, decimals);

                        tokenList.add(item);
                    } while (cursor.moveToNext());
                }

                db.close();
                cursor.close();
            } catch (Exception ignore) {

            }
        }

        // return list
        return tokenList;
    }

    // used when upgrading the database only (decimals is hardcoded since thats a new field we're adding)
    public ArrayList<TokenObj> internalGetTokenList(SQLiteDatabase db) {
        ArrayList<TokenObj> tokenList = new ArrayList<>();

        synchronized (sInstance) {
            try {
                // Select All Query
                String selectQuery = "SELECT * FROM " + TABLE_CW;

                Cursor cursor = db.rawQuery(selectQuery, null);

                // looping through all rows and adding to list
                if (cursor.moveToFirst()) {
                    do {
                        String iconUrl = cursor.getString(0);
                        String name = cursor.getString(1);
                        String usdValue = cursor.getString(2);
                        String balance = cursor.getString(3);
                        String ticker = cursor.getString(4);
                        String chain = cursor.getString(5);
                        String chainName = cursor.getString(6);

                        boolean isCustom = !chain.equals("null");

                        TokenObj item = new TokenObj(isCustom, iconUrl, name, usdValue, balance, ticker, chain, chainName, "18");

                        tokenList.add(item);
                    } while (cursor.moveToNext());
                }

                cursor.close();
            } catch (Exception ignore) {

            }
        }

        // return list
        return tokenList;
    }

    // update token price
    public void updateTokenPrice(String tokenSymbol, double price) {
        synchronized (sInstance) {
            SQLiteDatabase db = getWritableDatabase();

            try {
                // db.beginTransaction();

                db.execSQL("UPDATE " + TABLE_CW + " SET " + KEY_TOKEN_USD_VALUE + "= '" + price + "' WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'");
                db.close();
                // db.setTransactionSuccessful();
            } catch(Exception e) {

            } finally {
                // db.endTransaction();
            }
        }
    }

    public void updateTokenBalance(String tokenSymbol, String balance) {
        synchronized (sInstance) {
            SQLiteDatabase db = getWritableDatabase();

            try {
                // db.beginTransaction();

                db.execSQL("UPDATE " + TABLE_CW + " SET " + KEY_TOKEN_BALANCE + "= '" + balance + "' WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'");
                db.close();
                // db.setTransactionSuccessful();
            } catch(Exception e) {

            } finally {
                // db.endTransaction();
            }
        }
    }

    // Remove token
    public void removeToken(String tokenSymbol) {
        synchronized (sInstance) {
            SQLiteDatabase db = getWritableDatabase();

            db.execSQL("DELETE FROM " + TABLE_CW + " WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'");
            db.close();
        }
    }

    public String getTokenNonce(String tokenSymbol) {
      synchronized (sInstance) {
          SQLiteDatabase db = getReadableDatabase();

          String nonce = "";

          try {
              String selectQuery = "SELECT * FROM " + TABLE_CW + " WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'";
              Cursor cursor = db.rawQuery(selectQuery, null);
              if (cursor.moveToFirst()) {
                  nonce = cursor.getString(7);
              }
          } catch (Exception e) {

          }
          db.close();

          return nonce;
      }
    }

    public String getTokenUSDValue(boolean isEth) {
      synchronized (sInstance) {
          SQLiteDatabase db = getReadableDatabase();

          String tokenSymbol = isEth ? "ETH" : "BSC";

          String usdValue = "";

          try {
              String selectQuery = "SELECT * FROM " + TABLE_CW + " WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'";
              Cursor cursor = db.rawQuery(selectQuery, null);
              if (cursor.moveToFirst()) {
                  usdValue = cursor.getString(2);
              }
          } catch (Exception e) {

          }
          db.close();

          return usdValue;
      }
    }

    public void setTokenNonce(String tokenSymbol, String nonce) {
        synchronized (sInstance) {
            SQLiteDatabase db = getWritableDatabase();

            try {
              db.execSQL("UPDATE " + TABLE_CW + " SET " + KEY_HIGHEST_LATEST_NONCE + "= '" + nonce + "' WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'");
            } catch (Exception e) {

            }
            db.close();
        }
    }

    public void clearDatabase() {
        synchronized (sInstance) {
            SQLiteDatabase db = getWritableDatabase();

            db.execSQL("DROP TABLE IF EXISTS " + TABLE_CW);

            db.execSQL("DROP TABLE IF EXISTS " + TABLE_TRX);

            onCreate(db);

            db.close();
        }
    }
}
