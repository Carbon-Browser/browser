package org.chromium.chrome.browser.wallet;

import org.chromium.chrome.browser.ChromeBaseAppCompatActivity;
import android.os.Bundle;
import org.chromium.chrome.R;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;

import androidx.annotation.Nullable;
import android.graphics.drawable.Drawable;
import android.widget.TextView;
import android.content.SharedPreferences;
import org.chromium.base.ContextUtils;
import android.widget.LinearLayout;

import wallet.core.jni.HDWallet;
import wallet.core.jni.PrivateKey;
import wallet.core.java.AnySigner;
import wallet.core.jni.proto.Ethereum;
import wallet.core.jni.BitcoinScript;
import wallet.core.jni.BitcoinSigHashType;
import wallet.core.jni.proto.Bitcoin;
import wallet.core.jni.proto.Common;
import wallet.core.jni.StoredKey;
//
import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import java.math.BigInteger;
import java.math.BigDecimal;
import wallet.core.java.AnySigner;
import wallet.core.jni.CoinType;
import wallet.core.jni.HDWallet;
import wallet.core.jni.PrivateKey;
import wallet.core.jni.proto.Ethereum;
//

import org.chromium.chrome.browser.pininput.data.EncryptSharedPreferences;
import org.chromium.chrome.browser.pininput.PinCodeFragment;
import android.content.SharedPreferences;
import android.app.Activity;
import org.chromium.ui.widget.Toast;

import java.util.Date;
import android.icu.util.Calendar;
import java.util.Locale;
import java.util.ArrayList;
import android.icu.text.SimpleDateFormat;

public class WalletActivity extends ChromeBaseAppCompatActivity implements WalletInterface {

    public static String PIN_CODE_KEY = "PIN_CODE_KEY";

    private PinCodeInterface mPinCodeInterface;

    private HDWallet mWallet;

    private SharedPreferences mSharedPreferences;

    private String mPinCode = "";

    private static WalletDatabaseInterface mWalletDatabaseInterface;

    private String mMnemonicString;

    private TrxObj mPendingTrx;

    public WalletActivity() {
        fetchLatestPrices();
    }

    @Override
    public ArrayList<TransactionObj> getTrxList() {
        return TokenDatabase.getInstance(this).getAllTrx(mWallet.getAddressForCoin(CoinType.SMARTCHAIN), mWallet.getAddressForCoin(CoinType.ETHEREUM));
    }

    @Override
    public ArrayList<TokenObj> getTokenList() {
        return TokenDatabase.getInstance(this).getTokenList();
    }

    @Override
    public void onReceivedTransactions(ArrayList<TransactionObj> list) {
        // send to database
        TokenDatabase.getInstance(this).addAndReplaceTrx(list);

        if (mWalletDatabaseInterface != null) mWalletDatabaseInterface.onTransactionsReceived();
    }

    private void getTokenTrx() {
        CryptoAPIHelper mCryptoAPIHelper = new CryptoAPIHelper(this);

        ArrayList<TokenObj> mTokenArray = getTokenList();
        try {
            for (int i = 0; i != mTokenArray.size(); i++) {
                TokenObj token = mTokenArray.get(i);

                if (token.isCustomType) {
                    if (token.chainName.equals("BEP20")) {
                        mCryptoAPIHelper.getBEPTrx(mWallet.getAddressForCoin(CoinType.SMARTCHAIN), token.chain, token.ticker);
                    } else {
                        mCryptoAPIHelper.getERCTrx(mWallet.getAddressForCoin(CoinType.ETHEREUM), token.chain, token.ticker);
                    }
                } else if (token.ticker.equals("ETH")) {
                    mCryptoAPIHelper.getBSCETHTrx(mWallet.getAddressForCoin(CoinType.ETHEREUM), true, token.ticker);
                } else if (token.ticker.equals("BSC")) {
                    mCryptoAPIHelper.getBSCETHTrx(mWallet.getAddressForCoin(CoinType.SMARTCHAIN), false, token.ticker);
                } else {
                    // btc
                }
            }
        } catch (Exception ignore) {

        }
    }

