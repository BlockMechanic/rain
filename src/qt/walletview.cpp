// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletview.h>

#include <qt/addressbookpage.h>
#include <qt/askpassphrasedialog.h>
#include <qt/raingui.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/messagemodel.h>
#include <qt/messagepage.h>
#include <qt/optionsmodel.h>
#include <qt/dashboardwidget.h>
#include <qt/platformstyle.h>
#include <qt/sendmessagespage.h>
#include <qt/signverifymessagedialog.h>
#include <qt/transactiontablemodel.h>
#include <qt/transactionview.h>
#include <qt/walletmodel.h>

#include <qt/chartswidget.h>
//#include <qt/blockexplorer.h>
#include <qt/addresseswidget.h>
#include <qt/assetpage.h>
#include <qt/historywidget.h>
#include <qt/receivewidget.h>
#include <qt/send.h>
#include <qt/settings/settingswidget.h>

#include <qt/masternodeswidget.h>
#include <qt/coldstakingwidget.h>


#include <interfaces/node.h>
#include <ui_interface.h>

#include <QAction>
#include <QActionGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QProgressDialog>
#include <QPushButton>
#include <QVBoxLayout>

WalletView::WalletView(const PlatformStyle *_platformStyle, QWidget *parent):
    QSlideStackedWidget(parent),
    clientModel(nullptr),
    walletModel(nullptr),
    platformStyle(_platformStyle)
{
    // customize
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSizePolicy(sizePolicy);
    setContentsMargins(0,200,0,0);
    setSpeed(300);

    // Create tabs
    dashboard = new DashboardWidget(platformStyle);
    sendMessagesPage = new SendMessagesPage(platformStyle, this);
    messagePage = new MessagePage(platformStyle, this);
    chartsWidget = new ChartsWidget(this);
    addressesWidget = new AddressesWidget(this);
    assetPage = new AssetPage(platformStyle, this);
    historyWidget = new HistoryWidget(this);
    sendWidget = new SendWidget(this);
    receiveWidget = new ReceiveWidget(this);
    settingsWidget = new SettingsWidget(this);
    usedSendingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::SendingTab, this);
    usedReceivingAddressesPage = new AddressBookPage(platformStyle, AddressBookPage::ForEditing, AddressBookPage::ReceivingTab, this);

    masterNodesWidget = new MasterNodesWidget(this);
    coldStakingWidget = new ColdStakingWidget(this);

//    WalletFrame* pWalletFrame = qobject_cast<WalletFrame*>(parent());

    addWidget(dashboard);
    addWidget(sendMessagesPage);
    addWidget(messagePage);
    addWidget(chartsWidget);
    addWidget(addressesWidget);
    addWidget(assetPage);
    addWidget(historyWidget);
    addWidget(sendWidget);
    addWidget(receiveWidget);
    addWidget(coldStakingWidget);
    addWidget(masterNodesWidget);    
        
    addWidget(settingsWidget);
    // Clicking on a transaction on the overview pre-selects the transaction on the transaction history page
    //connect(dashboard, &DashboardWidget::transactionClicked, transactionView, static_cast<void (TransactionView::*)(const QModelIndex&)>(&TransactionView::focusTransaction));

    //connect(dashboard, &DashboardWidget::outOfSyncWarningClicked, this, &WalletView::requestedSyncWarningInfo);

    // Highlight transaction after send
    //connect(sendCoinsPage, &SendCoinsDialog::coinsSent, transactionView, static_cast<void (TransactionView::*)(const uint256&)>(&TransactionView::focusTransaction));

    // Clicking on "Export" allows to export the transaction list
    //connect(exportButton, &QPushButton::clicked, transactionView, &TransactionView::exportClicked);

    // Pass through messages from sendCoinsPage
   // connect(sendCoinsPage, &SendCoinsDialog::message, this, &WalletView::message);
    // Pass through messages from transactionView
   // connect(transactionView, &TransactionView::message, this, &WalletView::message);
}

WalletView::~WalletView()
{
}

