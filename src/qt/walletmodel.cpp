// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/rain-config.h>
#endif

#include <qt/walletmodel.h>

#include <qt/addresstablemodel.h>
#include <qt/coincontrolmodel.h>
#include <qt/guiconstants.h>
#include <qt/optionsmodel.h>
#include <qt/paymentserver.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/transactiontablemodel.h>
#include <consensus/validation.h>
#include <consensus/tx_check.h>
#include <qt/assettablemodel.h>

#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <addressbook.h>

#include <ui_interface.h>
#include <util/system.h> // for GetBoolArg
#include <util/validation.h> 
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#include <rpc/util.h>

#include <stdint.h>

#include <QDebug>
#include <QMessageBox>
#include <QSet>
#include <QTimer>

SendAssetsRecipient::SendAssetsRecipient(SendCoinsRecipient r) :
    address(r.address),
    label(r.label),
    asset(uint256()),
    asset_amount(r.amount),
    message(r.message),
    sPaymentRequest(r.sPaymentRequest),
    authenticatedMerchant(r.authenticatedMerchant),
    fSubtractFeeFromAmount(r.fSubtractFeeFromAmount)
{
}

#define SendCoinsRecipient SendAssetsRecipient

WalletModel::WalletModel(std::unique_ptr<interfaces::Wallet> wallet, interfaces::Node& node, const PlatformStyle *platformStyle, OptionsModel *_optionsModel, QObject *parent) :
    QObject(parent), m_wallet(std::move(wallet)), m_node(node), optionsModel(_optionsModel), addressTableModel(nullptr), 
    transactionTableModel(nullptr),
    assetTableModel(nullptr),
    recentRequestsTableModel(nullptr),
    cachedEncryptionStatus(Unencrypted),
    cachedNumBlocks(0),
    cachedNumBlocksHeadersChain(0),
    nWeight(0),
    updateStakeWeight(true)

{
    fHaveWatchOnly = m_wallet->haveWatchOnly();
    addressTableModel = new AddressTableModel(this);
    transactionTableModel = new TransactionTableModel(platformStyle, this);
    assetTableModel = new AssetTableModel(m_node, this);
    coinControlModel = new CoinControlModel(this);
    recentRequestsTableModel = new RecentRequestsTableModel(this);

    // This timer will be fired repeatedly to update the balance
    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &WalletModel::pollBalanceChanged);
    pollTimer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

WalletModel::~WalletModel()
{
    unsubscribeFromCoreSignals();
}

std::set<CAsset> WalletModel::getAssetTypes() const
{
    return cached_asset_types;
}

bool WalletModel::isColdStakingNetworkelyEnabled() const
{
    return true;
}

CAmount WalletModel::getMinColdStakingAmount() const
{
    return Params().GetConsensus().nMinColdStakingAmount;
}

bool WalletModel::isColdStaking() const
{
    // TODO: Complete me..
    return false;
}

void WalletModel::updateStatus()
{
    EncryptionStatus newEncryptionStatus = getEncryptionStatus();

    if(cachedEncryptionStatus != newEncryptionStatus) {
        Q_EMIT encryptionStatusChanged(newEncryptionStatus);
    }
}

bool WalletModel::isWalletUnlocked() const
{
    EncryptionStatus status = getEncryptionStatus();
    return (status == Unencrypted || status == Unlocked);
}

bool WalletModel::isWalletLocked(bool fFullUnlocked) const
{
    EncryptionStatus status = getEncryptionStatus();
    return (status == Locked || (!fFullUnlocked && status == UnlockedForStaking));
}

