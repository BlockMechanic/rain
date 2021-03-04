// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_WALLETMODEL_H
#define RAIN_QT_WALLETMODEL_H

#include <amount.h>
#include <key.h>
#include <serialize.h>
#include <script/standard.h>

#include <wallet/coincontrol.h>
#include <ui_interface.h>

#if defined(HAVE_CONFIG_H)
#include <config/rain-config.h>
#endif

#include <qt/walletmodeltransaction.h>

#include <interfaces/wallet.h>
#include <support/allocators/secure.h>
#include "pairresult.h"
#include <map>
#include <vector>

#include <QObject>

enum class OutputType;

class AddressTableModel;
class OptionsModel;
class PlatformStyle;
class RecentRequestsTableModel;
class TransactionTableModel;
class AssetTableModel;
class CoinControlModel;
class WalletModelTransaction;

class CCoinControl;
class CKeyID;
class COutPoint;
class COutput;
class CPubKey;
class uint256;

namespace interfaces {
class Node;
} // namespace interfaces

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class SendCoinsRecipient
{
public:
    explicit SendCoinsRecipient() : amount(0), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION) { }
    explicit SendCoinsRecipient(const QString &addr, const QString &_label, const CAmount& _amount, const QString &_message):
        address(addr), label(_label), amount(_amount), message(_message), fSubtractFeeFromAmount(false), nVersion(SendCoinsRecipient::CURRENT_VERSION) {}

    // If from an unauthenticated payment request, this is used for storing
    // the addresses, e.g. address-A<br />address-B<br />address-C.
    // Info: As we don't need to process addresses in here when using
    // payment requests, we can abuse it for displaying an address list.
    // Todo: This is a hack, should be replaced with a cleaner solution!
    QString address;
    QString label;
    CoinType inputType;

    // Cold staking.
    bool isP2CS = false;
    QString ownerAddress;

    CAmount amount;
    // If from a payment request, this is used for storing the memo
    QString message;

    // If building with BIP70 is disabled, keep the payment request around as
    // serialized string to ensure load/store is lossless
    std::string sPaymentRequest;

    // Empty if no authentication or invalid signature/cert/etc.
    QString authenticatedMerchant;

    bool fSubtractFeeFromAmount; // memory only

    static const int CURRENT_VERSION = 1;
    int nVersion;

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        std::string sAddress = address.toStdString();
        std::string sLabel = label.toStdString();
        std::string sMessage = message.toStdString();

        std::string sAuthenticatedMerchant = authenticatedMerchant.toStdString();

        READWRITE(this->nVersion);
        READWRITE(sAddress);
        READWRITE(sLabel);
        READWRITE(amount);
        READWRITE(sMessage);
        READWRITE(sPaymentRequest);
        READWRITE(sAuthenticatedMerchant);

        if (ser_action.ForRead())
        {
            address = QString::fromStdString(sAddress);
            label = QString::fromStdString(sLabel);
            message = QString::fromStdString(sMessage);

            authenticatedMerchant = QString::fromStdString(sAuthenticatedMerchant);
        }
    }
};

class SendAssetsRecipient
{
public:
    explicit SendAssetsRecipient() : fSubtractFeeFromAmount(false) { }
    explicit SendAssetsRecipient(SendCoinsRecipient r);
public:
    QString address;

    // Cold staking.
    bool isP2CS = false;
    QString ownerAddress;

    QString label;
    CAsset asset;
    CAmount asset_amount;
    QString message;

    // If building with BIP70 is disabled, keep the payment request around as
    // serialized string to ensure load/store is lossless
    std::string sPaymentRequest;
    QString authenticatedMerchant;

    bool fSubtractFeeFromAmount; // memory only
};

/** Interface to Rain wallet from Qt view code. */
class WalletModel : public QObject
{
    Q_OBJECT

public:
    explicit WalletModel(std::unique_ptr<interfaces::Wallet> wallet, interfaces::Node& node, const PlatformStyle *platformStyle, OptionsModel *optionsModel, QObject *parent = nullptr);
    ~WalletModel();

