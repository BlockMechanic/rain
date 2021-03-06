// Copyright (c) 2018 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_INTERFACES_WALLET_H
#define RAIN_INTERFACES_WALLET_H

#include <amount.h>                    // For CAmount
#include <pubkey.h>                    // For CKeyID and CScriptID (definitions needed in CTxDestination instantiation)
#include <wallet/ismine.h>             // For isminefilter, isminetype
#include <script/standard.h>           // For CTxDestination
#include <support/allocators/secure.h> // For SecureString
#include <ui_interface.h>              // For ChangeType
#include <pairresult.h>

#include <functional>
#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

class CCoinControl;
class CFeeRate;
class CKey;
class CWallet;
class CWalletTx;
class COutput;
class CAddressBookData;
enum isminetype : unsigned int;
enum class FeeReason;
typedef uint8_t isminefilter;

enum class OutputType;
enum class CoinType;
struct CRecipient;

namespace interfaces {

class Handler;
struct WalletAddress;
struct WalletBalances;
struct WalletTx;
struct WalletTxOut;
struct WalletTxStatus;

using WalletOrderForm = std::vector<std::pair<std::string, std::string>>;
using WalletValueMap = std::map<std::string, std::string>;

//! Interface for accessing a wallet.
class Wallet
{
public:
    virtual ~Wallet() {}

    
    virtual int KeysLeftSinceAutoBackup () =0;
    
    virtual bool autoBackupWallet (CWallet* wallet, const std::string& strWalletFile_, std::string& strBackupWarningRet, std::string& strBackupErrorRet) =0;

    //! Encrypt wallet.
    virtual bool encryptWallet(const SecureString& wallet_passphrase) = 0;

    //! Return whether wallet is encrypted.
    virtual bool isCrypted() = 0;

    //! Lock wallet.
    virtual bool lock() = 0;

    //! Unlock wallet.
    virtual bool unlock(const SecureString& wallet_passphrase, bool fStakingOnly) = 0;

    //! Return whether wallet is locked.
    virtual bool isLocked() = 0;

    //! Change wallet passphrase.
    virtual bool changeWalletPassphrase(const SecureString& old_wallet_passphrase,
        const SecureString& new_wallet_passphrase) = 0;

    //! Abort a rescan.
    virtual void abortRescan() = 0;

    //! Back up wallet.
    virtual bool backupWallet(const std::string& filename) = 0;

    //! Get wallet name.
    virtual std::string getWalletName() = 0;

    // Get a new address.
    virtual bool getNewDestination(const OutputType type, const std::string label, CTxDestination& dest) = 0;

    //! Get public key.
    virtual bool getPubKey(const CKeyID& address, CPubKey& pub_key) = 0;

    //! Get private key.
    virtual bool getPrivKey(const CKeyID& address, CKey& key) = 0;

    virtual CKeyID getKeyForDestination(const CTxDestination& dest) = 0 ;

    //virtual CPubKey GetDestinationBlindingKey(const CTxDestination& dest);

    virtual bool haveKey(const CKeyID &address) = 0;

    virtual bool addKeyPubKey(const CKey& secret, const CPubKey &pubkey) = 0;

    //! Return whether wallet has private key.
    virtual bool isSpendable(const CTxDestination& dest) = 0;

    //! Return whether wallet has watch only keys.
    virtual bool haveWatchOnly() = 0;

    //! Add or update address.
    virtual bool setAddressBook(const CTxDestination& dest, const std::string& name, const std::string& purpose) = 0;

    // Remove address.
    virtual bool delAddressBook(const CTxDestination& dest) = 0;

    virtual bool isMine(CTxDestination& dest) = 0;
    
    virtual void markDirty() =0;
    
    virtual void getAvailableP2CSCoins(std::vector<COutput>& vCoins) =0;

    //! Look up address in wallet, return whether exists.
    virtual bool getAddress(const CTxDestination& dest,
        std::string* name,
        isminetype* is_mine,
        std::string* purpose) = 0;

    virtual PairResult getNewAddress(CTxDestination& dest, std::string& label, bool staking) =0;