void WalletModel::pollBalanceChanged()
{
    // Try to get balances and return early if locks can't be acquired. This
    // avoids the GUI from getting stuck on periodical polls if the core is
    // holding the locks for a longer time - for example, during a wallet
    // rescan.
    interfaces::WalletBalances new_balances;
    int numBlocks = -1;
    if (!m_wallet->tryGetBalances(new_balances, numBlocks)) {
        return;
    }

    if(fForceCheckBalanceChanged || m_node.getNumBlocks() != cachedNumBlocks /*|| (spvEnabled() && m_node.getNumHeaders() != cachedNumBlocksHeadersChain)*/)
    {
        fForceCheckBalanceChanged = false;

        // Balance and number of transactions might have changed
        cachedNumBlocks = m_node.getNumBlocks();
//        cachedNumBlocksHeadersChain = m_node.getNumHeaders();

        checkBalanceChanged(new_balances);
        if(transactionTableModel)
            transactionTableModel->updateConfirmations();

        // The stake weight is used for the staking icon status
        // Get the stake weight only when not syncing because it is time consuming
        updateStakeWeight = true;
       
    }
}

void WalletModel::checkBalanceChanged(const interfaces::WalletBalances& new_balances)
{
    if(new_balances.balanceChanged(m_cached_balances)) {
        m_cached_balances = new_balances;
        Q_EMIT balanceChanged(new_balances);

        std::set<CAsset> new_asset_types;
        for (const auto& assetamount : new_balances.balance + new_balances.unconfirmed_balance) {
            if (!assetamount.second) continue;
            new_asset_types.insert(assetamount.first);
        }
        if (new_asset_types != cached_asset_types) {
            cached_asset_types = new_asset_types;
            Q_EMIT assetTypesChanged();
        }
    }
}


void WalletModel::updateTransaction()
{
    // Balance and number of transactions might have changed
    fForceCheckBalanceChanged = true;
}

void WalletModel::updateAddressBook(const QString &address, const QString &label,
        bool isMine, const QString &purpose, int status)
{
    if(addressTableModel)
        addressTableModel->updateEntry(address, label, isMine, purpose, status);
}

void WalletModel::updateWatchOnlyFlag(bool fHaveWatchonly)
{
    fHaveWatchOnly = fHaveWatchonly;
    Q_EMIT notifyWatchonlyChanged(fHaveWatchonly);
}

bool WalletModel::validateAddress(const QString &address)
{
    // Only regular base58 addresses accepted here
    return IsValidDestinationString(address.toStdString(), false);
}

bool WalletModel::validateAddress(const QString& address, bool fStaking)
{
    return IsValidDestinationString(address.toStdString(), fStaking);
}

bool WalletModel::updateAddressBookLabels(const CTxDestination& dest, const std::string& strName, const std::string& strPurpose)
{
	auto mapAddressBook = m_wallet->getMapAddressBook();
    auto mi = mapAddressBook.find(dest);

    // Check if we have a new address or an updated label
    if (mi == mapAddressBook.end()) {
        return m_wallet->setAddressBook(dest, strName, strPurpose);
    } else if (mi->second.name != strName) {
        return m_wallet->setAddressBook(dest, strName, ""); // "" means don't change purpose
    }
    return false;
}

