#include <qt/raingui.h>

#include <init.h>
#include <univalue.h>
#include <node/ui_interface.h>
#include <qt/rain.h>
#include <qt/guiutil.h>
#include <qt/walletcontroller.h>
#include <qt/networkstyle.h>
#include <qt/platformstyle.h>
#include <qt/addresstablemodel.h>
#include <qt/addressfilterproxymodel.h>
#include <qt/qrimagewidget.h>
#include <qt/transactiontablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/rainunits.h>
#include <qt/walletmodel.h>
#include <qt/rpcconsole.h>
#include <qt/messagemodel.h>
#include <qt/assettablemodel.h>
#include <qt/coincontrolmodel.h>
#include <qt/notificator.h>

#include <qt/assetfilterproxy.h>
#include <qt/clientmodel.h>
#include <validation.h>
#include <miner.h>
#include <key_io.h>
#include <wallet/wallet.h>
#include <wallet/coincontrol.h>

#include <interfaces/node.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <QTextDocumentFragment>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QtQuick/QQuickItem>
#include <QtQml/QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include <QtQuick/QQuickImageProvider>

#include <boost/xpressive/xpressive.hpp>

class IconProvider : public QQuickImageProvider
{
public:
    IconProvider(const NetworkStyle *networkStyle)
        : QQuickImageProvider(QQuickImageProvider::Pixmap)
    {
        m_networkStyle = networkStyle;
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        int width = 500;
        int height = 500;

        if (size)
            *size = QSize(width, height);

        if (id == "app") {
            return m_networkStyle->getAppIcon().pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                                                       requestedSize.height() > 0 ? requestedSize.height() : height);
        }
        else if (id == "window") {
            return m_networkStyle->getTrayAndWindowIcon().pixmap(requestedSize.width() > 0 ? requestedSize.width() : width,
                                                                 requestedSize.height() > 0 ? requestedSize.height() : height);
        }
        return QPixmap();
    }

private:
    const NetworkStyle* m_networkStyle;
};

class QrProvider : public QQuickImageProvider
{
public:
    QrProvider() : QQuickImageProvider(QQuickImageProvider::Image)
    {}

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(requestedSize)

        int width = 500;
        int height = 500;

        if (size)
            *size = QSize(width, height);

        QRImageWidget qr;
        qr.setQR(id);
        return qr.exportImage();
    }
};

