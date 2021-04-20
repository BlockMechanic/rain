#include <qt/guiutil.h>
#include <qt/guiconstants.h>
#include <qt/rainunits.h>
#include <qt/optionsmodel.h>
#include <qt/walletmodel.h>
#include <qt/messagemodel.h>
#include <qt/addresstablemodel.h>
#include <pubkey.h>
#include <key_io.h>
#include <node/ui_interface.h>
#include <base58.h>
#include <init.h>
#include <util/system.h>
#include <smsg/db.h>

#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <QSortFilterProxyModel>
#include <QClipboard>
#include <QMessageBox>
#include <QMenu>
#include <QFont>
#include <QColor>

#include <db_cxx.h>
#include <wallet/coincontrol.h>

Q_DECLARE_METATYPE(std::vector<unsigned char>);

QList<QString> ambiguous; /**< Specifies Ambiguous addresses */

const QString MessageModel::Sent = "Sent";
const QString MessageModel::Received = "Received";

struct MessageTableEntryLessThan
{
    bool operator()(const MessageTableEntry &a, const MessageTableEntry &b) const {return a.received_datetime < b.received_datetime;}
    bool operator()(const MessageTableEntry &a, const QDateTime         &b) const {return a.received_datetime < b;}
    bool operator()(const QDateTime         &a, const MessageTableEntry &b) const {return a < b.received_datetime;}
};
// Private implementation
class MessageTablePriv
{
public:
    QList<MessageTableEntry> cachedMessageTable;
    MessageModel *parent;

    MessageTablePriv(WalletModel *walletModel, MessageModel *parent):
        walletModel(walletModel), parent(parent) {}