    virtual bool IsUsedDestination(const CTxDestination& dst) =0;

    virtual std::map<CTxDestination, CAddressBookData> getMapAddressBook () =0;

    //! Get wallet address list.
    virtual std::vector<WalletAddress> getAddresses() = 0;

    //! Add scripts to key store so old so software versions opening the wallet
    //! database can detect payments to newer address types.
    virtual void learnRelatedScripts(const CPubKey& key, OutputType type) = 0;

    //! Get blinding pubkey for script
    virtual CPubKey getBlindingPubKey(const CScript& script) = 0;

    //! Add dest data.
    virtual bool addDestData(const CTxDestination& dest, const std::string& key, const std::string& value) = 0;

    //! Erase dest data.
    virtual bool eraseDestData(const CTxDestination& dest, const std::string& key) = 0;

    //! Get dest values with prefix.
    virtual std::vector<std::string> getDestValues(const std::string& prefix) = 0;

    //! Lock coin.
    virtual void lockCoin(const COutPoint& output) = 0;

    //! Unlock coin.
    virtual void unlockCoin(const COutPoint& output) = 0;

    //! Return whether coin is locked.
    virtual bool isLockedCoin(const COutPoint& output) = 0;

    //! List locked coins.
    virtual void listLockedCoins(std::vector<COutPoint>& outputs) = 0;

    //! Create transaction.
    virtual CTransactionRef createTransaction(const std::vector<CRecipient>& recipients,
        const CCoinControl& coin_control,
        bool sign,
        int& change_pos,
        CAmountMap& fee,
        std::vector<CAmount>& out_amounts,
        std::string& srttxcomment, 
        std::string& fail_reason) = 0;

    //! Commit transaction.
    virtual bool commitTransaction(CTransactionRef tx,
        WalletValueMap value_map,
        WalletOrderForm order_form,
        std::string& reject_reason) = 0;


    virtual int64_t getCreationTime() =0;
    //! Return whether transaction can be abandoned.
    virtual bool transactionCanBeAbandoned(const uint256& txid) = 0;

    //! Abandon transaction.
    virtual bool abandonTransaction(const uint256& txid) = 0;

    //! Return whether transaction can be bumped.
    virtual bool transactionCanBeBumped(const uint256& txid) = 0;

    //! Create bump transaction.
    virtual bool createBumpTransaction(const uint256& txid,
        const CCoinControl& coin_control,
        CAmountMap total_fee,
        std::vector<std::string>& errors,
        CAmountMap& old_fee,
        CAmountMap& new_fee,
        CMutableTransaction& mtx) = 0;

    //! Sign bump transaction.
    virtual bool signBumpTransaction(CMutableTransaction& mtx) = 0;

    //! Commit bump transaction.
    virtual bool commitBumpTransaction(const uint256& txid,
        CMutableTransaction&& mtx,
        std::vector<std::string>& errors,
        uint256& bumped_txid) = 0;

    //! Get a transaction.
    virtual CTransactionRef getTx(const uint256& txid) = 0;

    virtual CWalletTx getAmounts(const uint256& txid) = 0;

    //! Get transaction information.
    virtual WalletTx getWalletTx(const uint256& txid) = 0;

    //! Get list of all wallet transactions.
    virtual std::vector<WalletTx> getWalletTxs() = 0;

    //! Try to get updated status for a particular transaction, if possible without blocking.
    virtual bool tryGetTxStatus(const uint256& txid,
        WalletTxStatus& tx_status,
        int& num_blocks,
        int64_t& block_time) = 0;

    //! Get transaction details.
    virtual WalletTx getWalletTxDetails(const uint256& txid,
        WalletTxStatus& tx_status,
        WalletOrderForm& order_form,
        bool& in_mempool,
        int& num_blocks,
        int64_t& block_time) = 0;

    //! Get balances.
    virtual WalletBalances getBalances() = 0;

    //! Get balances if possible without blocking.
    virtual bool tryGetBalances(WalletBalances& balances, int& num_blocks) = 0;

    //! Get balance.
    virtual CAmountMap getBalance() = 0;