    enum StatusCode // Returned by sendCoins
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed,
        TransactionCheckFailed,
        TransactionCommitFailed,
        StakingOnlyUnlocked,
        AbsurdFee,
        CannotCreateInternalAddress
    };

    enum EncryptionStatus {
        Unencrypted,                 // !wallet->IsCrypted()
        Locked,                      // wallet->IsCrypted() && wallet->IsLocked()
        Unlocked,                    // wallet->IsCrypted() && !wallet->IsLocked()
        UnlockedForStaking          // wallet->IsCrypted() && !wallet->IsLocked() && wallet->fWalletUnlockStaking
    };

    OptionsModel* getOptionsModel();
    AddressTableModel* getAddressTableModel();
    TransactionTableModel* getTransactionTableModel();
    AssetTableModel* getAssetTableModel();
    CoinControlModel* getCoinControlModel();
    RecentRequestsTableModel* getRecentRequestsTableModel();

    /** Whether cold staking is enabled or disabled in the network **/
    bool isColdStakingNetworkelyEnabled() const;
    CAmount getMinColdStakingAmount() const;
    /* current staking status from the miner thread **/

    bool isColdStaking() const;

    std::set<CAsset> getAssetTypes() const;
    EncryptionStatus getEncryptionStatus() const;
    bool isWalletUnlocked() const;
    bool isWalletLocked(bool fFullUnlocked = true) const;
    void getScriptForMining(std::shared_ptr<CTxDestination> &script);


    // Check address for validity
    bool validateAddress(const QString& address);
    // Check address for validity and type (whether cold staking address or not)
    bool validateAddress(const QString& address, bool fStaking);

    // Return status record for SendCoins, contains error id + information
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode _status = OK, QString _reasonCommitFailed = "")
            : status(_status),
              reasonCommitFailed(_reasonCommitFailed)
        {
        }
        StatusCode status;
        QString reasonCommitFailed;
    };

    const CWalletTx* getTx(uint256 id);
    bool isCoinStake(QString id);
    bool isCoinStakeMine(QString id);

    // prepare transaction for getting txfee before sending coins
    SendCoinsReturn prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl);

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(WalletModelTransaction &transaction);

    // Wallet encryption
    bool setWalletEncrypted(bool encrypted, const SecureString &passphrase);
    // Passphrase only needed when unlocking
    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString(), bool stakingOnly = false);
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);

    bool getWalletUnlockStakingOnly();
    void setWalletUnlockStakingOnly(bool unlock);

    // Method used to "lock" the wallet only for staking purposes. Just a flag that should prevent possible movements in the wallet.
    // Passphrase only needed when unlocking.
    bool lockForStakingOnly(const SecureString& passPhrase = SecureString());

    // Is wallet unlocked for staking only?
    bool isStakingOnlyUnlocked();

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, const WalletModel::EncryptionStatus& status_before);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy constructor is disabled.
        UnlockContext(const UnlockContext&) = delete;
        // Move operator and constructor transfer the context
        UnlockContext(UnlockContext&& obj) { CopyFrom(std::move(obj)); }
        UnlockContext& operator=(UnlockContext&& rhs) { CopyFrom(std::move(rhs)); return *this; }

    private:
        WalletModel *wallet;
        bool valid;
        WalletModel::EncryptionStatus was_status;   // original status
        mutable bool relock; // mutable, as it can be set to false by copying

        UnlockContext& operator=(const UnlockContext&) = default;
        void CopyFrom(UnlockContext&& rhs);
    };

    UnlockContext requestUnlock();

    bool getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    int64_t getCreationTime() const;
    int64_t getKeyCreationTime(const CPubKey& key);
    int64_t getKeyCreationTime(const CTxDestination& address);
    PairResult getNewAddress(CTxDestination& ret, std::string label = "", bool staking = false) const;

    bool IsSpendable(const CTxDestination& dest) const;
    bool getPrivKey(const CKeyID &address, CKey& vchPrivKeyOut) const;
    bool isMine(CTxDestination& dest);
    bool whitelistAddressFromColdStaking(const QString &addressStr);
    bool blacklistAddressFromColdStaking(const QString &address);
    bool updateAddressBookPurpose(const QString &addressStr, const std::string& purpose);
    std::string getLabelForAddress(const CTxDestination& dest);
    bool isMine(const QString& addressStr);
    bool isUsed(CTxDestination& dest);
    void getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs);
    bool getMNCollateralCandidate(COutPoint& outPoint);
    bool isSpent(const COutPoint& outpoint) const;
    void listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const;

    bool isLockedCoin(uint256 hash, unsigned int n) const;
    void lockCoin(COutPoint& output);
    void unlockCoin(COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& vOutpts);

    void loadReceiveRequests(std::vector<std::string>& vReceiveRequests);
    bool saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest);

    bool bumpFee(uint256 hash, uint256& new_hash);

    static bool isWalletEnabled();

    bool spvEnabled() const;
    void setSpvEnabled(bool state);

    bool privateKeysDisabled() const;
    bool canGetAddresses() const;

    interfaces::Node& node() const { return m_node; }
    interfaces::Wallet& wallet() const { return *m_wallet; }

    QString getWalletName() const;
    QString getDisplayName() const;

    bool isMultiwallet();
    uint64_t getStakeWeight();

    AddressTableModel* getAddressTableModel() const { return addressTableModel; }

    int getDefaultConfirmTarget() const;

    CWallet* getWallet() const;