    void refreshMessageTable()
    {

        cachedMessageTable.clear();
        if (parent->getWalletModel()->getEncryptionStatus() == WalletModel::Locked)
        {
            // -- messages are stored encrypted, can't load them without the private keys
            return;
        }

        std::vector<unsigned char> vchKey;

        {
            LOCK(smsg::cs_smsgDB);

            smsg::SecMsgDB dbSmsg;

            if (!dbSmsg.Open("cr+")){
                throw std::runtime_error("Could not open DB.");
            }
            vchKey.resize(30);
            uint32_t nMessages = 0;
            uint8_t chKey[30];
            int fCheckReadStatus = 1;
            bool update_status = true;

            smsg::SecMsgStored smsgStored;
            smsg::MessageData msg;
            QString label;
            QDateTime sent_datetime;
            QDateTime received_datetime;

            dbSmsg.TxnBegin();

            leveldb::Iterator* it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());

            while (dbSmsg.NextSmesg(it, smsg::DBK_INBOX, chKey, smsgStored))
            {

                if (fCheckReadStatus && !(smsgStored.status & SMSG_MASK_UNREAD)) {
                    continue;
                }
                const unsigned char *pHeader = smsgStored.vchMessage.data();
                smsg::SecureMessage smsg(pHeader);

                uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                int rv = smsgModule.Decrypt(false, smsgStored.addrTo, pHeader, pHeader + smsg::SMSG_HDR_LEN, nPayload, msg);
                if (rv == 0) {
                    std::string sAddrTo = EncodeDestination(PKHash(smsgStored.addrTo));
                    std::string sText = std::string((char*)msg.vchMessage.data());
                    sent_datetime    .setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString(msg.sFromAddress.c_str()));
                    memcpy(&vchKey[0], chKey, 30);
                    handleMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Received,
                                                      label,
                                                      QString::fromStdString(EncodeDestination(PKHash(smsgStored.addrTo))),
                                                      QString(msg.sFromAddress.c_str()),
                                                      sent_datetime,
                                                      received_datetime,
                                                      !(smsgStored.status & SMSG_MASK_UNREAD),
                                                      sText.c_str()),
                                                      true);
                }
                // Only set 'read' status if the message decrypted successfully and update_status is set
                if (fCheckReadStatus && rv == 0 && update_status) {
                    smsgStored.status &= ~SMSG_MASK_UNREAD;
                    dbSmsg.WriteSmesg(chKey, smsgStored);
                }
                nMessages++;
            }
            delete it;

            it = dbSmsg.pdb->NewIterator(leveldb::ReadOptions());
            while (dbSmsg.NextSmesg(it, smsg::DBK_OUTBOX, chKey, smsgStored)) {
                const unsigned char *pHeader = smsgStored.vchMessage.data();
                smsg::SecureMessage smsg(pHeader);

                uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;
                int rv = smsgModule.Decrypt(false, smsgStored.addrOutbox, pHeader, pHeader + smsg::SMSG_HDR_LEN, nPayload, msg);
                if (rv == 0) {
                    std::string sAddrTo = EncodeDestination(PKHash(smsgStored.addrTo));
                    std::string sText = std::string((char*)msg.vchMessage.data());
                    sent_datetime.setTime_t(msg.timestamp);
                    received_datetime.setTime_t(smsgStored.timeReceived);
                    label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(EncodeDestination(PKHash(smsgStored.addrTo))));
                    memcpy(&vchKey[0], chKey, 30);
                    handleMessageEntry(MessageTableEntry(vchKey,
                                                      MessageTableEntry::Sent,
                                                      label,
                                                      QString::fromStdString(EncodeDestination(PKHash(smsgStored.addrTo))),
                                                      QString(msg.sFromAddress.c_str()),
                                                      sent_datetime,
                                                      received_datetime,
                                                      !(smsgStored.status & SMSG_MASK_UNREAD),
                                                      sText.c_str()),
                                    true);

                }
                nMessages++;
            }
            delete it;
            dbSmsg.TxnCommit();
        }

    }

    void newMessage(const smsg::SecMsgStored& inboxHdr)
    {
        // we have to copy it, because it doesn't like constants going into Decrypt
        smsg::SecMsgStored smsgStored = inboxHdr;
        //SecMsgStored &smsgInbox = const_cast<SecMsgStored&>(inboxHdr); // un-const the reference

        smsg::MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;
        interfaces::Wallet& wallet = walletModel->wallet();

        std::string errorMsg;
        const unsigned char *pHeader = smsgStored.vchMessage.data();
        smsg::SecureMessage smsg(pHeader);
        uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;

        int rv = smsgModule.Decrypt(false, smsgStored.addrOutbox, pHeader, pHeader + smsg::SMSG_HDR_LEN, nPayload, msg);
        if (rv == 0) {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString(msg.sFromAddress.c_str()));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::vector<unsigned char> vchKey;
            std::string sPrefix("im");
            vchKey.resize(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &msg.timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[smsg::SMSG_HDR_LEN], 8);    // sample
                    std::string sText = std::string((char*)msg.vchMessage.data());

            handleMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Received,
                                              label,
                                              QString::fromStdString(EncodeDestination(PKHash(smsgStored.addrTo))),
                                              msg.sFromAddress.c_str(),
                                              sent_datetime,
                                              received_datetime,
                                              !(smsgStored.status & SMSG_MASK_UNREAD),
                                              sText.c_str()),
                            false);
        }
    }

    void newOutboxMessage(const smsg::SecMsgStored& outboxHdr)
    {
        smsg::SecMsgStored &smsgStored = const_cast<smsg::SecMsgStored&>(outboxHdr); // un-const the reference

        smsg::MessageData msg;
        QString label;
        QDateTime sent_datetime;
        QDateTime received_datetime;

        const unsigned char *pHeader = smsgStored.vchMessage.data();
        smsg::SecureMessage smsg(pHeader);
        uint32_t nPayload = smsgStored.vchMessage.size() - smsg::SMSG_HDR_LEN;

        int rv = smsgModule.Decrypt(false, smsgStored.addrOutbox, pHeader, pHeader + smsg::SMSG_HDR_LEN, nPayload, msg);

        if (rv == 0)
        {
            label = parent->getWalletModel()->getAddressTableModel()->labelForAddress(QString::fromStdString(EncodeDestination(PKHash(smsgStored.addrTo))));

            sent_datetime    .setTime_t(msg.timestamp);
            received_datetime.setTime_t(smsgStored.timeReceived);

            std::string sPrefix("sm");
            std::vector<unsigned char> vchKey(18);
            memcpy(&vchKey[0],  sPrefix.data(),  2);
            memcpy(&vchKey[2],  &msg.timestamp, 8);
            memcpy(&vchKey[10], &smsgStored.vchMessage[smsg::SMSG_HDR_LEN], 8);    // sample
                    std::string sText = std::string((char*)msg.vchMessage.data());

            handleMessageEntry(MessageTableEntry(vchKey,
                                              MessageTableEntry::Sent,
                                              label,
                                              QString::fromStdString(EncodeDestination(PKHash(smsgStored.addrTo))),
                                              msg.sFromAddress.c_str(),
                                              sent_datetime,
                                              received_datetime,
                                              !(smsgStored.status & SMSG_MASK_UNREAD),
                                              sText.c_str()),
                            false);
        }
    }

    void setEncryptionStatus()
    {
        if (parent->getWalletModel()->getEncryptionStatus() == WalletModel::Locked)
        {
            // -- Wallet is locked, clear secure message display.
            cachedMessageTable.clear();
            //cachedInvoiceTable.clear();
            //cachedInvoiceItemTable.clear();

        }
    }


    bool markAsRead(const int& idx)
    {
        MessageTableEntry *rec = index(idx);

        if(!rec || rec->read)
            // Can only mark one row at a time, and cannot mark rows not in model.
            return false;

        {
            LOCK(smsg::cs_smsgDB);
            smsg::SecMsgDB dbSmsg;

            if (!dbSmsg.Open("cr+"))
                //throw runtime_error("Could not open DB.");
                return false;

            smsg::SecMsgStored smsgStored;
            dbSmsg.ReadSmesg(&rec->vchKey[0], smsgStored);

            smsgStored.status &= ~SMSG_MASK_UNREAD;
            dbSmsg.WriteSmesg(&rec->vchKey[0], smsgStored);
            rec->read = !(smsgStored.status & SMSG_MASK_UNREAD);
        }

        return true;
    }

    void walletUnlocked()
    {
        // -- wallet is unlocked, can get at the private keys now
        refreshMessageTable();
        //parent->reset(); // reload table view
       // parent->getInvoiceTableModel()->getInvoiceItemTableModel()->reset();

    }
    MessageTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedMessageTable.size())
            return &cachedMessageTable[idx];
        else
            return 0;
    }