WalletModel::SendCoinsReturn WalletModel::prepareTransaction(WalletModelTransaction &transaction, const CCoinControl& coinControl)
{
    CAmountMap total;
    bool fSubtractFeeFromAmount = false;
    QList<SendAssetsRecipient> recipients = transaction.getRecipients();
    std::vector<CRecipient> vecSend;

    if(recipients.empty())
    {
        return OK;
    }

    if (isStakingOnlyUnlocked()) {
        return StakingOnlyUnlocked;
    }

    QSet<QString> setAddress; // Used to detect duplicates
    int nAddresses = 0;

    // Pre-check input data for validity
    for (const SendAssetsRecipient &rcp : recipients)
    {
        if (rcp.fSubtractFeeFromAmount)
            fSubtractFeeFromAmount = true;
        {   // User-entered rain address / amount:
            if(!validateAddress(rcp.address, rcp.isP2CS))
            {
                return InvalidAddress;
            }
            if(rcp.asset_amount <= 0)
            {
                return InvalidAmount;
            }
            setAddress.insert(rcp.address);
            ++nAddresses;

            CScript scriptPubKey;
            CTxDestination out = DecodeDestination(rcp.address.toStdString());
            if (rcp.isP2CS) {
                CTxDestination ownerAdd;
                if (rcp.ownerAddress.isEmpty()) {
                    // Create new internal owner address
                    if (!getNewAddress(ownerAdd).result)
                        return CannotCreateInternalAddress;
                } else {
                    ownerAdd = DecodeDestination(rcp.ownerAddress.toStdString());
                }

                const auto stakerId = boost::get<PKHash>(&out);
                const auto ownerId = boost::get<PKHash>(&ownerAdd);
                if (!stakerId || !ownerId) {
                    return InvalidAddress;
                }

                scriptPubKey = GetScriptForStakeDelegation(ToKeyID(*stakerId), ToKeyID(*ownerId));
            } else {
                scriptPubKey = GetScriptForDestination(out);
            }
            CPubKey confidentiality_pubkey = GetDestinationBlindingKey(out);
            CRecipient recipient = {scriptPubKey, rcp.asset_amount, rcp.asset, confidentiality_pubkey, rcp.fSubtractFeeFromAmount};
            vecSend.push_back(recipient);

            total[rcp.asset] += rcp.asset_amount;
        }
    }
    if(setAddress.size() != nAddresses)
    {
        return DuplicateAddress;
    }

    CAmountMap nBalance = m_wallet->getAvailableBalance(coinControl);

    if(total[vecSend[0].asset] > nBalance[vecSend[0].asset])
    {
		for(unsigned int u = 0; u<vecSend.size(); u++)
            LogPrintf("address = %s, amount = %d , asset= %s \n", vecSend[u].scriptPubKey.ToString(), vecSend[u].nAmount, vecSend[u].asset.getName());

		LogPrintf("size: %d , bal dif %d vs %d \n", vecSend.size(), total[vecSend[0].asset], nBalance[vecSend[0].asset]);
        return AmountExceedsBalance;
    }

    {
        CAmountMap nFeeRequired;
        int nChangePosRet = -1;
        std::string strFailReason;

        auto& newTx = transaction.getWtx();
        std::vector<CAmount> out_amounts;
        std::string strtxcomment = transaction.getstrTxComment();
        newTx = m_wallet->createTransaction(vecSend, coinControl, true /* sign */, nChangePosRet, nFeeRequired, out_amounts, strtxcomment, strFailReason);
        CAsset asset = vecSend[0].asset;
        transaction.setTransactionFee(nFeeRequired[asset]);
        if (fSubtractFeeFromAmount && newTx) {
            assert(out_amounts.size() == newTx->vout.size());
            transaction.reassignAmounts(out_amounts, nChangePosRet);
        }

        if(!newTx)
        {
            total += nFeeRequired;
            if(!fSubtractFeeFromAmount && total > nBalance)
            {
                return SendCoinsReturn(AmountWithFeeExceedsBalance);
            }
            Q_EMIT message(tr("Send Coins"), QString::fromStdString(strFailReason), CClientUIInterface::MSG_ERROR);
            LogPrintf("%s\n",strFailReason);
            return TransactionCreationFailed;
        }

        // reject absurdly high fee. (This can never happen because the
        // wallet caps the fee at m_default_max_tx_fee. This merely serves as a
        // belt-and-suspenders check)
        if (nFeeRequired[asset] > m_node.getMaxTxFee())
            return AbsurdFee;
    }

    return SendCoinsReturn(OK);
}

