#ifndef RAINGUI_H
#define RAINGUI_H

#include <QQuickView>
#include <memory>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <qt/transactionfilterproxy.h>
#include <qt/addresstablemodel.h>
#include <qt/assettablemodel.h>
#include <qt/walletmodel.h>
#include <QTimer>
#include <QComboBox>
#include <QStringListModel>

class AddressViewDelegate;
class PlatformStyle;
class NetworkStyle;
class ClientModel;
class WalletController;
class WalletModel;
class MessageModel;
class AddressFilterProxyModel;
class AssetFilterProxy;
class RPCConsole;
class Notificator;


class CCoinControl;
static constexpr int HEADER_HEIGHT_DELTA_SYNC = 24;

QT_BEGIN_NAMESPACE
class QItemSelection;
class QSortFilterProxyModel;
class QModelIndex;
QT_END_NAMESPACE

namespace interfaces {
class Node;
}

class RainGUI : public QQuickView
{
    Q_OBJECT

public:
    RainGUI(interfaces::Node& node, const PlatformStyle *m_platformStyle, const NetworkStyle *networkStyle, QWindow *parent = nullptr);

    void subscribeToCoreSignals();
    void setWalletModel(WalletModel *model);
    void updateDisplayUnit();
    void setBalance(const interfaces::WalletBalances &balances);

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in Q_EMIT message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel = nullptr, interfaces::BlockAndHeaderTipInfo* tip_info = nullptr);
#ifdef ENABLE_WALLET
    void setWalletController(WalletController* wallet_controller);
    WalletController* getWalletController();

    /** Set the wallet model.
        The wallet model represents a rain wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void addWallet(WalletModel* walletModel);
    void removeWallet(WalletModel* walletModel);
    void removeAllWallets();
#endif // ENABLE_WALLET
    bool enableWallet = false;

    /** Disconnect core signals from GUI client */
    void unsubscribeFromCoreSignals();

public Q_SLOTS:
    void showInitMessage(const QString &message, int alignment, const QColor &color);
    void message(const QString& title, QString message, unsigned int style, bool* ret = nullptr, const QString& detailed_message = QString());
    void splashFinished();
    /** called by a timer to check if ShutdownRequested() has been set **/
    void detectShutdown();
    void close();

    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);

    void processNewMessage(const QModelIndex& parent, int start, int /*end*/);

    /** Encrypt the wallet */
    void encryptWallet(QString pass);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase(QString oldpass, QString newpass);
    /** Ask for passphrase to unlock wallet temporarily */

    void unlockWallet(QString pass, bool stakingonly);
    /** Lock the wallet */
    void lockWallet();
    /** Re-emit encryption status signal */
    void updateEncryptionStatus();

    /** Show incoming transaction notification for new transactions. */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);
	void incomingMessage(const QString& sent_datetime, QString from_address, QString to_address, QString message, int type);

    /** Set number of connections shown in the UI */
    void setNumConnections(int count);
    /** Set network state shown in the UI */
    void setNetworkActive(bool networkActive);
    /** Set number of blocks and last block date shown in the UI */
    void setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header);

#ifdef ENABLE_WALLET
    void setCurrentWallet(WalletModel* wallet_model);
    /** Set the UI status indicators based on the currently selected wallet.
    */
    void updateWalletStatus();

private:
    /** Set the encryption status as shown in the UI.
       @param[in] status            current encryption status
       @see WalletModel::EncryptionStatus
    */
    void setEncryptionStatus(int status);

    /** Set the hd-enabled status as shown in the UI.
     @param[in] hdEnabled         current hd enabled status
     @see WalletModel::EncryptionStatus
     */
    void setHDStatus(bool privkeyDisabled, int hdEnabled);
