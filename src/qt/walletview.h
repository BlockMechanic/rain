// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_WALLETVIEW_H
#define RAIN_QT_WALLETVIEW_H

#include <amount.h>
#include <qt/qslidestakingwidget.h>

class RainGUI;
class ClientModel;
class MessagePage;
class DashboardWidget;
class PlatformStyle;
class SendCoinsRecipient;
class SendMessagesPage;
class WalletModel;
class AddressBookPage;
class MessageModel;
class ChartsWidget;
class AddressesWidget;
class AssetPage;
class HistoryWidget;
class SendWidget;
class ReceiveWidget;
class SettingsWidget;

class MasterNodesWidget;
class ColdStakingWidget;

QT_BEGIN_NAMESPACE
class QModelIndex;
class QProgressDialog;
QT_END_NAMESPACE

/*
  WalletView class. This class represents the view to a single wallet.
  It was added to support multiple wallet functionality. Each wallet gets its own WalletView instance.
  It communicates with both the client and the wallet models to give the user an up-to-date view of the
  current core state.
*/
class WalletView : public QSlideStackedWidget
{
    Q_OBJECT

public:
    explicit WalletView(const PlatformStyle *platformStyle, QWidget *parent);
    ~WalletView();

    void setRainGUI(RainGUI *gui);
    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    WalletModel *getWalletModel() { return walletModel; }
    /** Set the wallet model.
        The wallet model represents a rain wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel *walletModel);
	void setMessageModel(MessageModel *messageModel);

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);

private:
    ClientModel *clientModel;
    MessageModel *messageModel;
    WalletModel *walletModel;

    AddressBookPage *usedSendingAddressesPage;
    AddressBookPage *usedReceivingAddressesPage;
    SendMessagesPage *sendMessagesPage;
    MessagePage *messagePage;

    QProgressDialog *progressDialog;
    const PlatformStyle *platformStyle;

    ChartsWidget *chartsWidget;
    AddressesWidget *addressesWidget;
    AssetPage *assetPage;
    HistoryWidget *historyWidget;
    
    SendWidget *sendWidget;
    ReceiveWidget *receiveWidget;
    
    DashboardWidget *dashboard;
    SettingsWidget *settingsWidget;

    MasterNodesWidget *masterNodesWidget = nullptr;
    ColdStakingWidget *coldStakingWidget = nullptr;

    RainGUI * fgui;

public Q_SLOTS:
    /** Switch to overview (home) page */
    void gotoDashbord();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
	void gotoSendMessagesPage();
    /** Switch to view messages page */
    void gotoMessagesPage();

    /** Switch to view charts page */
    void gotoChartsPage();
    
    /** Switch to view contacts page */
    void gotoContactsPage();    
    
    /** Switch to view explorer page */
    void gotoAssetPage();  

    void gotoMasterNodes();

    void gotoColdStaking();

    void gotoSettings();
    
    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);

    void processNewMessage(const QModelIndex& parent, int start, int /*end*/);

    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */

    void unlockWallet(/*bool fromMenu = false*/);
    /** Lock the wallet */
    void lockWallet();

    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();

    /** Re-emit encryption status signal */
    void updateEncryptionStatus();

    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString &title, int nProgress);

    /** User has requested more information about the out of sync state */
    void requestedSyncWarningInfo();

    /** setter and getter of the wallet's spv mode */
    void setSPVMode(bool state);
    bool getSPVMode();

    /** Update the GUI to reflect the new SPV status */
    void updateSPVStatus();

Q_SIGNALS:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);
    /** Encryption status of wallet changed */
    void encryptionStatusChanged();
    /** HD-Enabled status of wallet changed (only possible during startup) */
    void hdEnabledStatusChanged();
    /** SPV-Enabled status of wallet changed*/
    void spvEnabledStatusChanged(int spvEnabled);
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);

    void incomingMessage(const QString& sent_datetime, QString from_address, QString to_address, QString message, int type);

    /** Notify that the out of sync warning icon has been pressed */
    void outOfSyncWarningClicked();
    /** Show the assets GUI */
    void checkAssets();
};

#endif // RAIN_QT_WALLETVIEW_H