RainGUI::RainGUI(interfaces::Node& node, const PlatformStyle *_platformStyle, const NetworkStyle *networkStyle, QWindow *parent) :
    QQuickView(parent),
    m_node(node),
    m_platformStyle(_platformStyle),
    m_networkStyle(networkStyle)
{
//    setFlags(Qt::FramelessWindowHint);

    QQmlEngine *engine = this->engine();
    engine->addImageProvider(QLatin1String("icons"), new IconProvider(networkStyle));
    engine->addImageProvider(QLatin1String("qr"), new QrProvider());
    engine->addImportPath("qrc:/qml");
    engine->addImportPath("qrc:/qml/charts");
    engine->addImportPath("qrc:/qml/contacts");
    engine->addImportPath("qrc:/qml/messages");

    QQuickStyle::setStyle("Default");

    QString licenseInfoHTML = QString::fromStdString(LicenseInfo());
    // Make URLs clickable
    QRegExp uri("<(.*)>", Qt::CaseSensitive, QRegExp::RegExp2);
    uri.setMinimal(true); // use non-greedy matching
    licenseInfoHTML.replace(uri, "<a href=\"\\1\">\\1</a>");
    // Replace newlines with HTML breaks
    licenseInfoHTML.replace("\n", "<br>");

    QList<QString> availableUnitNames;

    Q_FOREACH(RainUnits::Unit unit, RainUnits::availableUnits()) {
        availableUnitNames.append(RainUnits::longName(unit));
    }

    QVariantMap confirmationTargets;
/*
    for (const int n : confTargets) {
        confirmationTargets.insert(QString::number(n),
                                   QCoreApplication::translate("SendCoinsDialog", "%1 (%2 blocks)")
                                   .arg(GUIUtil::formatNiceTimeOffset(n*60))
                                   .arg(n));
    }
*/
    this->rootContext()->setContextProperty("availableUnits", QVariant(availableUnitNames));
    this->rootContext()->setContextProperty("confirmationTargets", QVariant(confirmationTargets));
    this->rootContext()->setContextProperty("licenceInfo", licenseInfoHTML);
    this->rootContext()->setContextProperty("version", QString::fromStdString(FormatFullVersion()));

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    this->setSource(url);
    this->setTitle(networkStyle->getTitleAddText());
    QString mode ="";
#ifdef Q_OS_ANDROID
    this->setHeight(2160/3);
    this->setWidth(1080/3);
    mode = "mobile";
#else
    this->setHeight(800);
    this->setWidth(1130);
    mode = "desktop";
#endif
    connect(this->rootObject(), SIGNAL(copyToClipboard(QString)), this, SLOT(setClipboard(QString)));
    connect(this->rootObject(), SIGNAL(changeUnit(int)), this, SLOT(setDisplayUnit(int)));
    connect(this->rootObject(), SIGNAL(lockWallet()), this, SLOT(lockWallet()));
    connect(this->rootObject(), SIGNAL(encrypt(QString)), this, SLOT(encryptWallet(QString)));
    connect(this->rootObject(), SIGNAL(lockWallet()), this, SLOT(lockWallet()));

    connect(engine, &QQmlEngine::quit, qApp, &QApplication::quit);
    connect(this->rootObject(), SIGNAL(quit()), this, SLOT(close()));
    connect(this, &QQuickView::close, this, &QApplication::quit);

    notificator = new Notificator(QApplication::applicationName(), nullptr, nullptr);
    rpcConsole = new RPCConsole(node, nullptr, nullptr);

    m_walletPane = this->rootObject()->findChild<QObject*>("walletPane");
    m_appPane = this->rootObject()->findChild<QObject*>("appWindow");

    m_AppDrawer = this->rootObject()->findChild<QObject*>("drawer");
    m_stakeChartPane = this->rootObject()->findChild<QObject*>("stakeChart");
    m_pricesWidget = this->rootObject()->findChild<QObject*>("pricesWidget");
    m_frontPage = this->rootObject()->findChild<QObject*>("frontPage");

    if (m_AppDrawer) {
        connect(m_AppDrawer, SIGNAL(request()), this, SLOT(requestRain()));
    }

    m_sendPane = this->rootObject()->findChild<QObject*>("sendPane");

    if (m_sendPane) {
        connect(m_sendPane, SIGNAL(send(QString, QString, QString)), this, SLOT(sendRain(QString, QString, QString)));
        connect(m_sendPane, SIGNAL(lockunlock(QString, int)), this, SLOT(lockunlock(QString, int)));
        connect(m_sendPane, SIGNAL(changesendbalance(QString)), this, SLOT(setSendBalance(QString)));
        connect(m_sendPane, SIGNAL(setcoincontroloutput(QString,int,bool)), this, SLOT(setCoinCotrolOutput(QString,int,bool)));
        connect(m_sendPane, SIGNAL(setrbf(bool)), this, SLOT(setCoinCotrolRbf(bool)));
        connect(m_sendPane, SIGNAL(setcoincontroltarget(int)), this, SLOT(setCoinCotrolConfirmTarget(int)));
    }

    m_consolePane = this->rootObject()->findChild<QObject*>("consolePane");

    if (m_consolePane) {
        connect(m_consolePane, SIGNAL(executeCommand(QString)), this, SLOT(executeRpcCommand(QString)));
    }

    m_stakeChartPage = this->rootObject()->findChild<QObject*>("stakeChartPage");

    if (m_stakeChartPage) {
        connect(m_stakeChartPage, SIGNAL(btnYearsClicked()), this, SLOT(setChartShow(YEAR)));
        connect(m_stakeChartPage, SIGNAL(btnMonthsClicked()), this, SLOT(setChartShow(MONTH)));
        connect(m_stakeChartPage, SIGNAL(btnAllClicked()), this, SLOT(setChartShow(ALL)));
    }


   // mnModel = new MNModel(this);

   // this->rootContext()->setContextProperty("mnModel", mnModel);

    timer = new QTimer(this);
   // connect(timer, SIGNAL(timeout()), this, SLOT(SetExchangeInfoTextLabels()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateStakingIcon()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNetworkState()));
    //connect(timer, &QTimer::timeout, [this]() {mnModel->updateMNList();});

    timer->start(300000);
    timerid = timer->timerId();
        LogPrintf("GUI: %d\n", 1);

    m_coin_control= new CCoinControl();
        LogPrintf("GUI: %d\n", 2);
    subscribeToCoreSignals();
}

void RainGUI::detectShutdown()
{
    if (m_node.shutdownRequested())
    {
        qApp->quit();
    }
}

void RainGUI::setClientModel(ClientModel *clientModel, interfaces::BlockAndHeaderTipInfo* tip_info)
{
    this->m_clientModel = clientModel;
    if(m_clientModel)
    {
        // Keep up to date with client
        updateNetworkState();
        connect(m_clientModel, &ClientModel::numConnectionsChanged, this, &RainGUI::setNumConnections);
        connect(m_clientModel, &ClientModel::networkActiveChanged, this, &RainGUI::setNetworkActive);

        //modalOverlay->setKnownBestHeight(_clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(_clientModel->getHeaderTipTime()));
        setNumBlocks(m_node.getNumBlocks(), QDateTime::fromTime_t(m_node.getLastBlockTime()), m_node.getVerificationProgress(), false);
        connect(m_clientModel, &ClientModel::numBlocksChanged, this, &RainGUI::setNumBlocks);

        // Propagate cleared model to child objects
        rpcConsole->setClientModel(m_clientModel);

        // Receive and report messages from client model
        connect(m_clientModel, &ClientModel::message, [this](const QString &title, const QString &message, unsigned int style){
            this->message(title, message, style);
        });
    }
}

void RainGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void RainGUI::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void RainGUI::updateNetworkState()
{
    int count = m_clientModel->getNumConnections();
    QString icon;
    switch(count)
    {
        case 0: icon = ":/icons/connect_0"; break;
        case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
        case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
        case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
        default: icon = ":/icons/connect_4"; break;
    }

    QString tooltip;
    if (m_node.getNetworkActive()) {
        tooltip = tr("%n active connection(s) to Rain network", "", count) + QString(".<br>") + tr("Click to disable network activity.");
    } else {
        tooltip = tr("Network activity disabled.") + QString("<br>") + tr("Click to enable network activity again.");
    }

    QMetaObject::invokeMethod(this->rootObject(), "updateNetwork", Q_ARG(QVariant, count));
    setNumBlocks(m_node.getNumBlocks(), QDateTime::fromTime_t(m_node.getLastBlockTime()), m_node.getVerificationProgress(), false);

}

#ifdef ENABLE_WALLET
void RainGUI::setWalletController(WalletController* wallet_controller)
{
    assert(!m_walletController);
    assert(wallet_controller);

    m_walletController = wallet_controller;

    connect(wallet_controller, &WalletController::walletAdded, this, &RainGUI::addWallet);
    connect(wallet_controller, &WalletController::walletRemoved, this, &RainGUI::removeWallet);

    // TODO: support more than just the default wallet
//    if (m_walletController->getOpenWallets()[0]) {
//        addWallet(m_walletController->getOpenWallets()[0]);
//    }

    for (WalletModel* wallet_model : m_walletController->getOpenWallets()) {
        addWallet(wallet_model);
    }

}

WalletController* RainGUI::getWalletController()
{
    return m_walletController;
}

void RainGUI::addWallet(WalletModel* walletModel)
{
    //const QString display_name = walletModel->getDisplayName();
    rpcConsole->addWallet(walletModel);
    setWalletModel(walletModel);
}

void RainGUI::removeWallet(WalletModel* walletModel)
{
    rpcConsole->removeWallet(walletModel);
}

void RainGUI::setCurrentWallet(WalletModel* wallet_model)
{
   // walletFrame->setCurrentWallet(wallet_model);

}

void RainGUI::removeAllWallets()
{

//    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

static void InitMessage(RainGUI *gui, const std::string &message)
{
    QMetaObject::invokeMethod(gui, "showInitMessage",
                              Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(message)),
                              Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
                              Q_ARG(QColor, QColor(55,55,55)));
}

static void ShowProgress(RainGUI *gui, const std::string &title, int nProgress)
{
    InitMessage(gui, title + strprintf("%d", nProgress) + "%");
}

static bool ThreadSafeMessageBox(RainGUI* gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    bool invoked = QMetaObject::invokeMethod(gui, "message", Qt::QueuedConnection,
                                             Q_ARG(QString, QString::fromStdString(caption)),
                                             Q_ARG(QString, QString::fromStdString(message)),
                                             Q_ARG(unsigned int, style));
    assert(invoked);
    return ret;
}

void RainGUI::subscribeToCoreSignals()
{
    // Connect signals to client
//    m_handler_message_box = m_node.handleMessageBox(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
//    m_handler_question = m_node.handleQuestion(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
//    m_handler_init_message = m_node.handleInitMessage(std::bind(InitMessage, this, std::placeholders::_1));
//    m_handler_show_progress = m_node.handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2));
}

void RainGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
//    m_handler_message_box->disconnect();
//    m_handler_question->disconnect();
//    m_handler_init_message->disconnect();
//    m_handler_show_progress->disconnect();
}

void RainGUI::showInitMessage(const QString &message, int alignment, const QColor &color)
{
    Q_UNUSED(alignment)
    Q_UNUSED(color)

    QVariant returnedValue;

    QMetaObject::invokeMethod(this->rootObject(), "showInitMessage",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, message));
}

void RainGUI::message(const QString& title, QString message, unsigned int style, bool* ret, const QString& detailed_message)
{
    // Default title. On macOS, the window title is ignored (as required by the macOS Guidelines).
    QString strTitle{"Rain"};
    // Default to information icon
   // int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;
    if (!title.isEmpty()) {
        msgType = title;
    } else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            message = tr("Error: %1").arg(message);
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            message = tr("Warning: %1").arg(message);
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            // No need to prepend the prefix here.
            break;
        default:
            break;
        }
    }

    if (!msgType.isEmpty()) {
        strTitle += " - " + msgType;
    }

    notificator->notify(static_cast<Notificator::Class>(nNotifyIcon), strTitle, message);

}

void RainGUI::splashFinished()
{
    QMetaObject::invokeMethod(this->rootObject(), "hideSplash");
}

void RainGUI::requestRain()
{
	
    QString address = m_walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, "", "", OutputType::BECH32);

    QMetaObject::invokeMethod(m_AppDrawer, "showQr",
                              Q_ARG(QVariant, address));
}

void RainGUI::requestAddress(QString purpose)
{
    QString userAddress = m_walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, purpose, "", OutputType::BECH32);
    QString aiAddress = m_walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, purpose, "", OutputType::BECH32);

    //QMetaObject::invokeMethod(, "setAddresses", Q_ARG(QVariant, userAddress), Q_ARG(QVariant, aiAddress));
}

void RainGUI::createAsset(QString sAssetName, QString sAssetShortName , QString addressTo, QString inputamount, QString assetToUse, QString outputamount, bool transferable, bool convertable, bool restricted, bool limited)
{


}

void RainGUI::convertAsset(QString addressTo, QString inputamount, QString inputsAssetName, QString outputamount, QString outputsAssetName)
{


}

