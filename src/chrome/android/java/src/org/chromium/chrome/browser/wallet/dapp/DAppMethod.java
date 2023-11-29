package org.chromium.chrome.browser.wallet.dapp;

public enum DAppMethod {
    SIGNTRANSACTION,
    SIGNPERSONALMESSAGE,
    SIGNMESSAGE,
    SIGNTYPEDMESSAGE,
    ECRECOVER,
    REQUESTACCOUNTS,
    WATCHASSET,
    ADDETHEREUMCHAIN,
    SWITCHCHAIN,
    SWITCHETHEREUMCHAIN,
    UNKNOWN;

    public static DAppMethod fromValue(String value) {
        switch (value) {
            case "signTransaction":
                return SIGNTRANSACTION;
            case "signPersonalMessage":
                return SIGNPERSONALMESSAGE;
            case "signMessage":
                return SIGNMESSAGE;
            case "signTypedMessage":
                return SIGNTYPEDMESSAGE;
            case "ecRecover":
                return ECRECOVER;
            case "requestAccounts":
                return REQUESTACCOUNTS;
            case "switchEthereumChain":
                return SWITCHETHEREUMCHAIN;
            case "switchChain":
                return SWITCHCHAIN;
            case "watchAsset":
                return WATCHASSET;
            case "addEthereumChain":
                return ADDETHEREUMCHAIN;
            default:
                return UNKNOWN;
        }
    }
}