private:
    // Get the json value
    // Determine if it is a special message, i.e.: Invoice, Receipt, etc...
    void handleMessageEntry(const MessageTableEntry & message, const bool append)
    {
        addMessageEntry(message, append);
    }

    void addMessageEntry(const MessageTableEntry & message, const bool & append)
    {
        if(append)
        {
            cachedMessageTable.append(message);
        } else
        {
            int index = std::lower_bound(cachedMessageTable.begin(), cachedMessageTable.end(), message.received_datetime, MessageTableEntryLessThan()) - cachedMessageTable.begin();
            parent->beginInsertRows(QModelIndex(), index, index);
            cachedMessageTable.insert(
                        index,
                        message);
            parent->endInsertRows();
        }
    }
    WalletModel *walletModel;
};

MessageModel::MessageModel(WalletModel *walletModel, QObject *parent) :
    QAbstractTableModel(parent), walletModel(walletModel), optionsModel(nullptr), priv(nullptr)
{
    columns << tr("Type") << tr("Sent Date Time") << tr("Received Date Time") << tr("Label") << tr("To Address") << tr("From Address") << tr("Message");

    optionsModel = walletModel->getOptionsModel();

    priv = new MessageTablePriv(walletModel, this);
    priv->refreshMessageTable();

    subscribeToCoreSignals();
}