void RainGUI::setBets(QString userAddress, QString aiAddress, QString amountt, QString sAssetName, QString sNonce)
{
    bool ok;
    uint32_t nNonce = sNonce.toInt(&ok);
    if (!ok) {
        LogPrintf(" FAILED TO GET nNONCE\n");
        return;
    }

    QString recovAddress = m_walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, "recovery", "", OutputType::BECH32);

    CTxDestination userdest = DecodeDestination(userAddress.toStdString());
    CTxDestination aidest = DecodeDestination(aiAddress.toStdString()), dest;
    CTxDestination recovdest = DecodeDestination(recovAddress.toStdString());

    CPubKey vchuPubKey, vchaPubKey, vchPubKey;
    PKHash upkhash = std::get<PKHash>(userdest);
    PKHash aipkhash = std::get<PKHash>(aidest);
    PKHash recovpkhash = std::get<PKHash>(recovdest);

    CKeyID ukeyID{ToKeyID(upkhash)};
    CKeyID aikeyID{ToKeyID(aipkhash)};
    CKeyID recovkeyID{ToKeyID(recovpkhash)};

    interfaces::Wallet& wallet = m_walletModel->wallet();
    CScript scriptPubKey;

    CHashWriter uhasher(SER_GETHASH, 0);
    uhasher << nNonce+0;
    uint256 uimage = uhasher.GetHash();

    CHashWriter ahasher(SER_GETHASH, 0);
    uhasher << nNonce+1;
    uint256 aimage = ahasher.GetHash();

    if(wallet.getPubKey(GetScriptForDestination(userdest), ukeyID, vchuPubKey) &&
     wallet.getPubKey(GetScriptForDestination(aidest), aikeyID, vchaPubKey) &&
     wallet.getPubKey(GetScriptForDestination(recovdest), ukeyID, vchPubKey))
        scriptPubKey = GetScriptForPvP(CScriptNum(100), vchuPubKey, vchaPubKey, vchPubKey, uimage, aimage);

    if (!ExtractDestination(scriptPubKey, dest)){
        LogPrintf(" FAILED to ExtractDestination \n");
        return;
    }

	if (!IsValidDestination(dest)) {
        LogPrintf(" Invalid Destination \n");
        return;
	}

    QString address = QString::fromStdString(EncodeDestination(dest));
    sendRain(address, amountt, sAssetName);

}

void RainGUI::sendRain(QString address, QString amountt, QString sAssetName)
{
    SendAssetsRecipient recipient;
    recipient.address = address;

    CAmount amount = 0;
    if (ParseFixedPoint(amountt.toStdString(), 8, &amount))
        recipient.amount = amount;

    for(auto const& x : passetsCache->GetItemsMap()){
        if(x.second->second.asset.IsExplicit()){
            LogPrintf("Logging assets %s vs %s\n", x.second->second.asset.GetAsset().getAssetName(), recipient.asset.getAssetName());
            if(QString::fromStdString(x.second->second.asset.GetAsset().getName()) == sAssetName){
                recipient.asset = x.second->second.asset.GetAsset();
            }
        }
    }
    LogPrintf("TESTING address = %s, amount = %d , asset= %s\n",address.toStdString(), amount, recipient.asset.getName() );

    QList<SendAssetsRecipient> recipientList;
    recipientList.append(recipient);

    WalletModelTransaction transaction(recipientList);

    WalletModel::SendCoinsReturn prepareStatus = m_walletModel->prepareTransaction(transaction, *m_coin_control);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus, RainUnits::formatWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), transaction.getTransactionFee()));

    if (prepareStatus.status != WalletModel::OK) {
        // TODO: Notify the user of type of failure
        return;
    }

    WalletModel::SendCoinsReturn sendStatus = m_walletModel->sendCoins(transaction);

    processSendCoinsReturn(sendStatus, RainUnits::formatWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), transaction.getTransactionFee()));

    if (sendStatus.status != WalletModel::OK)
    {
        LogPrintf("ERROR on SEND\n");
    }

    clearCoinCotrol();
}

