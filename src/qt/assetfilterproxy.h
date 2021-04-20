// Copyright (c) 2017-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_ASSETFILTERPROXY_H
#define RAIN_ASSETFILTERPROXY_H

#include <QObject>
#include <QSortFilterProxyModel>

class AssetFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit AssetFilterProxy(QObject *parent = 0);

    void setAssetNamePrefix(const QString &sAssetNamePrefix);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    /** Only Mine **/
    void setOnlyMine(bool fOnlyMine);
    /** Only Held **/
    void setOnlyHeld(bool fOnlyHeld);    
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    QString sAssetNamePrefix;
    bool fOnlyMine = false;
    bool fOnlyHeld = false;
};
#endif //RAIN_ASSETFILTERPROXY_H
