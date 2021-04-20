// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_ASSETTABLEMODEL_H
#define RAIN_QT_ASSETTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <interfaces/wallet.h>
#include <interfaces/node.h>

class AssetTablePriv;
class WalletModel;
class AssetRecord;

struct CAsset;

/** Models table of Global assets.
 */
class AssetTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit AssetTableModel(interfaces::Node& node, WalletModel *parent = nullptr);
    ~AssetTableModel();

    enum ColumnIndex {
        Name,
        Shortname,
        Balance,
        Holdings, // % of total
        Issuer,
        Date
    };

    /** Roles to get specific information from a row.
        These are independent of column.
    */
    enum RoleIndex {
        NameRole = 100,
        SymbolRole = 101,
        BalanceRole = 102,
        HoldingsRole = 103,
        IssuerRole = 104,
        DateRole = 105
    };


    Q_INVOKABLE int rowCount(const QModelIndex &parent=QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QString formatTooltip(const AssetRecord *rec) const;
    QString formatAssetName(const AssetRecord *wtx) const;
    QString formatAssetQuantity(const AssetRecord *wtx) const;
	QHash<int, QByteArray> roleNames() const override
	{
		QHash<int, QByteArray> roles;
		roles[NameRole] = "name";
		roles[SymbolRole] = "symbol";
		roles[BalanceRole] = "balance";
		roles[HoldingsRole] = "holdings";
		roles[IssuerRole] = "issuer";
		roles[DateRole] = "date";
		return roles;
	}
    void checkBalanceChanged(const interfaces::WalletBalances& balances);

    Q_INVOKABLE void update();

private:
    WalletModel *walletModel;
    QStringList columns;
    AssetTablePriv *priv;
    interfaces::Node& m_node;

    friend class AssetTablePriv;
};
/*
class AssetTableModelWrapper: public QObject
{
public:
	explicit AssetTableModelWrapper(QObject* parent=nullptr);

    ~AssetTableModelWrapper() {}

	void initialize(interfaces::Node& node, WalletModel *parent = 0)
	{
	    m_obj = new AssetTableModel(node,parent);
	}
    //... + additional member functions you need.
protected:
    AssetTableModel * m_obj =nullptr;
};*/
#endif // RAIN_QT_ASSETTABLEMODEL_H