void WalletView::setRainGUI(RainGUI *gui)
{
    this->fgui = gui;
    if (gui)
    {
        // Clicking on a transaction on the overview page simply sends you to transaction history page
//        connect(overviewPage, &DashboardWidget::transactionClicked, gui, &RainGUI::goToHistory);

        // Navigate to transaction history page after send
     //   connect(sendCoinsPage, &SendCoinsDialog::coinsSent, gui, &RainGUI::goToHistory);

        // Receive and report messages
        connect(this, &WalletView::message, [gui](const QString &title, const QString &message, unsigned int style) {
            gui->message(title, message, style);
        });

        connect(sendWidget, &SendWidget::message, gui, &RainGUI::message);
        connect(receiveWidget, &ReceiveWidget::message, gui, &RainGUI::message);
        connect(addressesWidget, &AddressesWidget::message, gui, &RainGUI::message);
        connect(settingsWidget, &SettingsWidget::message, gui, &RainGUI::message);
        connect(historyWidget, &HistoryWidget::message, gui, &RainGUI::message);
        connect(chartsWidget, &ChartsWidget::message, gui, &RainGUI::message);

        // Pass through encryption status changed signals
        connect(this, &WalletView::encryptionStatusChanged, gui, &RainGUI::updateWalletStatus);

        // Pass through transaction notifications
        connect(this, &WalletView::incomingTransaction, gui, &RainGUI::incomingTransaction);

        connect(this, &WalletView::incomingMessage, gui, &RainGUI::incomingMessage);

        // Connect HD enabled state signal
        connect(this, &WalletView::hdEnabledStatusChanged, gui, &RainGUI::updateWalletStatus);

        // Connect SPV enabled state signal
        connect(this, &WalletView::spvEnabledStatusChanged, gui, &RainGUI::updateWalletStatus);


    }
}

void WalletView::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    dashboard->setClientModel(_clientModel);
    settingsWidget->setClientModel(_clientModel);
}

void WalletView::setWalletModel(WalletModel *_walletModel)
{
    this->walletModel = _walletModel;

    // Put transaction list in tabs
    //assetPage->setWalletModel(_walletModel);
    dashboard->setWalletModel(_walletModel);
    historyWidget->setWalletModel(_walletModel);
    sendMessagesPage->setWalletModel(_walletModel);

    receiveWidget->setWalletModel(_walletModel);
    sendWidget->setWalletModel(_walletModel);
    addressesWidget->setWalletModel(_walletModel);
    settingsWidget->setWalletModel(walletModel);
    chartsWidget->setWalletModel(_walletModel);
    fgui->setWalletModel(_walletModel);
    usedReceivingAddressesPage->setModel(_walletModel ? _walletModel->getAddressTableModel() : nullptr);
    usedSendingAddressesPage->setModel(_walletModel ? _walletModel->getAddressTableModel() : nullptr);

    coldStakingWidget->setWalletModel(_walletModel);
    masterNodesWidget->setWalletModel(_walletModel);

    if (_walletModel)
    {
        // Receive and pass through messages from wallet model
        connect(_walletModel, &WalletModel::message, this, &WalletView::message);

        // Handle changes in encryption status
        connect(_walletModel, &WalletModel::encryptionStatusChanged, this, &WalletView::encryptionStatusChanged);
        updateEncryptionStatus();

        // update HD status
        Q_EMIT hdEnabledStatusChanged();

        // update SPV status
        connect(_walletModel, SIGNAL(spvEnabledStatusChanged(int)), this, SLOT(updateSPVStatus()));
        updateSPVStatus();

        // Balloon pop-up for new transaction
        connect(_walletModel->getTransactionTableModel(), &TransactionTableModel::rowsInserted, this, &WalletView::processNewTransaction);

        // Ask for passphrase if needed
        //connect(_walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
        connect(_walletModel, &WalletModel::requireUnlock, this, &WalletView::unlockWallet);

        // Show progress dialog
        connect(_walletModel, &WalletModel::showProgress, this, &WalletView::showProgress);
    }
}

void WalletView::setMessageModel(MessageModel *messageModel)
{
    this->messageModel = messageModel;
    if(messageModel)
    {
        messagePage->setModel(messageModel);
        sendMessagesPage->setModel(messageModel);

        // Balloon pop-up for new message
        connect(messageModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(processNewMessage(QModelIndex,int,int)));
    }
}


void WalletView::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!walletModel || !clientModel || clientModel->node().isInitialBlockDownload())
        return;

    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;


    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = ttm->data(index, TransactionTableModel::LabelRole).toString();

    Q_EMIT incomingTransaction(date, walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label, walletModel->getWalletName());
}

void WalletView::processNewMessage(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if(!messageModel)
        return;

    MessageModel *mm = messageModel;

    QString sent_datetime = mm->index(start, MessageModel::ReceivedDateTime, parent).data().toString();
    QString from_address  = mm->index(start, MessageModel::FromAddress,      parent).data().toString();
    QString to_address    = mm->index(start, MessageModel::ToAddress,        parent).data().toString();
    QString message       = mm->index(start, MessageModel::Message,          parent).data().toString();
    int type              = mm->index(start, MessageModel::TypeInt,          parent).data().toInt();

    Q_EMIT incomingMessage(sent_datetime, from_address, to_address, message, type);
}