private:
    std::unique_ptr<interfaces::Wallet> m_wallet;
    std::unique_ptr<interfaces::Handler> m_handler_unload;
    std::unique_ptr<interfaces::Handler> m_handler_status_changed;
    std::unique_ptr<interfaces::Handler> m_handler_address_book_changed;
    std::unique_ptr<interfaces::Handler> m_handler_transaction_changed;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    std::unique_ptr<interfaces::Handler> m_handler_watch_only_changed;
    std::unique_ptr<interfaces::Handler> m_handler_can_get_addrs_changed;
    std::unique_ptr<interfaces::Handler> m_handler_spv_mode_changed;
    interfaces::Node& m_node;

    bool fHaveWatchOnly;
    bool fForceCheckBalanceChanged{false};

    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;
    AssetTableModel *assetTableModel;
    CoinControlModel *coinControlModel;
    RecentRequestsTableModel *recentRequestsTableModel;

    // Cache some values to be able to detect changes
    interfaces::WalletBalances m_cached_balances;
    std::set<CAsset> cached_asset_types;
    EncryptionStatus cachedEncryptionStatus;
    int cachedNumBlocks;
    int cachedNumBlocksHeadersChain;

    QTimer *pollTimer;

    uint64_t nWeight;
    std::atomic<bool> updateStakeWeight;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    void checkBalanceChanged(const interfaces::WalletBalances& new_balances);

Q_SIGNALS:
    // Signal that balance in wallet changed
    void balanceChanged(const interfaces::WalletBalances& balances);

    // Signal that the set of possessed asset types changed
    void assetTypesChanged();

    // Encryption status of wallet changed
    void encryptionStatusChanged(int status);

    // Signal emitted when wallet needs to be unlocked
    // It is valid behaviour for listeners to keep the wallet locked after this signal;
    // this means that the unlocking failed or was cancelled.
    void requireUnlock();

    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);

    // Coins sent: from wallet, to recipient, in (serialized) transaction:
    void coinsSent(WalletModel* wallet, SendAssetsRecipient recipient, QByteArray transaction);

    // Show progress dialog e.g. for rescan
    void showProgress(const QString &title, int nProgress);

    // Watch-only address added
    void notifyWatchonlyChanged(bool fHaveWatchonly);

    // Signal that wallet is about to be removed
    void spvEnabledStatusChanged(int status);

    // Signal that wallet is about to be removed
    void unload();

    // Notify that there are now keys in the keypool
    void canGetAddressesChanged();

public Q_SLOTS:
    /* Wallet status might have changed */
    void updateStatus();
    /* New transaction, or transaction changed status */
    void updateTransaction();
    /* New, updated or removed address book entry */
    void updateAddressBook(const QString &address, const QString &label, bool isMine, const QString &purpose, int status);
    /* Watch-only added */
    void updateWatchOnlyFlag(bool fHaveWatchonly);
    /* Current, immature or unconfirmed balance might have changed - emit 'balanceChanged' if so */
    void pollBalanceChanged();
    /* Update the SPV Mode */
    void updateSPVMode(bool state);

    /* Update stake weight when changed*/
    void checkStakeWeightChanged();
    /* Update address book labels in the database */
    bool updateAddressBookLabels(const CTxDestination& address, const std::string& strName, const std::string& strPurpose);
};

#endif // RAIN_QT_WALLETMODEL_H