WalletModel::SendCoinsReturn WalletModel::sendCoins(WalletModelTransaction &transaction)
{
    QByteArray transaction_array; /* store serialized transaction */

    if (isStakingOnlyUnlocked()) {
		LogPrintf(" StakingOnlyUnlocked \n");
        return StakingOnlyUnlocked;
    }

    //bool fColdStakingActive = true;

    // Double check tx before do anything
    CValidationState state;
    if (!CheckTransaction(*transaction.getWtx().get(), state)) {
		LogPrintf(" TransactionCheckFailed , %s \n", FormatStateMessage(state));
        return TransactionCheckFailed;
    }

    {
        std::vector<std::pair<std::string, std::string>> vOrderForm;
        for (const SendCoinsRecipient &rcp : transaction.getRecipients())
        {
            if (!rcp.message.isEmpty()) // Message from normal rain:URI (rain:123...?message=example)
                vOrderForm.emplace_back("Message", rcp.message.toStdString());
        }

        auto& newTx = transaction.getWtx();
        std::string rejectReason;
        if (!wallet().commitTransaction(newTx, {} /* mapValue */, std::move(vOrderForm), rejectReason))
            return SendCoinsReturn(TransactionCommitFailed, QString::fromStdString(rejectReason));

        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << *newTx;
        transaction_array.append(&(ssTx[0]), ssTx.size());
    }

    // Add addresses / update labels that we've sent to the address book,
    // and emit coinsSent signal for each recipient
    for (const SendCoinsRecipient &rcp : transaction.getRecipients())
    {
        {
            std::string strAddress = rcp.address.toStdString();
            CTxDestination dest = DecodeDestination(strAddress);
            std::string strLabel = rcp.label.toStdString();
            {
                // Check if we have a new address or an updated label
                std::string name;
                if (!m_wallet->getAddress(
                     dest, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr))
                {
                    m_wallet->setAddressBook(dest, strLabel, "send");
                }
                else if (name != strLabel)
                {
                    m_wallet->setAddressBook(dest, strLabel, ""); // "" means don't change purpose
                }
            }
        }
        Q_EMIT coinsSent(this, rcp, transaction_array);
    }

    checkBalanceChanged(m_wallet->getBalances()); // update balance immediately, otherwise there could be a short noticeable delay until pollBalanceChanged hits

    return SendCoinsReturn(OK);
}

OptionsModel *WalletModel::getOptionsModel()
{
    return optionsModel;
}

AddressTableModel *WalletModel::getAddressTableModel()
{
    return addressTableModel;
}

TransactionTableModel *WalletModel::getTransactionTableModel()
{
    return transactionTableModel;
}

AssetTableModel *WalletModel::getAssetTableModel()
{
    return assetTableModel;
}

CoinControlModel *WalletModel::getCoinControlModel()
{
    return coinControlModel;
}

RecentRequestsTableModel *WalletModel::getRecentRequestsTableModel()
{
    return recentRequestsTableModel;
}

WalletModel::EncryptionStatus WalletModel::getEncryptionStatus() const
{
    if(!m_wallet->isCrypted())
    {
        return Unencrypted;
    } else if (m_wallet->getWalletUnlockStakingOnly()) {
        return UnlockedForStaking;
    }
    else if(m_wallet->isLocked())
    {
        return Locked;
    }
    else
    {
        return Unlocked;
    }
}

void WalletModel::getScriptForMining(std::shared_ptr<CTxDestination> &script){
	
	m_wallet->getScriptForMining(script);
}

bool WalletModel::setWalletEncrypted(bool encrypted, const SecureString &passphrase)
{
    if(encrypted)
    {
        // Encrypt
        return m_wallet->encryptWallet(passphrase);
    }
    else
    {
        // Decrypt -- TODO; not supported yet
        return false;
    }
}

bool WalletModel::setWalletLocked(bool locked, const SecureString &passPhrase, bool stakingOnly)
{
    if(locked)
    {
        // Lock
        m_wallet->setWalletUnlockStakingOnly(false);
        return m_wallet->lock();
    }
    else
    {
        // Unlock
        return m_wallet->unlock(passPhrase, stakingOnly);
    }
}

bool WalletModel::lockForStakingOnly(const SecureString& passPhrase)
{
    if (!m_wallet->isLocked()) {
        m_wallet->setWalletUnlockStakingOnly(true);
        return true;
    } else {
        setWalletLocked(false, passPhrase, true);
    }
    return false;
}

bool WalletModel::isStakingOnlyUnlocked()
{
    return m_wallet->getWalletUnlockStakingOnly();
}


bool WalletModel::changePassphrase(const SecureString &oldPass, const SecureString &newPass)
{
    m_wallet->lock(); // Make sure wallet is locked before attempting pass change
    return m_wallet->changeWalletPassphrase(oldPass, newPass);
}

// Handlers for core signals
static void NotifyUnload(WalletModel* walletModel)
{
    qDebug() << "NotifyUnload";
    bool invoked = QMetaObject::invokeMethod(walletModel, "unload");
    assert(invoked);
}

