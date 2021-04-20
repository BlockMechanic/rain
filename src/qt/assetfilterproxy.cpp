// Copyright (c) 2017-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <qt/assetfilterproxy.h>
#include <qt/assettablemodel.h>

AssetFilterProxy::AssetFilterProxy(QObject *parent) :
        QSortFilterProxyModel(parent),
        sAssetNamePrefix()
{
}

bool AssetFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    QString sAssetName = index.data(AssetTableModel::NameRole).toString();

    if(!sAssetName.startsWith(sAssetNamePrefix, Qt::CaseInsensitive))
        return false;

    //bool isMine = index.data(AssetTableModel::IssuerRole).toBool();

    qint64 balance = llabs(index.data(AssetTableModel::BalanceRole).toLongLong());

    if (fOnlyHeld && balance <= 0)
        return false;

    //if (fOnlyMine && !isMine)
    //    return false;

    return true;
}

void AssetFilterProxy::setAssetNamePrefix(const QString &_sAssetNamePrefix)
{
    this->sAssetNamePrefix = _sAssetNamePrefix;
    invalidateFilter();
}

int AssetFilterProxy::rowCount(const QModelIndex& parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

void AssetFilterProxy::setOnlyMine(bool fOnlyMine){
    this->fOnlyMine = fOnlyMine;
    invalidateFilter();
}

void AssetFilterProxy::setOnlyHeld(bool fOnlyHeld){
    this->fOnlyHeld = fOnlyHeld;
    invalidateFilter();
}