#endif // ENABLE_WALLET
    
    interfaces::Node& m_node;
    const PlatformStyle *m_platformStyle;
    const NetworkStyle* const m_networkStyle;
    ClientModel* m_clientModel = nullptr;
    WalletController* m_walletController{nullptr};
    CCoinControl *m_coin_control = nullptr;
    WalletModel* m_walletModel;
    MessageModel *messageModel;
    QSortFilterProxyModel *proxyModel;

    AddressTableModel* addressTablemodel = nullptr;
    AddressFilterProxyModel *addrfilter = nullptr;

    AssetTableModel *assetTableModel =nullptr;
    AssetFilterProxy *assetFilter = nullptr;
    Notificator* notificator = nullptr;
    RPCConsole* rpcConsole = nullptr;

    QObject* m_walletPane;
    QObject* m_sendPane;
    QObject* m_consolePane;
    QObject* m_stakeChartPane;
    QObject* m_appPane;
    QObject* m_frontPage;
    QObject* m_pricesWidget;

    QObject* m_AppDrawer;
    
    QObject* gameMenu;
    
    QObject* m_pushButtonYears;
    QObject* m_pushButtonMonths;
    QObject* m_pushButtonAll;
    
    QObject* m_stakeChartPage;

    std::unique_ptr<interfaces::Handler> m_handler_init_message;
    std::unique_ptr<interfaces::Handler> m_handler_show_progress;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;

    std::unique_ptr<interfaces::Handler> m_handler_message_box;
    std::unique_ptr<interfaces::Handler> m_handler_question;

    std::unique_ptr<TransactionFilterProxy> filter;
    TransactionFilterProxy* stakesFilter = nullptr;
    interfaces::WalletBalances m_balances;
    int m_displayUnit;
    int timerid;
    QTimer* timer;
    QString sendRequest(QString url);
    int prevBlocks = 0;

    // Cached index
    QModelIndex cachedaddrindex;

    // Cached sort type and order
    AddressTableModel::ColumnIndex sortType = AddressTableModel::Label;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;

    void sortAddresses();

    void updateStakeFilter();
    void AssetList();
    
    QList<QString> assetListModel;
    bool encrypted = false;
    bool privkeys = false;
    bool hdstatus = false;
    int encryptionstatus =0;

private Q_SLOTS:
    void requestRain();
    void requestAddress(QString purpose);
    void sendRain(QString address, QString amount, QString asset);
    void setBets(QString userAddress, QString aiAddress, QString amount, QString sAssetName, QString nNonce);
    void setClipboard(QString text);
    void setDisplayUnit(int index);
    void executeRpcCommand(QString command);
    void SetExchangeInfoTextLabels();
    void onSortChanged(int idx);
    void onSortOrderChanged(int idx);


    void updateNetworkState();
    //void updateCoinControlModel();
    void setSendBalance(QString assetname);
    void lockunlock(QString txhash, int n);
    void clearCoinCotrol();
    void setCoinCotrolOutput(QString txhash, int n, bool set);
    void setCoinCotrolRbf(bool set);
    void setCoinCotrolConfirmTarget(int num);
    void createAsset(QString sAssetName, QString sAssetShortName , QString addressTo, QString inputamount, QString assetToUse, QString outputamount, bool transferable, bool convertable, bool restricted, bool limited);
    void convertAsset(QString addressTo, QString inputamount, QString inputsAssetName, QString outputamount, QString outputsAssetName);
    void updateStakingIcon();

Q_SIGNALS:
    /** Encryption status of wallet changed */
    void encryptionStatusChanged();
    /** HD-Enabled status of wallet changed (only possible during startup) */
    void hdEnabledStatusChanged();

};

class FileIO : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(FileIO)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    FileIO(QObject *parent = 0);
    ~FileIO();

    Q_INVOKABLE void read();
    Q_INVOKABLE void write();
    QUrl source() const;
    QString text() const;
public Q_SLOTS:
    void setSource(QUrl source);
    void setText(QString text);
Q_SIGNALS:
    void sourceChanged(QUrl arg);
    void textChanged(QString arg);
private:
    QUrl m_source;
    QString m_text;
};

#endif // RAINGUI_H