static void NotifyKeyStoreStatusChanged(WalletModel *walletmodel)
{
    qDebug() << "NotifyKeyStoreStatusChanged";
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateStatus", Qt::QueuedConnection);
    assert(invoked);
}

static void NotifyAddressBookChanged(WalletModel *walletmodel,
        const CTxDestination &address, const std::string &label, bool isMine,
        const std::string &purpose, ChangeType status)
{
    QString strAddress = QString::fromStdString(EncodeDestination(address));
    QString strLabel = QString::fromStdString(label);
    QString strPurpose = QString::fromStdString(purpose);

    qDebug() << "NotifyAddressBookChanged: " + strAddress + " " + strLabel + " isMine=" + QString::number(isMine) + " purpose=" + strPurpose + " status=" + QString::number(status);
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateAddressBook", Qt::QueuedConnection,
                              Q_ARG(QString, strAddress),
                              Q_ARG(QString, strLabel),
                              Q_ARG(bool, isMine),
                              Q_ARG(QString, strPurpose),
                              Q_ARG(int, status));
    assert(invoked);
}

static void NotifyTransactionChanged(WalletModel *walletmodel, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(hash);
    Q_UNUSED(status);
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateTransaction", Qt::QueuedConnection);
    assert(invoked);
}

static void ShowProgress(WalletModel *walletmodel, const std::string &title, int nProgress)
{
    // emits signal "showProgress"
    bool invoked = QMetaObject::invokeMethod(walletmodel, "showProgress", Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(title)),
                              Q_ARG(int, nProgress));
    assert(invoked);
}

static void NotifyWatchonlyChanged(WalletModel *walletmodel, bool fHaveWatchonly)
{
    bool invoked = QMetaObject::invokeMethod(walletmodel, "updateWatchOnlyFlag", Qt::QueuedConnection,
                              Q_ARG(bool, fHaveWatchonly));
    assert(invoked);
}

static void NotifyCanGetAddressesChanged(WalletModel* walletmodel)
{
    bool invoked = QMetaObject::invokeMethod(walletmodel, "canGetAddressesChanged");
    assert(invoked);
}

static void NotifySPVModeChanged(WalletModel *walletmodel, bool fSPVModeEnabled)
{
    QMetaObject::invokeMethod(walletmodel, "updateSPVMode", Qt::QueuedConnection,
                              Q_ARG(bool, fSPVModeEnabled));
}

void WalletModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    m_handler_unload = m_wallet->handleUnload(std::bind(&NotifyUnload, this));
    m_handler_status_changed = m_wallet->handleStatusChanged(std::bind(&NotifyKeyStoreStatusChanged, this));
    m_handler_address_book_changed = m_wallet->handleAddressBookChanged(std::bind(NotifyAddressBookChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    m_handler_transaction_changed = m_wallet->handleTransactionChanged(std::bind(NotifyTransactionChanged, this, std::placeholders::_1, std::placeholders::_2));
    m_handler_show_progress = m_wallet->handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2));
    m_handler_watch_only_changed = m_wallet->handleWatchOnlyChanged(std::bind(NotifyWatchonlyChanged, this, std::placeholders::_1));
    m_handler_can_get_addrs_changed = m_wallet->handleCanGetAddressesChanged(std::bind(NotifyCanGetAddressesChanged, this));
    m_handler_spv_mode_changed = m_wallet->handleSPVModeChanged(std::bind(NotifySPVModeChanged, this, std::placeholders::_1));
}

void WalletModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    m_handler_unload->disconnect();
    m_handler_status_changed->disconnect();
    m_handler_address_book_changed->disconnect();
    m_handler_transaction_changed->disconnect();
    m_handler_show_progress->disconnect();
    m_handler_watch_only_changed->disconnect();
    m_handler_can_get_addrs_changed->disconnect();
    m_handler_spv_mode_changed->disconnect();
}

