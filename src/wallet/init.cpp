// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/init.h>

#include <chainparams.h>
#include <init.h>
#include <net.h>
#include <scheduler.h>
#include <outputtype.h>
#include <util.h>
#include <utilmoneystr.h>
#include <validation.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>
#include <veil/ringct/anonwallet.h>
#include <veil/zerocoin/zwallet.h>
#include <veil/zerocoin/precompute.h>

void WalletInit::AddWalletOptions() const
{
    gArgs.AddArg("-addresstype", strprintf("What type of addresses to use (\"legacy\", \"p2sh-segwit\", or \"bech32\", default: \"%s\")", FormatOutputType(DEFAULT_ADDRESS_TYPE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-avoidpartialspends", strprintf(_("Group outputs by address, selecting all or none, instead of selecting on a per-output basis. Privacy is improved as an address is only used once (unless someone sends to it after spending from it), but may result in slightly higher fees as suboptimal coin selection may result due to the added limitation (default: %u)"), DEFAULT_AVOIDPARTIALSPENDS), false, OptionsCategory::WALLET);
    gArgs.AddArg("-backupzerocoin", "Enable automatic backups of zerocoin", false, OptionsCategory::WALLET);
    gArgs.AddArg("-changetype", "What type of change to use (\"legacy\", \"p2sh-segwit\", or \"bech32\"). Default is same as -addresstype, except when -addresstype=p2sh-segwit a native segwit output is used when sending to a native segwit address)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-disablewallet", "Do not load the wallet and disable wallet RPC calls", false, OptionsCategory::WALLET);
    gArgs.AddArg("-discardfee=<amt>", strprintf("The fee rate (in %s/kB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target",
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-fallbackfee=<amt>", strprintf("A fee rate (in %s/kB) that will be used when fee estimation has insufficient data (default: %s)",
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)), false, OptionsCategory::WALLET);
    gArgs.AddArg("-generateseed", "Generate a new wallet seed phrase", false, OptionsCategory::WALLET);
    gArgs.AddArg("-importseed", "Import an existing seed phrase", false, OptionsCategory::WALLET);
    gArgs.AddArg("-keypool=<n>", strprintf("Set key pool size to <n> (default: %u)", DEFAULT_KEYPOOL_SIZE), false, OptionsCategory::WALLET);
    gArgs.AddArg("-mintxfee=<amt>", strprintf("Fees (in %s/kB) smaller than this are considered zero fee for transaction creation (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)), false, OptionsCategory::WALLET);
    // default denom
    gArgs.AddArg("-nautomintdenom=<n>", strprintf("Set preferred automint denomination (default: %d)", DEFAULT_AUTOMINT_DENOM), false, OptionsCategory::WALLET);

    gArgs.AddArg("-paytxfee=<amt>", strprintf("Fee (in %s/kB) to add to transactions you send (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())), false, OptionsCategory::WALLET);
    gArgs.AddArg("-rescan", "Rescan the block chain for missing wallet transactions on startup", false, OptionsCategory::WALLET);
    gArgs.AddArg("-salvagewallet", "Attempt to recover private keys from a corrupt wallet on startup", false, OptionsCategory::WALLET);
    gArgs.AddArg("-spendzeroconfchange", strprintf("Spend unconfirmed change when sending transactions (default: %u)", DEFAULT_SPEND_ZEROCONF_CHANGE), false, OptionsCategory::WALLET);
    gArgs.AddArg("-staking", strprintf("Enable stake mining (default: %d)", true), false, OptionsCategory::WALLET);
    gArgs.AddArg("-txconfirmtarget=<n>", strprintf("If paytxfee is not set, include enough fee so transactions begin confirmation on average within n blocks (default: %u)", DEFAULT_TX_CONFIRM_TARGET), false, OptionsCategory::WALLET);
    gArgs.AddArg("-upgradewallet", "Upgrade wallet to latest format on startup", false, OptionsCategory::WALLET);
    gArgs.AddArg("-wallet=<path>", "Specify wallet database path. Can be specified multiple times to load multiple wallets. Path is interpreted relative to <walletdir> if it is not absolute, and will be created if it does not exist (as a directory containing a wallet.dat file and log files). For backwards compatibility this will also accept names of existing data files in <walletdir>.)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletbroadcast",  strprintf("Make the wallet broadcast transactions (default: %u)", DEFAULT_WALLETBROADCAST), false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletdir=<dir>", "Specify directory to hold wallets (default: <datadir>/wallets if it exists, otherwise <datadir>)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletnotify=<cmd>", "Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-walletrbf", strprintf("Send transactions with full-RBF opt-in enabled (RPC only, default: %u)", DEFAULT_WALLET_RBF), false, OptionsCategory::WALLET);
    gArgs.AddArg("-zapwallettxes=<mode>", "Delete all wallet transactions and only recover those parts of the blockchain through -rescan on startup"
                               " (1 = keep tx meta data e.g. payment request information, 2 = drop tx meta data)", false, OptionsCategory::WALLET);
    gArgs.AddArg("-zbackuppath=<path>", "Specify an additional path to backup zerocoin to", false, OptionsCategory::WALLET);

    gArgs.AddArg("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DEFAULT_WALLET_DBLOGSIZE), true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET), true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", DEFAULT_WALLET_PRIVDB), true, OptionsCategory::WALLET_DEBUG_TEST);
    gArgs.AddArg("-walletrejectlongchains", strprintf("Wallet will not create transactions that violate mempool chain limits (default: %u)", DEFAULT_WALLET_REJECT_LONG_CHAINS), true, OptionsCategory::WALLET_DEBUG_TEST);

    gArgs.AddArg("-gen=<n>", strprintf("Enable CPU mining to true on the given number of threads (default: %u)", 0), false, OptionsCategory::WALLET);
    gArgs.AddArg("-genoverride", strprintf("Allows you to override the IsInitialBlockDownload check in BitcoinMiner for PoW mining (default: %u)", false), false, OptionsCategory::HIDDEN);

    gArgs.AddArg("-precompute=<n>", strprintf("Enable the wallet to start solving zerocoin spend proofs inorder to make staking and spending zerocoin faster (default: %u)", false), false, OptionsCategory::WALLET);
    gArgs.AddArg("-precomputeblockpercycle=<n>", strprintf("Set the number of included blocks to precompute per cycle. (minimum: %d) (maximum: %d) (default: %d)", MIN_PRECOMPUTE_BPC, DEFAULT_PRECOMPUTE_BPC, MIN_PRECOMPUTE_BPC), false, OptionsCategory::WALLET);
    gArgs.AddArg("-autospend", strprintf("Enable to wallet to start trying to auto spend zerocoin to an address this wallet controls (default: %u)", false), false, OptionsCategory::WALLET);
    gArgs.AddArg("-autospendcount=<n>", strprintf("The selected number between 1 - 3 that the wallet will try to auto spend (default: %d)", 1), false, OptionsCategory::WALLET);
    gArgs.AddArg("-autospenddenom=<n>", strprintf("The selected denomination (10, 100, 1000, 10000) to try and auto spend (default: %d)", 10), false, OptionsCategory::WALLET);
    gArgs.AddArg("-autospendaddress=<address>", strprintf("The selected destination address used for auto spend. If one isn't given, a new address will be created by the wallet, and will be saved to use until it is changed"), false, OptionsCategory::WALLET);

}

bool WalletInit::ParameterInteraction() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
        }
        if (gArgs.SoftSetBoolArg("-staking", false))
            LogPrintf("%s: parameter interaction: -disablewallet -> disabeling staking\n", __func__);

        return true;
    }

    gArgs.SoftSetArg("-wallet", "");
    const bool is_multiwallet = gArgs.GetArgs("-wallet").size() > 1;

    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
    }

    if (gArgs.GetBoolArg("-salvagewallet", false)) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-salvagewallet"));
        }
        // Rewrite just private keys: rescan to find transactions
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -salvagewallet=1 -> setting -rescan=1\n", __func__);
        }
    }

    bool zapwallettxes = gArgs.GetBoolArg("-zapwallettxes", false);
    // -zapwallettxes implies dropping the mempool on startup
    if (zapwallettxes && gArgs.SoftSetBoolArg("-persistmempool", false)) {
        LogPrintf("%s: parameter interaction: -zapwallettxes enabled -> setting -persistmempool=0\n", __func__);
    }

    // -zapwallettxes implies a rescan
    if (zapwallettxes) {
        if (is_multiwallet) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-zapwallettxes"));
        }
        if (gArgs.SoftSetBoolArg("-rescan", true)) {
            LogPrintf("%s: parameter interaction: -zapwallettxes enabled -> setting -rescan=1\n", __func__);
        }
    }

    if (is_multiwallet) {
        if (gArgs.GetBoolArg("-upgradewallet", false)) {
            return InitError(strprintf("%s is only allowed with a single wallet file", "-upgradewallet"));
        }
    }

    if (gArgs.GetBoolArg("-sysperms", false))
        return InitError("-sysperms is not allowed in combination with enabled wallet functionality");
    if (gArgs.GetArg("-prune", 0) && gArgs.GetBoolArg("-rescan", false))
        return InitError(_("Rescans are not possible in pruned mode. You will need to use -reindex which will download the whole blockchain again."));

    if (::minRelayTxFee.GetFeePerK() > HIGH_TX_FEE_PER_KB)
        InitWarning(AmountHighWarn("-minrelaytxfee") + " " +
                    _("The wallet will avoid paying less than the minimum relay fee."));

    if (gArgs.IsArgSet("-maxtxfee"))
    {
        CAmount nMaxFee = 0;
        if (!ParseMoney(gArgs.GetArg("-maxtxfee", ""), nMaxFee))
            return InitError(AmountErrMsg("maxtxfee", gArgs.GetArg("-maxtxfee", "")));
        if (nMaxFee > HIGH_MAX_TX_FEE)
            InitWarning(_("-maxtxfee is set very high! Fees this large could be paid on a single transaction."));
        maxTxFee = nMaxFee;
        if (CFeeRate(maxTxFee, 1000) < ::minRelayTxFee)
        {
            return InitError(strprintf(_("Invalid amount for -maxtxfee=<amount>: '%s' (must be at least the minrelay fee of %s to prevent stuck transactions)"),
                                       gArgs.GetArg("-maxtxfee", ""), ::minRelayTxFee.ToString()));
        }
    }

    return true;
}

