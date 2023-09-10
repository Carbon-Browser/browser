package org.chromium.chrome.browser.wallet;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.util.ArrayList;
import java.util.List;

class TokenDatabase extends SQLiteOpenHelper {

    // All Static variables
    // Database Version
    // !-- If you change the database schema, must increment the database version --!
    private static final int DATABASE_VERSION = 1;

    // Database Name
    private static final String DATABASE_NAME = "cw_db";

    // Table name
    private final String TABLE_CW = "table_cw";

    // SQLiteDB Table Columns names
    private final String KEY_TOKEN_ICON_URL = "tokenUrl";
    private final String KEY_TOKEN_NAME = "tokenName";
    private final String KEY_TOKEN_USD_VALUE = "tokenUSDValue";
    private final String KEY_TOKEN_BALANCE = "tokenBalance";
    private final String KEY_TOKEN_SYMBOL = "tokenSymbol";
    private final String KEY_TOKEN_CONTRACT_ADDRESS = "tokenContractAddress";
    private final String KEY_TOKEN_CHAIN_NAME = "tokenChainName";

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
                + KEY_TOKEN_CHAIN_NAME + " VARCHAR"
                + ")";
        db.execSQL(CREATE_WALLET_TABLE);
    }

    // Upgrading database
    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // Drop older table if existed
        db.execSQL("DROP TABLE IF EXISTS " + TABLE_CW);

        // Create tables again
        onCreate(db);
    }

    public int getNTokens() {
        String countQuery = "SELECT  * FROM " + TABLE_CW;
        SQLiteDatabase db = this.getReadableDatabase();
        Cursor cursor = db.rawQuery(countQuery, null);
        int count = cursor.getCount();
        cursor.close();
        db.close();

        return count;
    }

    public void addToken(TokenObj tokenObj) {
        SQLiteDatabase db = this.getWritableDatabase();

        String COLUMNS = " (" + KEY_TOKEN_ICON_URL + "," + KEY_TOKEN_NAME + "," + KEY_TOKEN_USD_VALUE + "," + KEY_TOKEN_BALANCE + "," + KEY_TOKEN_SYMBOL + "," + KEY_TOKEN_CONTRACT_ADDRESS + "," + KEY_TOKEN_CHAIN_NAME + ")";
        String VALUES = " VALUES('" + tokenObj.iconUrl + "," + tokenObj.name + "," + tokenObj.usdValue + "," + tokenObj.balance + "," + tokenObj.ticker + "," + tokenObj.chain + "," + tokenObj.chainName + "')";
        String query = "INSERT OR REPLACE INTO "
                + TABLE_CW
                + COLUMNS
                + VALUES;

        db.execSQL(query);
        db.close();
    }

    public List<TokenObj> getTokenList() {
        List<TokenObj> tokenList = new ArrayList<>();
        // Select All Query
        String selectQuery = "SELECT * FROM " + TABLE_CW;

        SQLiteDatabase db = this.getReadableDatabase();
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

                // Add to list
                TokenObj item = new TokenObj(true, -1, iconUrl, name, usdValue, balance, ticker, chain, chainName);

                tokenList.add(item);
            } while (cursor.moveToNext());
        }

        db.close();
        cursor.close();

        // return list
        return tokenList;
    }

    // Remove token
    public void removeToken(String tokenSymbol) {
        SQLiteDatabase db = this.getWritableDatabase();

        db.execSQL("DELETE FROM " + TABLE_CW + " WHERE " + KEY_TOKEN_SYMBOL + "= '" + tokenSymbol + "'");
        db.close();
    }
}