// WalletModel::UnlockContext implementation
WalletModel::UnlockContext WalletModel::requestUnlock()
{
    const WalletModel::EncryptionStatus status_before = getEncryptionStatus();
    if (status_before == Locked || status_before == UnlockedForStaking)
    {
        // Request UI to unlock wallet
        Q_EMIT requireUnlock();
    }
    // If wallet is still locked, unlock was failed or cancelled, mark context as invalid
    bool valid = isWalletUnlocked();

    return UnlockContext(this, valid, status_before);
}

WalletModel::UnlockContext::UnlockContext(WalletModel *_wallet, bool _valid, const WalletModel::EncryptionStatus& status_before):
        wallet(_wallet),
        valid(_valid),
        was_status(status_before),
        relock(status_before == Locked || status_before == UnlockedForStaking)
{
}

WalletModel::UnlockContext::~UnlockContext()
{
    if (valid && relock && wallet) {
        if (was_status == Locked) wallet->setWalletLocked(true);
        else if (was_status == UnlockedForStaking) wallet->lockForStakingOnly();
        wallet->updateStatus();
    }
}

void WalletModel::UnlockContext::CopyFrom(UnlockContext&& rhs)
{
    // Transfer context; old object no longer relocks wallet
    *this = rhs;
    rhs.relock = false;
}

bool WalletModel::getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    return m_wallet->getPubKey(address, vchPubKeyOut);
}

int64_t WalletModel::getCreationTime() const {

    return m_wallet->getCreationTime();
}

PairResult WalletModel::getNewAddress(CTxDestination& dest, std::string label, bool staking) const
{
    PairResult res = m_wallet->getNewAddress(dest,label,staking);
    return res;
}

bool WalletModel::whitelistAddressFromColdStaking(const QString &addressStr)
{
    return updateAddressBookPurpose(addressStr, AddressBook::AddressBookPurpose::DELEGATOR);
}

bool WalletModel::blacklistAddressFromColdStaking(const QString &addressStr)
{
    return updateAddressBookPurpose(addressStr, AddressBook::AddressBookPurpose::DELEGABLE);
}

bool WalletModel::updateAddressBookPurpose(const QString &addressStr, const std::string& purpose)
{
    CTxDestination dest = DecodeDestination(addressStr.toStdString());
    if (IsStakingAddress(addressStr.toStdString()))
        return error("Invalid RAIN address, cold staking address");

    return m_wallet->setAddressBook(dest, getLabelForAddress(dest), purpose);
}

bool WalletModel::IsSpendable(const CTxDestination& dest) const
{
    return IsMine(*getWallet(), dest) & ISMINE_SPENDABLE;
}

bool WalletModel::getPrivKey(const CKeyID &address, CKey& vchPrivKeyOut) const
{
    return m_wallet->getPrivKey(address, vchPrivKeyOut);
}

std::string WalletModel::getLabelForAddress(const CTxDestination& dest)
{
    std::string label = "";
    {
		auto mapAddressBook = m_wallet->getMapAddressBook();
		auto mi = mapAddressBook.find(dest);
        if (mi != mapAddressBook.end()) {
            label = mi->second.name;
        }
    }
    return label;
}

// returns a list of COutputs from COutPoints
void WalletModel::getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs)
{
	auto locked_chain = getWallet()->chain().lock();
    for (const COutPoint& outpoint : vOutpoints)
    {
        auto it = getWallet()->mapWallet.find(outpoint.hash);
        if (it == getWallet()->mapWallet.end()) continue;
        int nDepth = it->second.GetDepthInMainChain(*locked_chain);
        if (nDepth < 0) continue;
        COutput out(&it->second, outpoint.n, nDepth, true /* spendable */, true /* solvable */, true /* safe */);
        vOutputs.push_back(out);
    }
}

// returns a COutPoint of 10000 PIV if found
bool WalletModel::getMNCollateralCandidate(COutPoint& outPoint)
{
    std::vector<COutput> vCoins = m_wallet->availableCoins(true, false, false, false, CoinType::ONLY_MASTERNODE_COLLATERAL, 1,MAX_MONEY, MAX_MONEY, 0, 0);
    for (const COutput& out : vCoins) {
        // skip locked collaterals
        if (!isLockedCoin(out.tx->GetHash(), out.i)) {
            outPoint = COutPoint(out.tx->GetHash(), out.i);
            return true;
        }
    }
    return false;
}