void WalletView::gotoDashbord()
{
    setCurrentWidget(dashboard);
}

void WalletView::gotoSendMessagesPage()
{
    setCurrentWidget(sendMessagesPage);
}

void WalletView::gotoMessagesPage()
{
    setCurrentWidget(messagePage);
}


void WalletView::gotoHistoryPage()
{
    setCurrentWidget(historyWidget);
}

void WalletView::gotoReceiveCoinsPage()
{
    setCurrentWidget(receiveWidget);
}

void WalletView::gotoSendCoinsPage(QString addr)
{
    setCurrentWidget(sendWidget);

   // if (!addr.isEmpty())
   //     sendCoinsPage->setAddress(addr);
}

void WalletView::gotoChartsPage()
{
    setCurrentWidget(chartsWidget);
}

void WalletView::gotoContactsPage()
{
    setCurrentWidget(addressesWidget);
}

void WalletView::gotoAssetPage()
{
    setCurrentWidget(assetPage);
}

void WalletView::gotoMasterNodes()
{
    setCurrentWidget(masterNodesWidget);
}

void WalletView::gotoColdStaking()
{
    setCurrentWidget(coldStakingWidget);
}

void WalletView::gotoSettings()
{
    setCurrentWidget(settingsWidget);
}

void WalletView::gotoSignMessageTab(QString addr)
{
    // calls show() in showTab_SM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_SM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_SM(addr);
}

void WalletView::gotoVerifyMessageTab(QString addr)
{
    // calls show() in showTab_VM()
    SignVerifyMessageDialog *signVerifyMessageDialog = new SignVerifyMessageDialog(platformStyle, this);
    signVerifyMessageDialog->setAttribute(Qt::WA_DeleteOnClose);
    signVerifyMessageDialog->setModel(walletModel);
    signVerifyMessageDialog->showTab_VM(true);

    if (!addr.isEmpty())
        signVerifyMessageDialog->setAddress_VM(addr);
}

bool WalletView::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    return false;
}

void WalletView::showOutOfSyncWarning(bool fShow)
{
//    dashboard->showOutOfSyncWarning(fShow);
}

void WalletView::updateEncryptionStatus()
{
    Q_EMIT encryptionStatusChanged();
}

void WalletView::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt : AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    updateEncryptionStatus();
}

void WalletView::backupWallet()
{
    QString filename = GUIUtil::getSaveFileName(this,
        tr("Backup Wallet"), QString(),
        tr("Wallet Data (*.dat)"), nullptr);

    if (filename.isEmpty())
        return;

    if (!walletModel->wallet().backupWallet(filename.toLocal8Bit().data())) {
        Q_EMIT message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        Q_EMIT message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void WalletView::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void WalletView::unlockWallet(/*bool fromMenu*/)
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
//#ifdef ENABLE_PROOF_OF_STAKE
//        AskPassphraseDialog::Mode mode = fromMenu ?  AskPassphraseDialog::UnlockStaking : AskPassphraseDialog::Unlock;
//        AskPassphraseDialog dlg(mode, this);
//else
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
//#endif
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void WalletView::lockWallet()
{
    if(!walletModel)
        return;

    walletModel->setWalletLocked(true);
}

void WalletView::usedSendingAddresses()
{
    if(!walletModel)
        return;

    GUIUtil::bringToFront(usedSendingAddressesPage);
}

void WalletView::usedReceivingAddresses()
{
    if(!walletModel)
        return;

    GUIUtil::bringToFront(usedReceivingAddressesPage);
}

void WalletView::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0) {
        progressDialog = new QProgressDialog(title, tr("Cancel"), 0, 100);
        GUIUtil::PolishProgressDialog(progressDialog);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    } else if (nProgress == 100) {
        if (progressDialog) {
            progressDialog->close();
            progressDialog->deleteLater();
            progressDialog = nullptr;
        }
    } else if (progressDialog) {
        if (progressDialog->wasCanceled()) {
            getWalletModel()->wallet().abortRescan();
        } else {
            progressDialog->setValue(nProgress);
        }
    }
}

void WalletView::requestedSyncWarningInfo()
{
    Q_EMIT outOfSyncWarningClicked();
}

void WalletView::setSPVMode(bool state)
{
    if(!walletModel)
        return;

    walletModel->setSpvEnabled(state);
}

bool WalletView::getSPVMode()
{
    if(!walletModel)
        return false;

    return walletModel->spvEnabled();
}

void WalletView::updateSPVStatus()
{
    Q_EMIT spvEnabledStatusChanged(walletModel->spvEnabled());
}
