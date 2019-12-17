#include "rainmobilegui.h"

#include <init.h>
#include <univalue.h>

#include <qt/guiutil.h>
#include <qt/walletcontroller.h>
#include <qt/networkstyle.h>
#include <qt/platformstyle.h>
#include <qt/addresstablemodel.h>
#include <qt/qrimagewidget.h>
#include <qt/transactiontablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/rainunits.h>
#include <qt/walletmodel.h>
#include <qt/rpcconsole.h>

#include <wallet/wallet.h>
#include <wallet/coincontrol.h>

#include <interfaces/node.h>

#include <QQuickItem>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickImageProvider>

static const std::array<int, 9> confTargets = { {2, 4, 6, 12, 24, 48, 144, 504, 1008} };

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

RainMobileGUI::RainMobileGUI(interfaces::Node& node, const PlatformStyle *_platformStyle, const NetworkStyle *networkStyle, QWindow *parent) :
    QQuickView(parent),
    m_node(node),
    m_platformStyle(_platformStyle),
    m_networkStyle(networkStyle)
{
    QQmlEngine *engine = this->engine();
    engine->addImageProvider(QLatin1String("icons"), new IconProvider(networkStyle));
    engine->addImageProvider(QLatin1String("qr"), new QrProvider());

    QQuickStyle::setStyle("Material");

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    this->setSource(url);
    this->setTitle(networkStyle->getTitleAddText());
    this->setHeight(2160/3);
    this->setWidth(1080/3);

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

    for (const int n : confTargets) {
        confirmationTargets.insert(QString::number(n),
                                   QCoreApplication::translate("SendCoinsDialog", "%1 (%2 blocks)")
                                   .arg(GUIUtil::formatNiceTimeOffset(n*10*60))
                                   .arg(n));
    }

    this->rootContext()->setContextProperty("availableUnits", QVariant(availableUnitNames));
    this->rootContext()->setContextProperty("confirmationTargets", QVariant(confirmationTargets));
    this->rootContext()->setContextProperty("licenceInfo", licenseInfoHTML);
    this->rootContext()->setContextProperty("version", QString::fromStdString(FormatFullVersion()));

    connect(this->rootObject(), SIGNAL(copyToClipboard(QString)), this, SLOT(setClipboard(QString)));
    connect(this->rootObject(), SIGNAL(changeUnit(int)), this, SLOT(setDisplayUnit(int)));

    m_walletPane = this->rootObject()->findChild<QObject*>("walletPane");

    if (m_walletPane) {
        connect(m_walletPane, SIGNAL(request()), this, SLOT(requestRain()));
    }

    m_sendPane = this->rootObject()->findChild<QObject*>("sendPane");

    if (m_sendPane) {
        connect(m_sendPane, SIGNAL(send(QString, int, int)), this, SLOT(sendRain(QString, int, int)));
    }

    m_consolePane = this->rootObject()->findChild<QObject*>("consolePane");

    if (m_consolePane) {
        connect(m_consolePane, SIGNAL(executeCommand(QString)), this, SLOT(executeRpcCommand(QString)));
    }

    subscribeToCoreSignals();
}

void RainMobileGUI::setClientModel(ClientModel *clientModel)
{
    this->m_clientModel = clientModel;
}

void RainMobileGUI::setWalletController(WalletController* wallet_controller)
{
    assert(!m_walletController);
    assert(wallet_controller);

    m_walletController = wallet_controller;

    // TODO: support more than just the default wallet
    if (m_walletController->getOpenWallets()[0]) {
        addWallet(m_walletController->getOpenWallets()[0]);
    }
}

void RainMobileGUI::addWallet(WalletModel* walletModel)
{
    const QString display_name = walletModel->getDisplayName();
    setWalletModel(walletModel);
}

static void InitMessage(RainMobileGUI *gui, const std::string &message)
{
    QMetaObject::invokeMethod(gui, "showInitMessage",
                              Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(message)),
                              Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
                              Q_ARG(QColor, QColor(55,55,55)));
}

static void ShowProgress(RainMobileGUI *gui, const std::string &title, int nProgress)
{
    InitMessage(gui, title + strprintf("%d", nProgress) + "%");
}

static bool ThreadSafeMessageBox(RainMobileGUI* gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    bool invoked = QMetaObject::invokeMethod(gui, "message",
                                             modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                                             Q_ARG(QString, QString::fromStdString(caption)),
                                             Q_ARG(QString, QString::fromStdString(message)),
                                             Q_ARG(unsigned int, style),
                                             Q_ARG(bool*, &ret));
    assert(invoked);
    return ret;
}

void RainMobileGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_message_box = m_node.handleMessageBox(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    m_handler_question = m_node.handleQuestion(std::bind(ThreadSafeMessageBox, this, std::placeholders::_1, std::placeholders::_3, std::placeholders::_4));
    m_handler_init_message = m_node.handleInitMessage(std::bind(InitMessage, this, std::placeholders::_1));
    m_handler_show_progress = m_node.handleShowProgress(std::bind(ShowProgress, this, std::placeholders::_1, std::placeholders::_2));
}

void RainMobileGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_message_box->disconnect();
    m_handler_question->disconnect();
    m_handler_init_message->disconnect();
    m_handler_show_progress->disconnect();
}

void RainMobileGUI::showInitMessage(const QString &message, int alignment, const QColor &color)
{
    Q_UNUSED(alignment)
    Q_UNUSED(color)

    QVariant returnedValue;

    QMetaObject::invokeMethod(this->rootObject(), "showInitMessage",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, message));
}

void RainMobileGUI::message(const QString& title, QString message, unsigned int style, bool* ret)
{
    Q_UNUSED(title)
    Q_UNUSED(message)
    Q_UNUSED(style)
    Q_UNUSED(ret)

    // TODO: Show messages to the user
}

void RainMobileGUI::splashFinished()
{
    QMetaObject::invokeMethod(this->rootObject(), "hideSplash");
}

void RainMobileGUI::requestRain()
{
    QString address = m_walletModel->getAddressTableModel()->addRow(AddressTableModel::Receive, "", "", OutputType::BECH32);

    QMetaObject::invokeMethod(m_walletPane, "showQr",
                              Q_ARG(QVariant, address));
}

void RainMobileGUI::sendRain(QString address, int amount, int confirmations)
{
    Q_UNUSED(confirmations)

    SendCoinsRecipient recipient;
    recipient.address = address;
    recipient.amount = amount;

    QList<SendCoinsRecipient> recipientList;
    recipientList.append(recipient);

    WalletModelTransaction transaction(recipientList);
    WalletModel::SendCoinsReturn prepareStatus;

    CCoinControl ctrl;

    ctrl.m_confirm_target = confirmations;
    ctrl.m_signal_bip125_rbf = true;

    prepareStatus = m_walletModel->prepareTransaction(transaction, ctrl);

    if (prepareStatus.status != WalletModel::OK) {
        // TODO: Notify the user of type of failure
        return;
    }

    WalletModel::SendCoinsReturn sendStatus = m_walletModel->sendCoins(transaction);

    qDebug() << sendStatus.status;

    if (sendStatus.status == WalletModel::OK)
    {}
}

void RainMobileGUI::setWalletModel(WalletModel *model)
{
    this->m_walletModel = model;
    if(model && model->getOptionsModel())
    {
        //Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(50);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        this->rootContext()->setContextProperty("transactionsModel", filter.get());

        // Keep up to date with wallet
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, &WalletModel::balanceChanged, this, &RainMobileGUI::setBalance);

        connect(model->getOptionsModel(), &OptionsModel::displayUnitChanged, this, &RainMobileGUI::updateDisplayUnit);
    }

    updateDisplayUnit();
}

void RainMobileGUI::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = m_walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;

    QMetaObject::invokeMethod(m_walletPane, "updateBalance",
                              Q_ARG(QVariant, RainUnits::formatWithUnit(unit, balances.balance, false, RainUnits::separatorAlways)));
}

void RainMobileGUI::setClipboard(QString text)
{
    GUIUtil::setClipboard(text);
}

void RainMobileGUI::setDisplayUnit(int index)
{
    if(m_walletModel && m_walletModel->getOptionsModel())
    {
        m_walletModel->getOptionsModel()->setDisplayUnit(index);
    }
}

void RainMobileGUI::executeRpcCommand(QString command)
{
    QString response;

    try
    {
        std::string result;
        std::string executableCommand = command.toStdString() + "\n";

        if (!RPCConsole::RPCExecuteCommandLine(m_node, result, executableCommand, nullptr, m_walletModel)) {
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

void RainMobileGUI::updateDisplayUnit()
{
    if(m_walletModel && m_walletModel->getOptionsModel())
    {
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

        m_displayUnit = m_walletModel->getOptionsModel()->getDisplayUnit();
        this->rootContext()->setContextProperty("displayUnit", m_displayUnit);
    }
}