bool WalletModel::isSpent(const COutPoint& outpoint) const
{
	auto locked_chain = getWallet()->chain().lock();
    return getWallet()->IsSpent(*locked_chain, outpoint.hash, outpoint.n);
}

void WalletModel::loadReceiveRequests(std::vector<std::string>& vReceiveRequests)
{
    vReceiveRequests = m_wallet->getDestValues("rr"); // receive request
}

bool WalletModel::saveReceiveRequest(const std::string &sAddress, const int64_t nId, const std::string &sRequest)
{
    CTxDestination dest = DecodeDestination(sAddress);

    std::stringstream ss;
    ss << nId;
    std::string key = "rr" + ss.str(); // "rr" prefix = "receive request" in destdata

    if (sRequest.empty())
        return m_wallet->eraseDestData(dest, key);
    else
        return m_wallet->addDestData(dest, key, sRequest);
}

bool WalletModel::bumpFee(uint256 hash, uint256& new_hash)
{
    CCoinControl coin_control;
    coin_control.m_signal_bip125_rbf = true;
    std::vector<std::string> errors;
    CAmountMap old_fee;
    CAmountMap new_fee;
    CAmountMap total_fee;
    CMutableTransaction mtx;
    if (!m_wallet->createBumpTransaction(hash, coin_control, total_fee , errors, old_fee, new_fee, mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Increasing transaction fee failed") + "<br />(" +
            (errors.size() ? QString::fromStdString(errors[0]) : "") +")");
         return false;
    }

    // allow a user based fee verification
    QString questionString = tr("Do you want to increase the fee?");
    questionString.append("<br />");
    questionString.append("<table style=\"text-align: left;\">");
    questionString.append("<tr><td>");
    questionString.append(tr("Current fee:"));
    questionString.append("</td><td>");
    questionString.append(RainUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), old_fee[mtx.vout[0].nAsset.GetAsset()]));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("Increase:"));
    questionString.append("</td><td>");
    questionString.append(RainUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), (new_fee - old_fee)[mtx.vout[0].nAsset.GetAsset()]));
    questionString.append("</td></tr><tr><td>");
    questionString.append(tr("New fee:"));
    questionString.append("</td><td>");
    questionString.append(RainUnits::formatHtmlWithUnit(getOptionsModel()->getDisplayUnit(), new_fee[mtx.vout[0].nAsset.GetAsset()]));
    questionString.append("</td></tr></table>");

    WalletModel::UnlockContext ctx(requestUnlock());
    if(!ctx.isValid())
    {
        return false;
    }

    // sign bumped transaction
    if (!m_wallet->signBumpTransaction(mtx)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Can't sign transaction."));
        return false;
    }
    // commit the bumped transaction
    if(!m_wallet->commitBumpTransaction(hash, std::move(mtx), errors, new_hash)) {
        QMessageBox::critical(nullptr, tr("Fee bump error"), tr("Could not commit transaction") + "<br />(" +
            QString::fromStdString(errors[0])+")");
         return false;
    }
    return true;
}

bool WalletModel::isWalletEnabled()
{
   return !gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET);
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
void WalletModel::listCoins(std::map<QString, std::vector<COutput> >& mapCoins) const
{
	auto locked_chain = getWallet()->chain().lock();
    for (auto& group : getWallet()->ListCoins(*locked_chain)) {
        auto& resultGroup = mapCoins[QString::fromStdString(EncodeDestination(group.first))];
        for (auto& coin : group.second) {
            resultGroup.emplace_back(std::move(coin));
        }
    }
}