MessageModel::~MessageModel()
{
    delete priv;
    //delete invoiceTableModel;
    //delete receiptTableModel;
    unsubscribeFromCoreSignals();
}

bool MessageModel::getAddressOrPubkey(QString &address, QString &pubkey) const
{

    CTxDestination dest = DecodeDestination(address.toStdString());
    if (IsValidDestination(dest)){

        const PKHash *pkhash = std::get_if<PKHash>(&dest);
        if (!pkhash) {
            return false;
        }

        CKeyID destinationAddress = ToKeyID(*pkhash);
        //CKeyID destinationAddress = GetKeyForDestination(dest);
        if (destinationAddress.IsNull())
            return false;
            
        interfaces::Wallet& wallet = walletModel->wallet();
        
        CPubKey destinationKey;
        if (wallet.getPubKey(GetScriptForDestination(dest), destinationAddress, destinationKey))
          return false;
        if (!destinationKey.IsValid() || !destinationKey.IsCompressed()) {
            return false;
        }

        if (smsgModule.GetStoredKey(destinationAddress, destinationKey) != 0 && smsgModule.GetLocalKey(destinationAddress, destinationKey) != 0) // test if it's a local key
            return false;

        address = EncodeDestination(dest).c_str();
        pubkey = QString::fromStdString(EncodeBase58(destinationKey));

        return true;

    }
    return false;
}

WalletModel *MessageModel::getWalletModel()
{
    return walletModel;
}

OptionsModel *MessageModel::getOptionsModel()
{
    return optionsModel;
}
MessageModel::StatusCode MessageModel::sendMessages(const QList<SendMessagesRecipient> &recipients, const QString &addressFrom)
{

    QSet<QString> setAddress;

    if(recipients.empty())
        return OK;

    interfaces::Wallet& wallet = walletModel->wallet();

    // Pre-check input data for validity
    for(const SendMessagesRecipient &rcp: recipients)
    {
        if(!walletModel->validateAddress(rcp.address))
            return InvalidAddress;

        if(rcp.message == "")
            return MessageCreationFailed;

        std::string addrTo  = rcp.address.toStdString();
        std::string pubkey  = rcp.pubkey.toStdString();
        std::string message = rcp.message.toStdString();
        std::string addrFrom = addressFrom.toStdString();

        smsgModule.AddAddress(addrTo, pubkey);
        setAddress.insert(rcp.address);

		CTxDestination destFrom = DecodeDestination(addrFrom);
		if (!IsValidDestination(destFrom)) {
			return InvalidAddress;
		}
		CKeyID kiFrom = ToKeyID(std::get<WitnessV0KeyHash>(destFrom));

		CTxDestination destTo = DecodeDestination(addrTo);
		if (!IsValidDestination(destTo)) {
			return InvalidAddress;
		}
		CKeyID kiTo = ToKeyID(std::get<WitnessV0KeyHash>(destTo));
		bool fPaid = false; // cover this later
        int nRetention = 7; // cover this later 
        CAmount nFee = 0;
        size_t nTxBytes = 0;

		bool fFromFile = false;
		bool submit_msg = true;
		bool save_msg = true;
        CCoinControl cctl;

        std::string sError;
        smsg::SecureMessage smsgOut;
        if (smsgModule.Send(kiFrom, kiTo, message, smsgOut, sError, fPaid, nRetention, false, &nFee, &nTxBytes,
                        fFromFile, submit_msg, save_msg, &cctl) != 0)
        {
            QMessageBox::warning(nullptr, tr("Send Secure Message"),
                tr("Send failed: %1.").arg(sError.c_str()),
                QMessageBox::Ok, QMessageBox::Ok);

            return FailedErrorShown;
        }

        // Add addresses / update labels that we've sent to to the address book
        std::string strAddress = rcp.address.toStdString();
        CTxDestination dest = DecodeDestination(strAddress);
        std::string strLabel = rcp.label.toStdString();
        {
            /*
            LOCK(wallet->cs_wallet);

            std::map<CTxDestination, std::string>::iterator mi = wallet->mapAddressBook.find(dest);

             Check if we have a new address or an updated label
            if (mi == wallet->mapAddressBook.end() || mi->second != strLabel)
            {
                wallet->SetAddressBookName(dest, strLabel);
            }*/
        }
    }

    if(recipients.size() > setAddress.size())
        return DuplicateAddress;

    return OK;
}