const QStringList monthsNames = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void RainGUI::setWalletModel(WalletModel *model)
{
    if(model && model->getOptionsModel())
    {
        this->m_walletModel = model;
        //Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(m_walletModel->getTransactionTableModel());
       // filter->setLimit(50);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        this->rootContext()->setContextProperty("transactionsModel", filter.get());

        addressTablemodel = m_walletModel->getAddressTableModel();
        addrfilter = new AddressFilterProxyModel(AddressTableModel::Send, this);
        addrfilter->setSourceModel(addressTablemodel);
        addrfilter->sort(sortType, sortOrder);

        this->rootContext()->setContextProperty("addressTablemodel", addrfilter);

        // chart filter
        stakesFilter = new TransactionFilterProxy();
        stakesFilter->setDynamicSortFilter(true);
        stakesFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
        stakesFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
        stakesFilter->setSortRole(Qt::EditRole);
        stakesFilter->setOnlyStakes(true);
        stakesFilter->setSourceModel(m_walletModel->getTransactionTableModel());
        stakesFilter->sort(TransactionTableModel::Date, Qt::AscendingOrder);
        //hasStakes = stakesFilter->rowCount() > 0;

        this->rootContext()->setContextProperty("stakemodel", stakesFilter);

        //assetFilter = new AssetFilterProxy();
        //assetFilter->setSourceModel();
        //assetFilter->sort(AssetTableModel::NameRole, Qt::DescendingOrder);
        this->rootContext()->setContextProperty("assettable_model", m_walletModel->getAssetTableModel());
        this->rootContext()->setContextProperty("coincontrolmodel", m_walletModel->getCoinControlModel());

        // Keep up to date with wallet
        interfaces::Wallet& wallet = m_walletModel->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(m_walletModel, &WalletModel::balanceChanged, this, &RainGUI::setBalance);

        connect(m_walletModel->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &RainGUI::updateDisplayUnit);

        // Receive and report messages from wallet model
        connect(m_walletModel, &WalletModel::message, [this](const QString &title, const QString &message, unsigned int style){
            this->message(title, message, style);
        });

        // Handle changes in encryption status
        connect(m_walletModel, &WalletModel::encryptionStatusChanged, this, &RainGUI::updateWalletStatus);

        // Balloon pop-up for new transaction
        connect(m_walletModel->getTransactionTableModel(), &TransactionTableModel::rowsInserted, this, &RainGUI::processNewTransaction);

        // Ask for passphrase if needed
        //connect(_walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));
       //connect(model, &WalletModel::requireUnlock, this, &RainGUI::unlockWallet);
       //mnModel->setWalletModel(m_walletModel);
    }


    this->messageModel = new MessageModel(m_walletModel);
    if(messageModel)
    {
        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(messageModel);
        proxyModel->setDynamicSortFilter(true);
        proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->sort(MessageModel::ColumnIndex::ReceivedDateTime, Qt::DescendingOrder);
        this->rootContext()->setContextProperty("messageModel", proxyModel);
        // Balloon pop-up for new message
//        connect(messageModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(processNewMessage(QModelIndex,int,int)));
    }

    // Refresh years filter, first address created is the start
    int currentYear = QDateTime::currentDateTime().date().year();
    QList<QString> yearsModel;

    for(int i = 2019;  i < currentYear +1; i++)
        yearsModel.append(QString::number(i));

    this->rootContext()->setContextProperty("yearsModel", QVariant(yearsModel));
    AssetList();

    updateDisplayUnit();
    updateWalletStatus();

}

void RainGUI::updateWalletStatus()
{
    setEncryptionStatus(m_walletModel->getEncryptionStatus());
    setHDStatus(m_walletModel->wallet().privateKeysDisabled(), m_walletModel->wallet().hdEnabled());

    this->rootContext()->setContextProperty("EncryptionStatus", QVariant(m_walletModel->getEncryptionStatus()));
    this->rootContext()->setContextProperty("HDStatus", QVariant(m_walletModel->wallet().privateKeysDisabled()));
    QMetaObject::invokeMethod(this->rootObject(), "changeStatus");
}

void RainGUI::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = m_walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;

    CAmountMap tmp;
    for (auto bal: m_balances.balance){
        if (bal.first == Params().GetConsensus().subsidy_asset)
            tmp.insert(bal);
    }


    QMetaObject::invokeMethod(m_frontPage, "updateBalance",
                              Q_ARG(QVariant, formatMultiAssetAmount(tmp, unit, RainUnits::SeparatorStyle::ALWAYS, "")),
                              Q_ARG(QVariant, formatMultiAssetAmount(balances.immature_balance, unit, RainUnits::SeparatorStyle::ALWAYS, "\n")),
                              Q_ARG(QVariant, formatMultiAssetAmount(balances.unconfirmed_balance, unit, RainUnits::SeparatorStyle::ALWAYS, "\n")),
                              Q_ARG(QVariant, formatMultiAssetAmount(balances.stake, unit, RainUnits::SeparatorStyle::ALWAYS, "\n")));

   // SetExchangeInfoTextLabels();
    AssetList();
}

void RainGUI::setSendBalance(QString assetname)
{
    int unit = m_walletModel->getOptionsModel()->getDisplayUnit();
    CAmountMap tmp;
    for (auto bal: m_balances.balance){
        if (bal.first.getName() == assetname.toStdString())
            tmp.insert(bal);
    }

    QMetaObject::invokeMethod(m_sendPane, "setsendbalance", Q_ARG(QVariant, formatMultiAssetAmount(tmp, unit, RainUnits::SeparatorStyle::ALWAYS, "")));
}

void RainGUI::setClipboard(QString text)
{
    GUIUtil::setClipboard(text);
}

void RainGUI::setDisplayUnit(int index)
{
    if(m_walletModel && m_walletModel->getOptionsModel())
    {
        m_walletModel->getOptionsModel()->setDisplayUnit(index);
    }
    setBalance(m_balances);
}