// AvailableCoins + LockedCoins grouped by wallet address (put change in one group with wallet address)
/*
void WalletModel::listCoins(std::map<ListCoinsKey, std::vector<ListCoinsValue>>& mapCoins) const
{
    CWallet::AvailableCoinsFilter filter;
    filter.fIncludeLocked = true;
    std::vector<COutput> vCoins;
    wallet->AvailableCoins(&vCoins, nullptr, filter);

    for (const COutput& out : vCoins) {
        if (!out.fSpendable) continue;

        const CScript& scriptPubKey = out.tx->vout[out.i].scriptPubKey;
        const bool isP2CS = scriptPubKey.IsPayToColdStaking();

        CTxDestination outputAddress;
        CTxDestination outputAddressStaker;
        if (isP2CS) {
            txnouttype type; std::vector<CTxDestination> addresses; int nRequired;
            if(!ExtractDestinations(scriptPubKey, type, addresses, nRequired)
                        || addresses.size() != 2) throw std::runtime_error("Cannot extract P2CS addresses from a stored transaction");
            outputAddressStaker = addresses[0];
            outputAddress = addresses[1];
        } else {
            if (!ExtractDestination(scriptPubKey, outputAddress))
                continue;
        }

        QString address = QString::fromStdString(EncodeDestination(outputAddress));
        Optional<QString> stakerAddr = IsValidDestination(outputAddressStaker) ?
            Optional<QString>(QString::fromStdString(EncodeDestination(outputAddressStaker, CChainParams::STAKING_ADDRESS))) :
            nullopt;

        ListCoinsKey key{address, wallet->IsChange(outputAddress), stakerAddr};
        ListCoinsValue value{
                out.tx->GetHash(),
                out.i,
                out.tx->vout[out.i].nValue,
                out.tx->GetTxTime(),
                out.nDepth
        };
        mapCoins[key].emplace_back(value);
    }
}
*/
bool WalletModel::isLockedCoin(uint256 hash, unsigned int n) const
{
    return getWallet()->IsLockedCoin(hash, n);
}

void WalletModel::lockCoin(COutPoint& output)
{
    getWallet()->LockCoin(output);
}

void WalletModel::unlockCoin(COutPoint& output)
{
    getWallet()->UnlockCoin(output);
}

void WalletModel::listLockedCoins(std::vector<COutPoint>& vOutpts)
{
    getWallet()->ListLockedCoins(vOutpts);
}


bool WalletModel::spvEnabled() const
{
    return m_wallet->isSPVEnabled();
}

void WalletModel::setSpvEnabled(bool state)
{
    m_wallet->SetSPVEnabled(state);
}

void WalletModel::updateSPVMode(bool fSPVModeEnabled)
{
    Q_EMIT spvEnabledStatusChanged(fSPVModeEnabled);
}

bool WalletModel::privateKeysDisabled() const
{
    return m_wallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS);
}

bool WalletModel::canGetAddresses() const
{
    return m_wallet->canGetAddresses();
}

CWallet *WalletModel::getWallet() const
{
    return m_wallet->getWallet().get();
}

QString WalletModel::getWalletName() const
{
    return QString::fromStdString(m_wallet->getWalletName());
}

QString WalletModel::getDisplayName() const
{
    const QString name = getWalletName();
    return name.isEmpty() ? "["+tr("default wallet")+"]" : name;
}

bool WalletModel::isMine(CTxDestination& dest)
{
    return m_wallet->isMine(dest);
}

bool WalletModel::isMine(const QString& addressStr)
{
	CTxDestination dest = DecodeDestination(addressStr.toUtf8().constData());
    return m_wallet->isMine(dest);
}

bool WalletModel::isUsed(CTxDestination& dest)
{
    return m_wallet->IsUsedDestination(dest);
}

bool WalletModel::isMultiwallet()
{
    return m_node.getWallets().size() > 1;
}

uint64_t WalletModel::getStakeWeight()
{
    return nWeight;
}

bool WalletModel::getWalletUnlockStakingOnly()
{
    return m_wallet->getWalletUnlockStakingOnly();
}

void WalletModel::setWalletUnlockStakingOnly(bool unlock)
{
    m_wallet->setWalletUnlockStakingOnly(unlock);
}

void WalletModel::checkStakeWeightChanged()
{
    if(updateStakeWeight && m_wallet->tryGetStakeWeight(nWeight))
    {
        updateStakeWeight = false;
    }
}

int WalletModel::getDefaultConfirmTarget() const
{
    return DEFAULT_TX_CONFIRM_TARGET;
}