    //! Get available balance.
    virtual CAmountMap getAvailableBalance(const CCoinControl& coin_control) = 0;

    //! Return whether transaction input belongs to wallet.
    virtual isminetype txinIsMine(const CTxIn& txin) = 0;

    //! Return whether transaction output belongs to wallet.
    virtual isminetype txoutIsMine(const CTxOut& txout) = 0;

    //! Return debit amount if transaction input belongs to wallet.
    virtual CAmountMap getDebit(const CTxIn& txin, isminefilter filter) = 0;

    //! Return credit amount if transaction input belongs to wallet.
    virtual CAmountMap getCredit(const CTransaction& tx, const size_t out_index, isminefilter filter) = 0;

    //! Return credit amount if transaction input belongs to wallet.
    virtual CAmountMap getCredit(const CTxOut& txout, isminefilter filter) = 0;

    //! Return AvailableCoins + LockedCoins grouped by wallet address.
    //! (put change in one group with wallet address)
    using CoinsList = std::map<CTxDestination, std::vector<std::tuple<COutPoint, WalletTxOut>>>;
    virtual CoinsList listCoins() = 0;

    //! Return wallet transaction output information.
    virtual std::vector<WalletTxOut> getCoins(const std::vector<COutPoint>& outputs) = 0;

    //! Get required fee.
    virtual CAmountMap getRequiredFee(unsigned int tx_bytes) = 0;

    //! Get minimum fee.
    virtual CAmountMap getMinimumFee(unsigned int tx_bytes,
        const CCoinControl& coin_control,
        int* returned_target,
        FeeReason* reason) = 0;

    virtual std::vector<COutput> availableCoins (bool fOnlySafe, bool fStake, bool fIncludeDelegated, bool fIncludeColdStaking, CoinType nCoinType, const CAmount& nMinimumAmount, const CAmount& nMaximumAmount, const CAmount& nMinimumSumAmount, const uint64_t nMaximumCount, uint32_t nSpendTime) = 0;

    //! Get tx confirm target.
    virtual unsigned int getConfirmTarget() = 0;
    
    virtual std::vector<std::pair<std::string, int> > m_vMultiSend() =0;

    // Return whether HD enabled.
    virtual bool hdEnabled() = 0;

    // Return whether the wallet is blank.
    virtual bool canGetAddresses() = 0;

    // check if a certain wallet flag is set.
    virtual bool IsWalletFlagSet(uint64_t flag) = 0;

    // Get default address type.
    virtual OutputType getDefaultAddressType() = 0;

    // Get default change type.
    virtual OutputType getDefaultChangeType() = 0;

    //! Get max tx fee.
    virtual CAmount getDefaultMaxTxFee() = 0;

    // Remove wallet.
    virtual void remove() = 0;

    virtual bool isSPVEnabled() = 0;
    virtual void SetSPVEnabled(bool state)= 0;

    virtual std::shared_ptr<CWallet> getWallet() =0;

    //! Try get the stake weight
    virtual bool tryGetStakeWeight(uint64_t& nWeight) = 0;

    //! Get last coin stake search interval
    virtual int64_t getLastCoinStakeSearchInterval() = 0;

    //! Get wallet unlock for staking only
    virtual bool getWalletUnlockStakingOnly() = 0;

    //! Set wallet unlock for staking only
    virtual void setWalletUnlockStakingOnly(bool unlock) = 0;

    //! Register handler for unload message.
    using UnloadFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleUnload(UnloadFn fn) = 0;

    //! Register handler for show progress messages.
    using ShowProgressFn = std::function<void(const std::string& title, int progress)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;

    //! Register handler for status changed messages.
    using StatusChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleStatusChanged(StatusChangedFn fn) = 0;

    //! Register handler for address book changed messages.
    using AddressBookChangedFn = std::function<void(const CTxDestination& address,
        const std::string& label,
        bool is_mine,
        const std::string& purpose,
        ChangeType status)>;
    virtual std::unique_ptr<Handler> handleAddressBookChanged(AddressBookChangedFn fn) = 0;