    private void fetchLatestPrices() {
        // fetch balance for all crpytos
        CryptoAPIHelper mCryptoAPIHelper = new CryptoAPIHelper(this);
        mCryptoAPIHelper.getEthPrice();
        // mCryptoAPIHelper.getBTCPrice();
        mCryptoAPIHelper.getBNBPrice();
        mCryptoAPIHelper.getBEPPrice("CSIX", "0x04756126f044634c9a0f0e985e60c88a51acc206");
    }

    private void fetchBalance() {
        CryptoAPIHelper mCryptoAPIHelper = new CryptoAPIHelper(this);
        mCryptoAPIHelper.getEthBalance(mWallet.getAddressForCoin(CoinType.ETHEREUM));
        mCryptoAPIHelper.getBNBBalance(mWallet.getAddressForCoin(CoinType.SMARTCHAIN)); // BSC uses eth smart contracts
        // mCryptoAPIHelper.getBTCBalance(mWallet.getAddressForCoin(CoinType.BITCOIN));
        mCryptoAPIHelper.getBEPBalance("0x04756126f044634c9a0f0e985e60c88a51acc206", mWallet.getAddressForCoin(CoinType.SMARTCHAIN), "CSIX");
    }

    @Override
    public void onReceiveTokenPrice(String tokenSymbol, double price) {
        TokenDatabase.getInstance(this).updateTokenPrice(tokenSymbol, price);

        if (mWalletDatabaseInterface != null) mWalletDatabaseInterface.onTokenInfoReceived();
    }

    @Override
    public void onReceivedCustomTokenPrice(String tokenSymbol, double priceUsd) {
        TokenDatabase.getInstance(this).updateTokenPrice(tokenSymbol, priceUsd);

        if (mWalletDatabaseInterface != null) mWalletDatabaseInterface.onTokenInfoReceived();
    }

    @Override
    public void onUpdatedBalance(String tokenSymbol, String balance) {
        TokenDatabase.getInstance(this).updateTokenBalance(tokenSymbol, balance);

        if (mWalletDatabaseInterface != null) mWalletDatabaseInterface.onTokenInfoReceived();
    }

    @Override
    public String getWalletValue() {
        return TokenDatabase.getInstance(this).getWalletValue();
    }

    @Override
    public void checkGasPrice(String gas, ConfigureTrxCallback callback, int coinType) {
        CryptoAPIHelper mCryptoAPIHelper = new CryptoAPIHelper(this);
        mCryptoAPIHelper.checkGasPrice(gas, callback, CoinType.createFromValue(coinType));
    }

