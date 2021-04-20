// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/coincontrolmodel.h>
#include <qt/coincontrolrecord.h>
#include <qt/addresstablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/rainunits.h>

#include <key_io.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <validation.h>
#include <QDebug>
#include <QStringList>

class CoinControlPriv {
public:
    CoinControlPriv(WalletModel *_walletModel, CoinControlModel *_parent) :
            parent(_parent),
            walletModel(_walletModel)
    {
    }

    // loads all current balances into cache
    void refreshWallet() {
        qDebug() << "CoinControlPriv::refreshWallet";
        cachedRecords.clear();

        // Keep up to date with wallet
        interfaces::Wallet& wallet = walletModel->wallet();
        int unit = walletModel->getOptionsModel()->getDisplayUnit();

        for (const auto& coins : wallet.listCoins()) {
            QString sWalletAddress = QString::fromStdString(EncodeDestination(coins.first));
            QString sWalletLabel = walletModel->getAddressTableModel()->labelForAddress(sWalletAddress);
            if (sWalletLabel.isEmpty())
                sWalletLabel = "(no label)";

            for (const auto& outpair : coins.second) {
                CoinControlRecord rec;

                const COutPoint& output = std::get<0>(outpair);
                const interfaces::WalletTxOut& out = std::get<1>(outpair);

                // address
                CTxDestination outputAddress;
                QString sAddress = "";
                if(ExtractDestination(out.txout.scriptPubKey, outputAddress))
                    sAddress = QString::fromStdString(EncodeDestination(outputAddress));

                // label
                QString sLabel = walletModel->getAddressTableModel()->labelForAddress(sAddress);
                if (sLabel.isEmpty())
                    sLabel = "(no label)";
                
                rec.address = sAddress;
                rec.label = sLabel;
                rec.asset = out.txout.nAsset.IsExplicit() ? QString::fromStdString(out.txout.nAsset.GetAsset().getName()) : "UNDEF";
                rec.amount = out.txout.nValue.IsExplicit() ? RainUnits::simplestFormat(unit, out.txout.nValue.GetAmount(), 4, false, RainUnits::SeparatorStyle::ALWAYS) : 0;
                rec.date = GUIUtil::dateTimeStr(out.time);
                rec.confirmations = out.depth_in_main_chain;
                rec.txhash = QString::fromStdString(output.hash.GetHex());
                rec.index = output.n;
                rec.locked = wallet.isLockedCoin(output);
                cachedRecords.append(rec);
            }
        }

        QString msg = QString("CoinControlPriv size: %1\n").arg(cachedRecords.size());
        qDebug() << msg;
    }

    int size() {
        return cachedRecords.size();
    }

    CoinControlRecord *index(int idx) {
        if (idx >= 0 && idx < cachedRecords.size()) {
            return &cachedRecords[idx];
        }
        return 0;
    }

private:

    CoinControlModel *parent;
    WalletModel *walletModel;
    QList<CoinControlRecord> cachedRecords;

};

CoinControlModel::CoinControlModel(WalletModel *parent) :
        QAbstractItemModel(parent), walletModel(parent), priv(new CoinControlPriv(parent, this))
{
    columns << tr("Label") << tr("Address") << tr("Asset") << tr("Amount") << tr("Date") << tr("Confirms");
    connect(walletModel, &WalletModel::balanceChanged, this, &CoinControlModel::checkBalanceChanged);
    priv->refreshWallet();
};

CoinControlModel::~CoinControlModel()
{
    delete priv;
};

void CoinControlModel::checkBalanceChanged(const interfaces::WalletBalances& balances) {
    qDebug() << "CoinControlModel::CheckBalanceChanged";
    Q_UNUSED(balances);
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
}

int CoinControlModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int CoinControlModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant CoinControlModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    CoinControlRecord *rec = static_cast<CoinControlRecord*>(index.internalPointer());

    switch (role)
    {
        case LabelRole:
            return rec->label;
        case AddressRole:
            return rec->address;
        case AssetRole:
            return rec->asset;
        case AmountRole: {
            return rec->amount;
        }
        case DateRole:
            return rec->date;
        case ConfirmsRole:
            return QString::number(rec->confirmations);
        case NindexRole:
            return rec->index;
        case TxHashRole:
            return rec->txhash;
        case LockedRole:
            return rec->locked;
        default:
            return QVariant();
    }
}

QVariant CoinControlModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            if (section < columns.size())
                return columns.at(section);
        } else {
            return section;
        }
    } else if (role == Qt::SizeHintRole) {
        if (orientation == Qt::Vertical)
            return QSize(30, 50);
    } else if (role == Qt::TextAlignmentRole) {
        if (orientation == Qt::Vertical)
            return Qt::AlignLeft + Qt::AlignVCenter;

        return Qt::AlignHCenter + Qt::AlignVCenter;
    }

    return QVariant();
}

QModelIndex CoinControlModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    CoinControlRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        return idx;
    }

    return QModelIndex();
}

void CoinControlModel::update()
{
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
}