    //! Register handler for transaction changed messages.
    using TransactionChangedFn = std::function<void(const uint256& txid, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleTransactionChanged(TransactionChangedFn fn) = 0;

    //! Register handler for watchonly changed messages.
    using WatchOnlyChangedFn = std::function<void(bool have_watch_only)>;
    virtual std::unique_ptr<Handler> handleWatchOnlyChanged(WatchOnlyChangedFn fn) = 0;

    //! Register handler for keypool changed messages.
    using CanGetAddressesChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn fn) = 0;
    virtual void getScriptForMining(std::shared_ptr<CTxDestination> &script) =0;;

    using SPVModeChangedFn = std::function<void(bool spv_mode_changed)>;
    virtual std::unique_ptr<Handler> handleSPVModeChanged(SPVModeChangedFn fn) = 0;
};

//! Information about one wallet address.
struct WalletAddress
{
    CTxDestination dest;
    isminetype is_mine;
    std::string name;
    std::string purpose;

    WalletAddress(CTxDestination dest, isminetype is_mine, std::string name, std::string purpose)
        : dest(std::move(dest)), is_mine(is_mine), name(std::move(name)), purpose(std::move(purpose))
    {
    }
};

//! Collection of wallet balances.
struct WalletBalances
{
    CAmountMap balance = CAmountMap();
    CAmountMap unconfirmed_balance = CAmountMap();
    CAmountMap immature_balance = CAmountMap();
    CAmountMap stake = CAmountMap();
    CAmountMap watch_only_stake = CAmountMap();

    bool have_watch_only = false;
    CAmountMap watch_only_balance = CAmountMap();
    CAmountMap unconfirmed_watch_only_balance = CAmountMap();
    CAmountMap immature_watch_only_balance = CAmountMap();
    CAmountMap locked = CAmountMap();
    CAmountMap unlocked =CAmountMap();
    bool balanceChanged(const WalletBalances& prev) const
    {
        return balance != prev.balance || unconfirmed_balance != prev.unconfirmed_balance ||
               immature_balance != prev.immature_balance || stake != prev.stake || watch_only_balance != prev.watch_only_balance ||
               unconfirmed_watch_only_balance != prev.unconfirmed_watch_only_balance ||
               immature_watch_only_balance != prev.immature_watch_only_balance || watch_only_stake != prev.watch_only_stake || locked != prev.locked || unlocked != prev.unlocked;
    }
};

// Wallet transaction information.
struct WalletTx
{
    CTransactionRef tx;
    std::vector<isminetype> txin_is_mine;
    std::vector<isminetype> txout_is_mine;
    std::vector<bool> txout_is_change;
    std::vector<CTxDestination> txout_address;
    std::vector<isminetype> txout_address_is_mine;
    std::vector<CAmount> txout_amounts;
    std::vector<CAsset> txout_assets;
    std::vector<CAmount> txin_issuance_asset_amount;
    std::vector<CAsset> txin_issuance_asset;
    std::vector<CAmount> txin_issuance_token_amount;
    std::vector<CAsset> txin_issuance_token;
    CAmountMap credit;
    CAmountMap debit;
    CAmountMap change;
    int64_t time;
    std::map<std::string, std::string> value_map;
    bool is_coinbase;
    bool is_coinstake;
    bool fValidated;
};

//! Updated transaction status.
struct WalletTxStatus
{
    int block_height;
    int blocks_to_maturity;
    int depth_in_main_chain;
    unsigned int time_received;
    uint32_t lock_time;
    bool is_final;
    bool is_trusted;
    bool is_abandoned;
    bool is_coinbase;
    bool is_coinstake;
    bool is_in_main_chain;
};

//! Wallet transaction output.
struct WalletTxOut
{
    CTxOut txout;
    int64_t time;
    int depth_in_main_chain = -1;
    bool is_spent = false;
};

//! Return implementation of Wallet interface. This function is defined in
//! dummywallet.cpp and throws if the wallet component is not compiled.
std::unique_ptr<Wallet> MakeWallet(const std::shared_ptr<CWallet>& wallet);

} // namespace interfaces

#endif // RAIN_INTERFACES_WALLET_H