    @Override
    public void sendTransaction(TransactionCallback callback) {
        try {
            CryptoAPIHelper mCryptoAPIHelper = new CryptoAPIHelper(this);

            CoinType coinType = CoinType.createFromValue(mPendingTrx.coinType);
            boolean isTokenCustomType = !mPendingTrx.contractAddress.equals("");

//             if (coinType == CoinType.BITCOIN) {
//
//             } else {
//                 final PrivateKey secretPrivateKey = mWallet.getKeyForCoin(coinType);
//                 final String receiverAddress = mPendingTrx.recipient;
//
//                 final BigDecimal wei = new BigDecimal("1000000000000000000");
//                 BigDecimal gasDecimal = new BigDecimal(mPendingTrx.gas);
//                 gasDecimal = gasDecimal.multiply(wei);
//                 BigInteger gas = gasDecimal.toBigInteger();
//
//                 BigDecimal amountDecimal = new BigDecimal(mPendingTrx.tokenAmount);
//                 amountDecimal = amountDecimal.multiply(wei);
//                 BigInteger amount = amountDecimal.toBigInteger();
//
//                 String gasString = gas.toString(16);
//                 String amountString = amount.toString(16);
//
//                 Ethereum.SigningInput.Builder signerInputBuilder = Ethereum.SigningInput.newBuilder();
//
//                 signerInputBuilder.setChainId(ByteString.copyFrom((new BigInteger("01")).toByteArray()));
//                 signerInputBuilder.setGasPrice(this.toByteString(new BigInteger(gasString, 16)));
//                 signerInputBuilder.setGasLimit(this.toByteString(new BigInteger("5208", 16)));
//                 signerInputBuilder.setPrivateKey(ByteString.copyFrom(secretPrivateKey.data()));
//
//                 if ((coinType == CoinType.ETHEREUM  || coinType == CoinType.SMARTCHAIN) && mPendingTrx.contractAddress.equals("")) {
//                     // ethereum mainnet
//                     signerInputBuilder.setToAddress(receiverAddress);
//
//                     Ethereum.Transaction.Builder ethTrxBuilder = Ethereum.Transaction.newBuilder();
//
//                     Ethereum.Transaction.Transfer.Builder ethTrxTransferBuilder = Ethereum.Transaction.Transfer.newBuilder();
//                     ethTrxTransferBuilder.setAmount(this.toByteString(new BigInteger(amountString, 16)));
//
//                     ethTrxBuilder.setTransfer(ethTrxTransferBuilder.build());
//
//                     signerInputBuilder.setTransaction(ethTrxBuilder.build());
//
//                     Ethereum.SigningInput signerInput = signerInputBuilder.build();
//
//                     if (mPendingTrx.chainType.equals("BEP20")) {
//                         Ethereum.SigningOutput signerOutput = (Ethereum.SigningOutput)AnySigner.sign((Message)signerInput, CoinType.SMARTCHAIN, Ethereum.SigningOutput.parser());
//                         mCryptoAPIHelper.sendAPIRequest("https://api.bscscan.com/api?module=proxy&action=eth_sendRawTransaction&hex="+ toHexString(signerOutput.getEncoded().toByteArray(), false) + "&apikey=UVMB2DE897HHNS5UX5U5X4U438N54F73IG",
//                             Web3Enum.TRANSACTION, "POST", mPendingTrx.tokenTicker, callback, null);
//                     } else {
//                         Ethereum.SigningOutput signerOutput = (Ethereum.SigningOutput)AnySigner.sign((Message)signerInput, CoinType.ETHEREUM, Ethereum.SigningOutput.parser());
//                         mCryptoAPIHelper.sendAPIRequest("https://api.etherscan.io/api?module=proxy&action=eth_sendRawTransaction&hex="+ toHexString(signerOutput.getEncoded().toByteArray(), false) + "&apikey=EGB6VW4Y4CNTQD3FP4TGA3MAWS3M9TTUKZ",
//                             Web3Enum.TRANSACTION, "POST", mPendingTrx.tokenTicker, callback, null);
//                     }
//                 } else {
//                     // BEP/ERC20
//                     signerInputBuilder.setToAddress(mPendingTrx.contractAddress); // contract address
//
//                     if (mPendingTrx.chainType.equals("BEP20")) {
//                         // bep20
//                         Ethereum.Transaction.Builder ethTrxBuilder = Ethereum.Transaction.ContractGeneric.newBuilder();
//
//                         Ethereum.Transaction.ContractGeneric.Transfer.Builder ethTrxTransferBuilder = Ethereum.Transaction.ContractGeneric.newBuilder();
//                         ethTrxTransferBuilder.setAmount(this.toByteString(new BigInteger(amountString, 16)));
//                         ethTrxTransferBuilder.setTo(receiverAddress);
//
//                         ethTrxBuilder.setErc20Transfer(ethTrxTransferBuilder.build());
//
//                         signerInputBuilder.setTransaction(ethTrxBuilder.build());
//
//                         Ethereum.SigningInput signerInput = signerInputBuilder.build();
//
//                         Ethereum.SigningOutput signerOutput = (Ethereum.SigningOutput)AnySigner.sign((Message)signerInput, CoinType.SMARTCHAIN, Ethereum.SigningOutput.parser());
//                         mCryptoAPIHelper.sendAPIRequest("https://api.bscscan.com/api?module=proxy&action=eth_sendRawTransaction&hex="+ toHexString(signerOutput.getEncoded().toByteArray(), false) + "&apikey=UVMB2DE897HHNS5UX5U5X4U438N54F73IG",
//                             Web3Enum.TRANSACTION, "POST", mPendingTrx.tokenTicker, callback, null);
//                     } else {
//                         // erc20
//                         Ethereum.Transaction.Builder ethTrxBuilder = Ethereum.Transaction.newBuilder();
//
//                         Ethereum.Transaction.ERC20Transfer.Builder ethTrxTransferBuilder = Ethereum.Transaction.ERC20Transfer.newBuilder();
//                         ethTrxTransferBuilder.setAmount(this.toByteString(new BigInteger(amountString, 16)));
//                         ethTrxTransferBuilder.setTo(receiverAddress);
//
//                         ethTrxBuilder.setErc20Transfer(ethTrxTransferBuilder.build());
//
//                         signerInputBuilder.setTransaction(ethTrxBuilder.build());
//
//                         Ethereum.SigningInput signerInput = signerInputBuilder.build();
//
//                         Ethereum.SigningOutput signerOutput = (Ethereum.SigningOutput)AnySigner.sign((Message)signerInput, CoinType.ETHEREUM, Ethereum.SigningOutput.parser());
//                         mCryptoAPIHelper.sendAPIRequest("https://api.etherscan.io/api?module=proxy&action=eth_sendRawTransaction&hex="+ toHexString(signerOutput.getEncoded().toByteArray(), false) + "&apikey=EGB6VW4Y4CNTQD3FP4TGA3MAWS3M9TTUKZ",
//                             Web3Enum.TRANSACTION, "POST", mPendingTrx.tokenTicker, callback, null);
//                     }
//                 }
            // }
        } catch (Exception e) {

        }
    }