void RainGUI::executeRpcCommand(QString command)
{
    QString response;

    try
    {
        std::string result;
        std::string executableCommand = command.toStdString() + "\n";

        if (!rpcConsole->RPCExecuteCommandLine(m_node, result, executableCommand, nullptr, m_walletModel)) {
            response = "Parse error: unbalanced ' or \"";
        }
        else {
            response = QString::fromStdString(result);
        }
    }
    catch (UniValue& objError)
    {
        try // Nice formatting for standard-format error
        {
            int code = find_value(objError, "code").get_int();
            std::string message = find_value(objError, "message").get_str();
            response =  QString::fromStdString(message) + " (code " + QString::number(code) + ")";
        }
        catch (const std::runtime_error&) // raised when converting to invalid type, i.e. missing code or message
        {   // Show raw JSON object
            response = QString::fromStdString(objError.write());
        }
    }
    catch (const std::exception& e)
    {
        response = QString("Error: ") + QString::fromStdString(e.what());
    }

    QMetaObject::invokeMethod(m_consolePane, "showReply", Q_ARG(QVariant, response));
}

void RainGUI::updateDisplayUnit()
{
    if(m_walletModel && m_walletModel->getOptionsModel())
    {
        if (m_balances.balance != CAmountMap()) {
            setBalance(m_balances);
        }

        m_displayUnit = m_walletModel->getOptionsModel()->getDisplayUnit();
        this->rootContext()->setContextProperty("displayUnit", m_displayUnit);
    }
}

const QString marketdetails = "https://explorer.alqo.app/api/getmarketinfo";

QString dequote(QString s)
{
    std::string str(s.toStdString());
    boost::xpressive::sregex nums = boost::xpressive::sregex::compile(":\\\"(-?\\d*(\\.\\d+))\\\"");
    std::string nm(":$1");
    str = regex_replace(str, nums, nm);
    boost::xpressive::sregex tru = boost::xpressive::sregex::compile("\\\"true\\\"");
    std::string tr("true");
    str = regex_replace(str, tru, tr);
    boost::xpressive::sregex fal = boost::xpressive::sregex::compile("\\\"false\\\"");
    std::string fl("false");
    str = regex_replace(str, fal, fl);
    QString res = str.c_str();
    return res;
}

QString RainGUI::sendRequest(QString url)
{
    QString Response = "";
    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req = QNetworkRequest(QUrl(url));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    req.setRawHeader("User-Agent", "AlQO Wallet"); //set header
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);


    QNetworkReply* reply = mgr.get(req);
    eventLoop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        Response = reply->readAll();
        delete reply;
    } else {
        Response = "Error";
    //    QMessageBox::information(this,"Error",reply->errorString());

        delete reply;
    }

    return Response;
}

void RainGUI::SetExchangeInfoTextLabels()
{
#ifndef WIN32
    // Get the current exchange information
    QString str = "";
    QString response = dequote(sendRequest(marketdetails));

    // parse the json result to get values.
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());       //get json from str.
    QJsonObject obj = jsonResponse.object();

    QJsonObject objmetrics = obj["metrics"].toArray().first().toObject();
    QJsonObject prices = obj["base_prices"].toObject();

    QJsonDocument doc(prices);
    QString strJson(doc.toJson(QJsonDocument::Compact));

    double newbtc = objmetrics["latest"].toDouble();
    double newusd = prices["USD"].toDouble();
    double newxlq = newusd * newbtc;

    CAmount tally =0;
    for(auto&balance: m_balances.balance){
        CAmount temp = balance.second;// * balance.first.floor;
        tally+=temp;
    }

    double newwalletvalue = (tally/COIN * newxlq);

    QMetaObject::invokeMethod(m_pricesWidget, "updateValue", Q_ARG(QVariant,"1 BTC = $ " + str.number(newusd, 'i', 2)),
     Q_ARG(QVariant, "1 RAIN = $ " + str.number(newxlq, 'i', 2)));

    QMetaObject::invokeMethod(m_frontPage, "updateWalletValue", Q_ARG(QVariant, str.number(newwalletvalue, 'i', 2) + " USD" ));

    obj.empty();
    objmetrics.empty();
    prices.empty();
#endif
}

void RainGUI::onSortChanged(int idx)
{
  //  sortType = (AddressTableModel::ColumnIndex) ;
    sortAddresses();
}

void RainGUI::onSortOrderChanged(int idx)
{
   // sortOrder = (Qt::SortOrder) ;
    sortAddresses();
}

void RainGUI::sortAddresses()
{
    if (addrfilter)
        addrfilter->sort(sortType, sortOrder);
}

void RainGUI::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid. Please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed);
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AbsurdFee:
        msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(RainUnits::formatWithUnit(m_walletModel->getOptionsModel()->getDisplayUnit(), m_walletModel->wallet().getDefaultMaxTxFee()));
        break;
    // included to prevent a compiler warning.
    case WalletModel::TransactionCheckFailed:
        msgParams.first = tr("TransactionCheckFailed!");
        break;
    case WalletModel::StakingOnlyUnlocked:
        msgParams.first = tr("StakingOnlyUnlocked!");
        break;
    case WalletModel::CannotCreateInternalAddress:
        msgParams.first = tr("CannotCreateInternalAddress!");
        break;
    case WalletModel::OK:
        msgParams.first = tr("SENT!");
        break;
      default:
        return;
    }
    LogPrintf("error : %s \n", msgParams.first.toStdString());
    message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void RainGUI::AssetList(){
    assetListModel.clear();
    for(auto const& x : passetsCache->GetItemsMap()){
        if(x.second->first !="UNDEFINED")
            assetListModel.append(QString::fromStdString(x.second->first));
//        assetListModel.append(QString::fromStdString(x.second->second.asset.GetAsset().getName()));
    }

    this->rootContext()->setContextProperty("assetListModel", QVariant(assetListModel));
}

