// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_TRANSACTIONFILTERPROXY_H
#define RAIN_QT_TRANSACTIONFILTERPROXY_H

#include <amount.h>

#include <QDateTime>
#include <QSortFilterProxyModel>

/** Filter the transaction list according to pre-specified rules. */
class TransactionFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TransactionFilterProxy(QObject *parent = nullptr);

    /** Earliest date that can be represented (far in the past) */
    static const QDateTime MIN_DATE;
    /** Last date that can be represented (far in the future) */
    static const QDateTime MAX_DATE;
    /** Type filter bit field (all types) */
    static const quint32 ALL_TYPES = 0xFFFFFFFF;

    static quint32 TYPE(int type) { return 1<<type; }

    enum WatchOnlyFilter
    {
        WatchOnlyFilter_All,
        WatchOnlyFilter_Yes,
        WatchOnlyFilter_No
    };

    void setDateRange(const QDateTime &from, const QDateTime &to);
    void clearDateRange() {
        if (dateFrom != MIN_DATE || dateTo == MAX_DATE)
            setDateRange(MIN_DATE, MAX_DATE);
    }

    void setAddressPrefix(const QString& addrPrefix);
    void setSearchString(const QString &);
    /**
      @note Type filter takes a bit field created with TYPE() or ALL_TYPES
     */
    void setTypeFilter(quint32 modes);
    void setMinAmount(const CAmount& minimum);
    void setWatchOnlyFilter(WatchOnlyFilter filter);

    /** Set maximum number of rows returned, -1 if unlimited. */
    void setLimit(int limit);

    /** Set whether to show conflicted transactions. */
    void setShowInactive(bool showInactive);

    /** Only stakes txes **/
    void setOnlyStakes(bool fOnlyStakes);

    /** Shows only p2cs-p2cs && xxx-p2cs **/
    void setOnlyColdStakes(bool fOnlyColdStakes);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    QDateTime dateFrom;
    QDateTime dateTo;
    QString m_search_string;
    QString addrPrefix;
    quint32 typeFilter;
    WatchOnlyFilter watchOnlyFilter;
    CAmount minAmount;
    int limitRows;
    bool showInactive;
    bool fOnlyStakes = false;
    bool fOnlyColdStaking = false;

    bool isStakeTx(int type) const;
    bool isColdStake(int type) const;
};

#endif // RAIN_QT_TRANSACTIONFILTERPROXY_H