    @Override
    public void onBackPressed() {
        try {
            if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
                getSupportFragmentManager().popBackStackImmediate();
            } else {
                setResult(Activity.RESULT_OK, null);
                finish();
            }
        } catch (Exception ignore) {}
    }

    @Override
    public String[] getMnemonic() {
        return mWallet.mnemonic().split(" ");
    }

    @Override
    public void onTabChanged(String tabTitle) {

    }

    @Override
    public void onNavigateImportWallet() {
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.wallet_fragment_container_view, WalletImport.class, null)
                .commit();
    }

    @Override
    public void onMnemonicImported(String mnemonic) {
        mMnemonicString = mnemonic;
        onNavigateCreatePin();
    }

    @Override
    public void onNavigateCreateWallet() {
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.wallet_fragment_container_view, WalletCreate.class, null)
                .commit();
    }

    @Override
    public void onNavigateMain() {
        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            getSupportFragmentManager().popBackStack();
        }

        MainTabFragment mMainTabFragment = new MainTabFragment();
        mWalletDatabaseInterface = mMainTabFragment;

        getSupportFragmentManager().beginTransaction()
                .addToBackStack(null)
                .replace(R.id.wallet_fragment_container_view, mMainTabFragment, null)
                .commit();
    }

    @Override
    public void onNavigateReceive(TokenObj tokenObj) {
        Bundle walletReceiveBundle = new Bundle();

        CoinType type = tokenObj.chainName.equals("BEP20") ? CoinType.SMARTCHAIN : CoinType.ETHEREUM;
        walletReceiveBundle.putString("COIN_NAME_KEY", tokenObj.ticker);
        walletReceiveBundle.putString("COIN_ADDRESS_KEY", getAddressForCoin(type));

        WalletReceive mWalletReceiveFragment = new WalletReceive();
        mWalletReceiveFragment.setArguments(walletReceiveBundle);

        getSupportFragmentManager().beginTransaction()
                .addToBackStack(null)
                .replace(R.id.wallet_fragment_container_view, mWalletReceiveFragment, null)
                .commit();
    }

    private String getAddressForCoin(CoinType coinType) {
        return mWallet.getAddressForCoin(coinType);
    }

    @Override
    public void onNavigateSend(TokenObj tokenObj) {
        Bundle walletSendBundle = new Bundle();
        walletSendBundle.putString("COIN_NAME_KEY", tokenObj.name);
        walletSendBundle.putString("COIN_ICON_URL_KEY", tokenObj.iconUrl);
        walletSendBundle.putString("COIN_BALANCE_KEY", tokenObj.balance);
        walletSendBundle.putString("COIN_USD_VALUE", tokenObj.usdValue);
        walletSendBundle.putString("COIN_TICKER_KEY", tokenObj.ticker);
        int coinType = tokenObj.chainName.equals("BEP20") ? CoinType.SMARTCHAIN.value() : CoinType.ETHEREUM.value();
        walletSendBundle.putInt("COIN_TYPE_KEY", coinType);
        walletSendBundle.putString("COIN_CONTRACT_ADDRESS_KEY", tokenObj.chain);
        walletSendBundle.putString("COIN_CHAIN_TYPE_KEY", tokenObj.chainName);

        WalletSend mWalletSendFragment = new WalletSend();
        mWalletSendFragment.setArguments(walletSendBundle);

        getSupportFragmentManager().beginTransaction()
                .addToBackStack(null)
                .replace(R.id.wallet_fragment_container_view, mWalletSendFragment, null)
                .commit();
    }

    @Override
    public void onNavigateCreatePin() {
        PinCodeFragment mPinCodeFragment = new PinCodeFragment();
        mPinCodeInterface = (PinCodeInterface)mPinCodeFragment;

        getSupportFragmentManager().beginTransaction()
                .replace(R.id.wallet_fragment_container_view, PinCodeFragment.class, null)
                .commit();
    }

    public void onPassCodeButtonPressed(View v) {
        mPinCodeInterface.onPinCodePressed(((Button)v).getText().toString());
    }

    public void onDeleteCodePressed(View v) {
        mPinCodeInterface.onDeleteCodePressed();
    }

    @Override
    public void onPinCreated(String pinCode) {
        // MAKE HD WALLET AND GO TO MAIN SCREEN
        mPinCode = pinCode;
        if (mMnemonicString == null) {
            createWallet();
        } else {
            importWallet();
        }
    }

    @Override
    public void onPinVerified(String pinCode) {
        // GET HD WALLET AND GO TO MAIN SCREEN
        mPinCode = pinCode;
        fetchWallet();
    }

    @Override
    public void onDeleteWallet() {
        if (mSharedPreferences == null) mSharedPreferences = new EncryptSharedPreferences(this).getSharedPreferences();
        mMnemonicString = null;
        mSharedPreferences.edit().putString("MNEMONIC_KEY", null).commit();
        mSharedPreferences.edit().putString("PIN_CODE_KEY", null).commit();
    }

    @Override
    public void onTrxVerified() {
        Bundle verifyTrxBundle = new Bundle();

        verifyTrxBundle.putString("COIN_ICON_URL_KEY", mPendingTrx.iconUrl);
        verifyTrxBundle.putString("COIN_USD_VALUE", mPendingTrx.tokenPriceUsd);
        verifyTrxBundle.putString("COIN_TICKER_KEY", mPendingTrx.tokenTicker);
        verifyTrxBundle.putBoolean("TRX_SENT_KEY", false);
        verifyTrxBundle.putString("TRX_RECIPIENT", mPendingTrx.recipient);
        verifyTrxBundle.putString("TRX_AMOUNT", mPendingTrx.tokenAmount);
        verifyTrxBundle.putInt("TRX_STATUS", mPendingTrx.status);
        verifyTrxBundle.putString("TRX_EXTERNAL_URL", mPendingTrx.externalUrl);
        verifyTrxBundle.putString("TRX_DATE", mPendingTrx.date);
        verifyTrxBundle.putString("TRX_GAS", mPendingTrx.gas);
        verifyTrxBundle.putString("COIN_CONTRACT_ADDRESS_KEY", mPendingTrx.contractAddress);
        verifyTrxBundle.putString("COIN_CHAIN_TYPE_KEY", mPendingTrx.chainType);
        verifyTrxBundle.putString("COIN_NAME_KEY", mPendingTrx.coinName);

        WalletTransaction mWalletTrxFragment = new WalletTransaction();
        mWalletTrxFragment.setArguments(verifyTrxBundle);

        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            getSupportFragmentManager().popBackStack();
        }

        getSupportFragmentManager().beginTransaction()
                .replace(R.id.wallet_fragment_container_view, mWalletTrxFragment, null)
                .commit();
    }

    @Override
    public void onNavigateTransactionView(TransactionObj trxObj) {
        Bundle trxBundle = new Bundle();

        String address = trxObj.isSender ? trxObj.recipientAddress : trxObj.senderAddress;

        String externalUrl = "";
        if (trxObj.chainType.equals("BEP20")) {
            externalUrl = "https://bscscan.io/tx/" + trxObj.hash;
        } else {
            externalUrl = "https://etherscan.io/tx/" + trxObj.hash;
        }

        trxBundle.putString("COIN_ICON_URL_KEY", trxObj.tokenIconUrl);
        trxBundle.putString("COIN_USD_VALUE", trxObj.tokenPriceUsd);
        trxBundle.putString("COIN_TICKER_KEY", trxObj.tokenSymbol);
        // trxBundle.putInt("COIN_TYPE_KEY", trxObj.coinType);
        trxBundle.putBoolean("TRX_SENT_KEY", true);
        trxBundle.putString("TRX_RECIPIENT", address);
        trxBundle.putString("TRX_AMOUNT", trxObj.amount);
        trxBundle.putInt("TRX_STATUS", 3);
        trxBundle.putString("TRX_EXTERNAL_URL", externalUrl);
        trxBundle.putString("TRX_DATE", trxObj.timestamp);
        trxBundle.putString("TRX_GAS", trxObj.gas);
        trxBundle.putString("COIN_CONTRACT_ADDRESS_KEY", trxObj.chainType);
        trxBundle.putString("COIN_CHAIN_TYPE_KEY", trxObj.chainType);
        trxBundle.putString("COIN_NAME_KEY", trxObj.coinName);

        WalletTransaction mWalletTrxFragment = new WalletTransaction();
        mWalletTrxFragment.setArguments(trxBundle);

        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            getSupportFragmentManager().popBackStack();
        }

        getSupportFragmentManager().beginTransaction()
                .addToBackStack(null)
                .replace(R.id.wallet_fragment_container_view, mWalletTrxFragment, null)
                .commit();
    }

    @Override
    public void onNavigateVerifyTrx(String mCoinName, int mCoinIcon, String mCoinIconUrl, String mCoinTicker, String mCoinBalance, int mCoinType,
            String mRecipientAddress, String mAmount, String mGas, String mCoinUSDValue, String mContractAddress, String mChainType) {

        Date c = Calendar.getInstance().getTime();

        SimpleDateFormat df = new SimpleDateFormat("dd-MMM-yyyy", Locale.getDefault());
        String date = df.format(c);

        mPendingTrx = new TrxObj(mCoinIcon, mCoinIconUrl, mCoinUSDValue, mAmount, date, "",
            mContractAddress, false, mChainType, mRecipientAddress, 1, mCoinName, mCoinType, mGas);

        Bundle verifyTrxBundle = new Bundle();
        verifyTrxBundle.putString("PIN_CODE_KEY", mPinCode);
        verifyTrxBundle.putBoolean("IS_VERIFY_TRX_KEY", true);

        PinCodeFragment mPinCodeFragment = new PinCodeFragment();
        mPinCodeFragment.setArguments(verifyTrxBundle);

        mPinCodeInterface = (PinCodeInterface)mPinCodeFragment;

        getSupportFragmentManager().beginTransaction()
                .addToBackStack(null)
                .replace(R.id.wallet_fragment_container_view, mPinCodeFragment, null)
                .commit();
    }

    private void fetchWallet() {
        if (mSharedPreferences == null) mSharedPreferences = new EncryptSharedPreferences(this).getSharedPreferences();
        String mnemonic = mSharedPreferences.getString("MNEMONIC_KEY", "");
        mWallet = new HDWallet(mnemonic, mPinCode);

        fetchBalance();

        getTokenTrx();

        MainTabFragment mMainTabFragment = new MainTabFragment();
        mWalletDatabaseInterface = mMainTabFragment;

        getSupportFragmentManager().beginTransaction()
                .add(R.id.wallet_fragment_container_view, mMainTabFragment, null)
                .commit();
    }

    private void importWallet() {
        try {
            mWallet = new HDWallet(mMnemonicString, mPinCode);

            if (mSharedPreferences == null) mSharedPreferences = new EncryptSharedPreferences(this).getSharedPreferences();
            mSharedPreferences.edit().putString("MNEMONIC_KEY", mWallet.mnemonic()).commit();

            MainTabFragment mMainTabFragment = new MainTabFragment();
            mWalletDatabaseInterface = mMainTabFragment;

            getSupportFragmentManager().beginTransaction()
                    .add(R.id.wallet_fragment_container_view, mMainTabFragment, null)
                    .commit();
        } catch (Exception e) {
            onDeleteWallet();

            getSupportFragmentManager().beginTransaction()
                    // .setReorderingAllowed(true)
                    .add(R.id.wallet_fragment_container_view, CreateWalletMenu.class, null)
                    .commit();

            Toast.makeText(this, "Failed to import wallet. Please try again.", Toast.LENGTH_SHORT).show();
        }
    }

    private void createWallet() {
        mWallet = new HDWallet(mWallet.mnemonic(), mPinCode);

        if (mSharedPreferences == null) mSharedPreferences = new EncryptSharedPreferences(this).getSharedPreferences();
        mSharedPreferences.edit().putString("MNEMONIC_KEY", mWallet.mnemonic()).commit();

        MainTabFragment mMainTabFragment = new MainTabFragment();
        mWalletDatabaseInterface = mMainTabFragment;

        getSupportFragmentManager().beginTransaction()
                .add(R.id.wallet_fragment_container_view, MainTabFragment.class, null)
                .commit();
    }

    private ByteString toByteString(BigInteger byteStr) {
        return ByteString.copyFrom(byteStr.toByteArray());
    }

    private String toHexString(byte[] byteStr, boolean withPrefix) {
      StringBuilder stringBuilder = new StringBuilder();
      if (withPrefix) {
         stringBuilder.append("0x");
      }

      byte[] var6 = byteStr;
      int var7 = byteStr.length;

      for(int var5 = 0; var5 < var7; ++var5) {
         byte element = var6[var5];
         String var9 = "%02x";
         Object[] var10001 = new Object[1];
         byte var11 = (byte)255;
         var10001[0] = (byte)(element & var11);
         Object[] var10 = var10001;
         String var12 = String.format(var9, java.util.Arrays.copyOf(var10, var10.length));

         stringBuilder.append(var12);
      }

      String var10000 = stringBuilder.toString();
      return var10000;
   }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.wallet_activity);

        LinearLayout mWalletTabTitleContainer = findViewById(R.id.wallet_tab_title_container);
        mWalletTabTitleContainer.setVisibility(View.GONE);

        mSharedPreferences = new EncryptSharedPreferences(this).getSharedPreferences();
        String pinCode = mSharedPreferences.getString("PIN_CODE_KEY", "");

        View mBackButton = findViewById(R.id.wallet_back_button);
        mBackButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onBackPressed();
            }
        });

        if (pinCode.length() == 0) {
            // Wallet doesnt exist
            getSupportFragmentManager().beginTransaction()
                    // .setReorderingAllowed(true)
                    .add(R.id.wallet_fragment_container_view, CreateWalletMenu.class, null)
                    .commit();

            mWallet = new HDWallet(128, "");
        } else {
            // Wallet exists go to pin screen
            Bundle existingWalletBundle = new Bundle();
            existingWalletBundle.putString("PIN_CODE_KEY", pinCode);

            PinCodeFragment mPinCodeFragment = new PinCodeFragment();
            mPinCodeFragment.setArguments(existingWalletBundle);

            mPinCodeInterface = (PinCodeInterface)mPinCodeFragment;

            getSupportFragmentManager().beginTransaction()
                    .replace(R.id.wallet_fragment_container_view, mPinCodeFragment, null)
                    .commit();
        }
    }
}