void WalletInit::RegisterRPC(CRPCTable &t) const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return;
    }

    RegisterWalletRPCCommands(t);
}

bool WalletInit::Verify() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        return true;
    }

    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        if (!fs::exists(wallet_dir)) {
            return InitError(strprintf(_("Specified -walletdir \"%s\" does not exist"), wallet_dir.string()));
        } else if (!fs::is_directory(wallet_dir)) {
            return InitError(strprintf(_("Specified -walletdir \"%s\" is not a directory"), wallet_dir.string()));
        } else if (!wallet_dir.is_absolute()) {
            return InitError(strprintf(_("Specified -walletdir \"%s\" is a relative path"), wallet_dir.string()));
        }
    }

    LogPrintf("Using wallet directory %s\n", GetWalletDir().string());

    uiInterface.InitMessage(_("Verifying wallet(s)..."));

    std::vector<std::string> wallet_files = gArgs.GetArgs("-wallet");

    // Parameter interaction code should have thrown an error if -salvagewallet
    // was enabled with more than wallet file, so the wallet_files size check
    // here should have no effect.
    bool salvage_wallet = gArgs.GetBoolArg("-salvagewallet", false) && wallet_files.size() <= 1;

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : wallet_files) {
        fs::path wallet_path = fs::absolute(wallet_file, GetWalletDir());

        if (!wallet_paths.insert(wallet_path).second) {
            return InitError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified."), wallet_file));
        }

        std::string error_string;
        std::string warning_string;
        bool verify_success = CWallet::Verify(wallet_file, salvage_wallet, error_string, warning_string);
        if (!error_string.empty()) InitError(error_string);
        if (!warning_string.empty()) InitWarning(warning_string);
        if (!verify_success) return false;
    }

    return true;
}

bool WalletInit::Open() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return true;
    }

    for (const std::string& walletFile : gArgs.GetArgs("-wallet")) {
        std::shared_ptr<CWallet> pwallet = CWallet::CreateWalletFromFile(walletFile, fs::absolute(walletFile, GetWalletDir()));
        if (!pwallet)
            return false;

        bool fEnableZPivBackups = gArgs.GetBoolArg("-backupzerocoin", true);
        pwallet->setZPivAutoBackups(fEnableZPivBackups);

        AddWallet(pwallet);
        pwallet->GetZWallet()->LoadMintPoolFromDB();
        pwallet->GetZWallet()->SyncWithChain();
    }

    return true;
}

void WalletInit::Start(CScheduler& scheduler) const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->postInitProcess();
    }

    // Run a thread to flush wallet periodically
    scheduler.scheduleEvery(MaybeCompactWalletDB, 500);
}

void WalletInit::Flush() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->Flush(false);
    }
}

void WalletInit::Stop() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        pwallet->Flush(true);
    }
}

void WalletInit::Close() const
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets()) {
        RemoveWallet(pwallet);
    }
}
