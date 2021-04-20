// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/assettablemodel.h>
#include <qt/assetrecord.h>
#include <key_io.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/walletmodel.h>
#include <validation.h>
#include <QDebug>
#include <QStringList>

class AssetTablePriv {
public:
    AssetTablePriv(interfaces::Node& _node, WalletModel *_walletModel, AssetTableModel *_parent) :
            parent(_parent),
            m_node(_node),
            walletModel(_walletModel)
    {
    }

    // loads all current balances into cache
    void refreshWallet() {
        qDebug() << "AssetTablePriv::refreshWallet";
        cachedAssets.clear();
        //iterate ALL chain Assets
        //Fill values, where we hold balances

        CAmountMap m_supply = m_node.getMoneySupply();
        // Keep up to date with wallet
        interfaces::Wallet& wallet = walletModel->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();

        for(auto const& x : passetsCache->GetItemsMap()){
            AssetRecord rec(QString::fromStdString(x.first));
            bool fIsAdministrator =false;
            CDatabasedAssetData data = x.second->second;

            rec.sAssetShortName = data.asset.IsExplicit() ? QString::fromStdString(data.asset.GetAsset().getShortName()) : "UNDEF";
            rec.inputAmount = data.inputAmount;
            rec.issuedAmount = data.issuedAmount;
            rec.inputAssetID = data.inputAssetID;
            rec.nTime = data.nTime;
            rec.nVersion = data.asset.IsExplicit() ? data.asset.GetAsset().nVersion : 0;

            if(data.asset.IsExplicit()){
                auto it = balances.balance.find(data.asset.GetAsset());
                if (it != balances.balance.end())
                    rec.balance = it->second;

                auto pit = balances.unconfirmed_balance.find(data.asset.GetAsset());
                if (pit != balances.unconfirmed_balance.end())
                    rec.pending = pit->second;

                auto iit = balances.immature_balance.find(data.asset.GetAsset());
                if (iit != balances.immature_balance.end())
                    rec.immature = iit->second;

                auto sit = balances.stake.find(data.asset.GetAsset());
                if (sit != balances.stake.end())
                    rec.stake = sit->second;

            }

            CTxDestination address;
            QString issuerr = "";
            if (ExtractDestination(data.issuingAddress, address)){
                if(walletModel->wallet().isMine(address))
                   fIsAdministrator = true;
                issuerr = QString::fromStdString(EncodeDestination(address));
            }

            if(data.asset.IsExplicit()){
                if (data.asset.GetAsset() == Params().GetConsensus().subsidy_asset){
                    if(ExtractDestination(Params().GetConsensus().mandatory_coinbase_destination, address))
                        issuerr = QString::fromStdString(EncodeDestination(address));
                }
            }
            rec.issuer = issuerr;
            rec.fIsAdministrator = fIsAdministrator;
            //std::cout<< " TESRT  " << x.first  <<  "issuer "  << issuerr.toStdString() << std::endl;
            cachedAssets.append(rec);
        }
    }

    int size() {
        return cachedAssets.size();
    }

    AssetRecord *index(int idx) {
        if (idx >= 0 && idx < cachedAssets.size()) {
            return &cachedAssets[idx];
        }
        return 0;
    }

private:

    AssetTableModel *parent;
    interfaces::Node& m_node;
    WalletModel *walletModel;
    QList<AssetRecord> cachedAssets;

};

AssetTableModel::AssetTableModel(interfaces::Node& node, WalletModel *parent) :
        QAbstractTableModel(parent), walletModel(parent), priv(new AssetTablePriv(node, parent, this)),
        m_node(node)
{
    columns << tr("Name") << tr("Shortname") << tr("Balance") << tr("Holdings") << tr("Issuer") << tr("Date");
    connect(walletModel, &WalletModel::balanceChanged, this, &AssetTableModel::checkBalanceChanged);
    priv->refreshWallet();
};

AssetTableModel::~AssetTableModel()
{
    delete priv;
};

void AssetTableModel::checkBalanceChanged(const interfaces::WalletBalances& balances) {
    qDebug() << "AssetTableModel::CheckBalanceChanged";
    // TODO: optimize by 1) updating cache incrementally; and 2) emitting more specific dataChanged signals
    Q_UNUSED(balances);
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
}

int AssetTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int AssetTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant AssetTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    AssetRecord *rec = static_cast<AssetRecord*>(index.internalPointer());

    switch (role)
    {
        case NameRole:
            return rec->sAssetName;
        case SymbolRole:
            return rec->sAssetShortName;
        case BalanceRole:
            return (double) rec->balance/COIN;
        case HoldingsRole: {
            CAmount issued = rec->issuedAmount;
            CAmount bal = rec->balance;
            //LogPrintf("Bal = %d, issued = %d", bal, issued);
            return (double) bal/COIN;//(double) (bal/issued);
        }
        case IssuerRole:
            return rec->issuer;
        case DateRole:
            return rec->nTime;
        default:
            return QVariant();
    }
}

QVariant AssetTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

QModelIndex AssetTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    AssetRecord *data = priv->index(row);
    if(data)
    {
        QModelIndex idx = createIndex(row, column, priv->index(row));
        return idx;
    }

    return QModelIndex();
}

QString AssetTableModel::formatTooltip(const AssetRecord *rec) const
{
    QString tooltip = formatAssetName(rec) + QString("\n") + formatAssetQuantity(rec);
    return tooltip;
}

QString AssetTableModel::formatAssetName(const AssetRecord *wtx) const
{
    return wtx->sAssetName;
}

QString AssetTableModel::formatAssetQuantity(const AssetRecord *wtx) const
{
    return QString("%1").arg(wtx->balance);
}

void AssetTableModel::update()
{
    priv->refreshWallet();
    Q_EMIT dataChanged(index(0, 0, QModelIndex()), index(priv->size(), columns.length()-1, QModelIndex()));
}