MessageModel::StatusCode MessageModel::sendMessages(const QList<SendMessagesRecipient> &recipients)
{
    return sendMessages(recipients, "anon");
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return priv->cachedMessageTable.size();
}

int MessageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return columns.length();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    MessageTableEntry *rec = static_cast<MessageTableEntry*>(index.internalPointer());

    switch(role)
    {

    case Qt::DisplayRole:
        switch(index.column())
        {
            case Label:            return (rec->label.isEmpty() ? tr("(no label)") : rec->label);
            case ToAddress:        return rec->to_address;
            case FromAddress:      return rec->from_address;
            case SentDateTime:     return rec->sent_datetime;
            case ReceivedDateTime: return rec->received_datetime;
            case Message:          return rec->message;
            case Read:             return rec->read;
            case TypeInt:          return rec->type;
            case HTML:             return rec->received_datetime.toString() + "<br>"  + (rec->label.isEmpty() ? rec->from_address : rec->label)  + "<br>" + rec->message;
            case Type:
                switch(rec->type)
                {
                    case MessageTableEntry::Sent:     return Sent;
                    case MessageTableEntry::Received: return Received;
                    default: break;
                }
            case Key: return (rec->type == MessageTableEntry::Sent ? Sent : Received) + rec->from_address + QString::number(rec->sent_datetime.toTime_t());
        }
        break;

    case Qt::EditRole:
        if(index.column() == Key)
            return (rec->type == MessageTableEntry::Sent ? Sent : Received) + rec->from_address + QString::number(rec->sent_datetime.toTime_t());
        break;

    case KeyRole:           return QVariant::fromValue(rec->vchKey);
    case TypeRole:          return rec->type;
    case SentDateRole:      return rec->sent_datetime;
    case ReceivedDateRole:  return rec->received_datetime;
    case FromAddressRole:   return rec->from_address;
    case ToAddressRole:     return rec->to_address;
    case FilterAddressRole: return (rec->type == MessageTableEntry::Sent ? rec->to_address + rec->from_address : rec->from_address + rec->to_address);
    case LabelRole:         return rec->label;
    case MessageRole:       return rec->message;
    case ShortMessageRole:  return rec->message; // TODO: Short message
    case ReadRole:          return rec->read;
    case HTMLRole:          return rec->received_datetime.toString() + "<br>"  + (rec->label.isEmpty() ? rec->from_address : rec->label)  + "<br>" + rec->message;
    case Ambiguous:
        int it;

        for (it = 0; it<ambiguous.length(); it++) {
            if(ambiguous[it] == (rec->type == MessageTableEntry::Sent ? rec->to_address + rec->from_address : rec->from_address + rec->to_address))
                return false;
        }
        QString address = (rec->type == MessageTableEntry::Sent ? rec->to_address + rec->from_address : rec->from_address + rec->to_address);
        ambiguous.append(address);

        return "true";
        break;
    }

    return QVariant();
}

QVariant MessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return (orientation == Qt::Horizontal && role == Qt::DisplayRole ? columns[section] : QVariant());
}

Qt::ItemFlags MessageModel::flags(const QModelIndex & index) const
{
    if(index.isValid())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return 0;
}

QModelIndex MessageModel::index(int row, int column, const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    MessageTableEntry *data = priv->index(row);
    return (data ? createIndex(row, column, priv->index(row)) : QModelIndex());
}