void RainGUI::close(){
    //QApplication::quit();
    qApp->quit();
}

void RainGUI::processNewTransaction(const QModelIndex& parent, int start, int /*end*/)
{
    // Prevent balloon-spam when initial block download is in progress
    if (!m_walletModel || !m_clientModel || m_clientModel->node().isInitialBlockDownload())
        return;

    TransactionTableModel *ttm = m_walletModel->getTransactionTableModel();
    if (!ttm || ttm->processingQueuedTransactions())
        return;

    QString date = ttm->index(start, TransactionTableModel::Date, parent).data().toString();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent).data(Qt::EditRole).toULongLong();
    QString type = ttm->index(start, TransactionTableModel::Type, parent).data().toString();
    QModelIndex index = ttm->index(start, 0, parent);
    QString address = ttm->data(index, TransactionTableModel::AddressRole).toString();
    QString label = ttm->data(index, TransactionTableModel::LabelRole).toString();

    incomingTransaction(date, m_walletModel->getOptionsModel()->getDisplayUnit(), amount, type, address, label, m_walletModel->getWalletName());
}

void RainGUI::processNewMessage(const QModelIndex& parent, int start, int /*end*/)
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

    incomingMessage(sent_datetime, from_address, to_address, message, type);
}

void RainGUI::updateEncryptionStatus()
{
    encryptionStatusChanged();
}

void RainGUI::encryptWallet(QString pass)
{
    if(!m_walletModel)
        return;
    SecureString ppass;
    ppass.assign(pass.toStdString().c_str());
   // bool res = m_walletModel->setWalletEncrypted(ppass);

    updateEncryptionStatus();
}

void RainGUI::backupWallet()
{
    //TODO
    QString filename ="aa" ;//GUIUtil::getSaveFileName(this, tr("Backup Wallet"), QString(),  tr("Wallet Data (*.dat)"), nullptr);

    if (filename.isEmpty())
        return;

    if (!m_walletModel->wallet().backupWallet(filename.toLocal8Bit().data())) {
        message(tr("Backup Failed"), tr("There was an error trying to save the wallet data to %1.").arg(filename),
            CClientUIInterface::MSG_ERROR);
        }
    else {
        message(tr("Backup Successful"), tr("The wallet data was successfully saved to %1.").arg(filename),
            CClientUIInterface::MSG_INFORMATION);
    }
}

void RainGUI::changePassphrase(QString oldpass, QString newpass)
{
    SecureString opass, npass;
    opass.assign(oldpass.toStdString().c_str());
    npass.assign(newpass.toStdString().c_str());

    if(m_walletModel->changePassphrase(opass, npass))
        updateWalletStatus();
}

void RainGUI::unlockWallet(QString pass, bool stakingonly)
{
    if(!m_walletModel)
        return;
    // Unlock wallet when requested by wallet model
    bool res = false;
    SecureString opass;
    opass.assign(pass.toStdString().c_str());
    if (m_walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        res = m_walletModel->setWalletLocked(false, opass, stakingonly);
    }
    updateWalletStatus();
}

void RainGUI::lockWallet()
{
    if(!m_walletModel)
        return;
    if (m_walletModel->getEncryptionStatus() != WalletModel::Locked)
    {
        m_walletModel->setWalletLocked(true);
    }
    updateWalletStatus();
}

void RainGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) + tr("Amount: %1\n").arg(RainUnits::formatWithUnit(unit, amount, true));
    if (m_node.walletClient().getWallets().size() > 1 && !walletName.isEmpty()) {
        msg += tr("Wallet: %1\n").arg(walletName);
    }
    msg += tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"), msg, CClientUIInterface::MSG_INFORMATION);
}

void RainGUI::incomingMessage(const QString& sent_datetime, QString from_address, QString to_address, QString message, int type)
{
/*
    if (type == MessageTableEntry::Received)
    {
        notificator->notify(Notificator::Information,
                            tr("Incoming Message"),
                            tr("Date: %1\n"
                               "From Address: %2\n"
                               "To Address: %3\n"
                               "Message: %4\n")
                              .arg(sent_datetime)
                              .arg(from_address)
                              .arg(to_address)
                              .arg(message));
    };
*/
}

