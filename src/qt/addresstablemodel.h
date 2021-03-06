// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_ADDRESSTABLEMODEL_H
#define RAIN_QT_ADDRESSTABLEMODEL_H

#include <QAbstractItemModel>
#include <QStringList>

enum class OutputType;

class AddressTablePriv;
class WalletModel;

namespace interfaces {
class Wallet;
}

/**
   Qt model of the address book in the core. This allows views to access and modify the address book.
 */
class AddressTableModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit AddressTableModel(WalletModel *parent = nullptr);
    ~AddressTableModel();

    enum ColumnIndex {
        Label = 0,  /**< User specified label */
        Address = 1, /**< Rain address */
        Date = 2, /**< Address creation date */
        Type = 3 /**< Address Type */
    };

    enum RoleIndex {
		LabelRole =100,
		AddressRole =101,
		DateRole=102,		
        TypeRole=103
    };

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,                     /**< Everything ok */
        NO_CHANGES,             /**< No changes were made during edit operation */
        INVALID_ADDRESS,        /**< Unparseable address */
        DUPLICATE_ADDRESS,      /**< Address already in address book */
        WALLET_UNLOCK_FAILURE,  /**< Wallet could not be unlocked to create new receiving address */
        KEY_GENERATION_FAILURE  /**< Generating a new public key for a receiving address failed */
    };

    static const QString Send;      /**< Specifies send address */
    static const QString Receive;   /**< Specifies receive address */
    static const QString Delegator; /**< Specifies cold staking addresses which delegated tokens to this wallet and ARE being staked */
    static const QString Delegable; /**< Specifies cold staking addresses which delegated tokens to this wallet*/
    static const QString ColdStaking; /**< Specifies cold staking own addresses */
    static const QString ColdStakingSend; /**< Specifies send cold staking addresses (simil 'contacts')*/

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    int sizeSend() const;
    int sizeRecv() const;
    int size() const;
    int sizeColdSend() const;
    void notifyChange(const QModelIndex &index);
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QModelIndex parent(const QModelIndex &child) const;
    /*@}*/

    /* Add an address to the model.
       Returns the added address on success, and an empty string otherwise.
     */
    QString addRow(const QString &type, const QString &label, const QString &address, const OutputType address_type);

    /** Look up label for address in address book, if not found return empty string. */
    QString labelForAddress(const QString &address) const;

    /** Look up purpose for address in address book, if not found return empty string. */
    QString purposeForAddress(const QString &address) const;

    /* Look up row index of an address in the model.
       Return -1 if not found.
     */
    int lookupAddress(const QString &address) const;

    /**
     * Checks if the address is whitelisted
     */
    bool isWhitelisted(const std::string& address) const;

    /**
     * Return last unused address
     */
    QString getLastUnusedAddress() const;

    EditStatus getEditStatus() const { return editStatus; }

    void refresh();
    OutputType GetDefaultAddressType() const;

	QHash<int, QByteArray> roleNames() const override
	{
		QHash<int, QByteArray> roles;
		roles[LabelRole] = "label";
		roles[AddressRole] = "address";
		roles[DateRole] = "date";
		roles[TypeRole] = "type";
		return roles;
	}

private:
    WalletModel* const walletModel;
    AddressTablePriv *priv = nullptr;
    QStringList columns;
    EditStatus editStatus = OK;

    /** Look up address book data given an address string. */
    bool getAddressData(const QString &address, std::string* name, std::string* purpose) const;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

public Q_SLOTS:
    /* Update address list from core.
     */
    void updateEntry(const QString &address, const QString &label, bool isMine, const QString &purpose, int status);

    friend class AddressTablePriv;
};

#endif // RAIN_QT_ADDRESSTABLEMODEL_H