bool MessageModel::removeRows(int row, int count, const QModelIndex & parent)
{
    MessageTableEntry *rec = priv->index(row);
    if(count != 1 || !rec)
        // Can only remove one row at a time, and cannot remove rows not in model.
        return false;

    {
        LOCK(smsg::cs_smsgDB);
        smsg::SecMsgDB dbSmsg;

        if (!dbSmsg.Open("cr+"))
            //throw runtime_error("Could not open DB.");
            return false;

       dbSmsg.EraseSmesg(&rec->vchKey[0]);
    }

    beginRemoveRows(parent, row, row);
    priv->cachedMessageTable.removeAt(row);
    endRemoveRows();

    return true;
}

void MessageModel::newMessage(const smsg::SecMsgStored &smsgInbox)
{
    priv->newMessage(smsgInbox);
}

void MessageModel::newOutboxMessage(const smsg::SecMsgStored &smsgOutbox)
{
    priv->newOutboxMessage(smsgOutbox);
}

void MessageModel::setEncryptionStatus()
{
     // We're only interested in NotifySecMsgWalletUnlocked when unlocked, as its called after new messagse are processed
    //if(status == WalletModel::Unlocked && QObject::sender()!=nullptr)
    //    return;

    priv->refreshMessageTable();

    //reset(); // reload table view
}

void MessageModel::walletUnlocked()
{
    priv->walletUnlocked();
}

bool MessageModel::markMessageAsRead(const QString &key) const
{
    return priv->markAsRead(lookupMessage(key));
}

int MessageModel::lookupMessage(const QString &key) const
{
    QModelIndexList lst = match(index(0, Key, QModelIndex()), Qt::EditRole, key, 1, Qt::MatchExactly);
    if(lst.isEmpty())
        return -1;
    else
        return lst.at(0).row();
}

static void NotifySecMsgInbox(MessageModel *messageModel, smsg::SecMsgStored inboxHdr)
{
    // Too noisy: OutputDebugStringF("NotifySecMsgInboxChanged %s\n", message);
    QMetaObject::invokeMethod(messageModel, "newMessage", Qt::QueuedConnection,
                              Q_ARG(smsg::SecMsgStored, inboxHdr));
}

static void NotifySecMsgOutbox(MessageModel *messageModel, smsg::SecMsgStored outboxHdr)
{
    QMetaObject::invokeMethod(messageModel, "newOutboxMessage", Qt::QueuedConnection,
                              Q_ARG(smsg::SecMsgStored, outboxHdr));
}

static void NotifySecMsgWallet(MessageModel *messageModel)
{
    QMetaObject::invokeMethod(messageModel, "walletUnlocked", Qt::QueuedConnection);
}

void MessageModel::subscribeToCoreSignals()
{
    qRegisterMetaType<smsg::SecMsgStored>("SecMsgStored");

    // Connect signals
    smsg::NotifySecMsgInboxChanged.connect(boost::bind(NotifySecMsgInbox, this, _1));
    smsg::NotifySecMsgOutboxChanged.connect(boost::bind(NotifySecMsgOutbox, this, _1));
    smsg::NotifySecMsgWalletUnlocked.connect(boost::bind(NotifySecMsgWallet, this));

    connect(walletModel, &WalletModel::encryptionStatusChanged, this, &MessageModel::setEncryptionStatus);
}

void MessageModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals
    smsg::NotifySecMsgInboxChanged.disconnect(boost::bind(NotifySecMsgInbox, this, _1));
    smsg::NotifySecMsgOutboxChanged.disconnect(boost::bind(NotifySecMsgOutbox, this, _1));
    smsg::NotifySecMsgWalletUnlocked.disconnect(boost::bind(NotifySecMsgWallet, this));

    disconnect(walletModel, SIGNAL(encryptionStatusChanged()), this, SLOT(setEncryptionStatus(walletModel->getEncryptionStatus())));
}