void RainGUI::updateStakingIcon()
{
    if(m_node.shutdownRequested())
        return;

    uint64_t nWeight = 0;
    if(m_walletModel)
        nWeight = m_walletModel->getStakeWeight();

    QString tooltip="";
    bool staking = false;
    if (m_walletModel && m_walletModel->wallet().getLastCoinStakeSearchInterval() && nWeight)
    {
        uint64_t nNetworkWeight = GetPoSKernelPS();
        const Consensus::Params& consensusParams = Params().GetConsensus();
        int64_t nTargetSpacing = consensusParams.nPowTargetSpacing;

        unsigned nEstimateTime = nTargetSpacing * nNetworkWeight / nWeight;

        QString text;
        if (nEstimateTime < 60)
        {
            text = tr("%n second(s)", "", nEstimateTime);
        }
        else if (nEstimateTime < 60*60)
        {
            text = tr("%n minute(s)", "", nEstimateTime/60);
        }
        else if (nEstimateTime < 24*60*60)
        {
            text = tr("%n hour(s)", "", nEstimateTime/(60*60));
        }
        else
        {
            text = tr("%n day(s)", "", nEstimateTime/(60*60*24));
        }

        nWeight /= COIN;
        nNetworkWeight /= COIN;

        staking =true;

        tooltip = tr("Stake weight is %1<br>Network weight is %2<br>Expected time is %3").arg(nWeight).arg(nNetworkWeight).arg(text);
    }
    else
    {
        QString text;
        if (m_node.getNodeCount(CConnman::CONNECTIONS_ALL) == 0)
            text = "Not staking because wallet is offline";
        else if (m_node.isInitialBlockDownload())
            text = "Not staking because wallet is syncing";
        else if (!nWeight)
            text = "Not staking because you don't have mature coins";
        else if (m_walletModel->wallet().isLocked())
            text = "Not staking because wallet is locked";
        else
            text = "Not staking";

        tooltip=text;
    }

    QMetaObject::invokeMethod(this->rootObject(), "updateStaking", Q_ARG(QVariant, staking), Q_ARG(QVariant, tooltip));

}

void RainGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{

    if (!m_clientModel)
        return;

    // Acquire current block source
    enum BlockSource blockSource = m_clientModel->getBlockSource();
    QString tooltip;
    bool synced = false;
    switch (blockSource) {
        case BlockSource::NETWORK:
            if (header) {
                int64_t headersTipTime = m_clientModel->getHeaderTipTime();
                int headersTipHeight = m_clientModel->getHeaderTipHeight();
                int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
                if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
                    tooltip =(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
                return;
            }
            tooltip = "Synchronizing with network...";
            break;
        case BlockSource::DISK:
            if (header) {
                tooltip = "Indexing blocks on disk...";
            } else {
                tooltip = "Processing blocks on disk...";
            }
            break;
        case BlockSource::REINDEX:
            tooltip = "Reindexing blocks on disk...";
            break;
        case BlockSource::NONE:
            if (header) {
                return;
            }
            tooltip = "Connecting to peers...";
            break;
    }

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = (tr(" Processed %n block(s) of transaction history.", "", count));

    // Set icon state: spinning if catching up, tick otherwise
    if (secs < MAX_BLOCK_TIME_GAP) {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        synced = true;
    }
    else
    {
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);
        synced = false;

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;

        prevBlocks = count;

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    QMetaObject::invokeMethod(this->rootObject(), "updateBlocks", Q_ARG(QVariant, synced), Q_ARG(QVariant, tooltip));

}

void RainGUI::setHDStatus(bool privkeyDisabled, int hdEnabled)
{
    hdstatus = hdEnabled;
    privkeys = privkeyDisabled;
}

void RainGUI::setEncryptionStatus(int status)
{
    encryptionstatus = status;
}

// context menu action: lock coin
void RainGUI::lockunlock(QString txhash, int n)
{
    LogPrintf("LOCKUNLOCK %s , %d\n", txhash.toStdString(), n);
    COutPoint outpt(uint256S(txhash.toStdString()), n);
    bool locked = m_walletModel->wallet().isLockedCoin(outpt);
    if(locked)
        m_walletModel->unlockCoin(outpt);
    else
        m_walletModel->lockCoin(outpt);
}

void RainGUI::clearCoinCotrol(){
    m_coin_control->SetNull();
}

void RainGUI::setCoinCotrolOutput(QString txhash, int n, bool set){

    LogPrintf("SETCOINCONTROLOUTPUT %s , %d , %s\n", txhash.toStdString(), n, set ? "true":"false");

    COutPoint outpt(uint256S(txhash.toStdString()), n);
    if(set)
        m_coin_control->Select(outpt);
    else
        m_coin_control->UnSelect(outpt);
}

void RainGUI::setCoinCotrolRbf(bool set){
    LogPrintf("SETTINGCOINCONTROL RBF %s\n", set ? "true" :"false");
    m_coin_control->m_signal_bip125_rbf = set;
}

void RainGUI::setCoinCotrolConfirmTarget(int num){
    LogPrintf("SETTINGCOINCONTROL TARGET %d\n", num);
    m_coin_control->m_confirm_target = num;
}

FileIO::FileIO(QObject *parent)
    : QObject(parent)
{
}

FileIO::~FileIO()
{
}

void FileIO::read()
{
    if(m_source.isEmpty()) {
        return;
    }
    QFile file(m_source.toLocalFile());
    if(!file.exists()) {
        qWarning() << "Does not exits: " << m_source.toLocalFile();
        return;
    }
    if(file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        m_text = stream.readAll();
        Q_EMIT textChanged(m_text);
    }
}

void FileIO::write()
{
    if(m_source.isEmpty()) {
        return;
    }
    QFile file(m_source.toLocalFile());
    if(file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << m_text;
    }
}

QUrl FileIO::source() const
{
    return m_source;
}

QString FileIO::text() const
{
    return m_text;
}

void FileIO::setSource(QUrl source)
{
    if (m_source == source)
        return;

    m_source = source;
    Q_EMIT sourceChanged(source);
}

void FileIO::setText(QString text)
{
    if (m_text == text)
        return;

    m_text = text;
    Q_EMIT textChanged(text);
}

