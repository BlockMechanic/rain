// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_WALLETMODELTRANSACTION_H
#define RAIN_QT_WALLETMODELTRANSACTION_H

#include <qt/walletmodel.h>

#include <memory>
#include <amount.h>

#include <QObject>

class SendCoinsRecipient;
class SendAssetsRecipient;

namespace interfaces {
class Node;
}

/** Data model for a walletmodel transaction. */
class WalletModelTransaction
{
public:
    explicit WalletModelTransaction(const QList<SendAssetsRecipient> &recipients);

    QList<SendAssetsRecipient> getRecipients() const;

    CTransactionRef& getWtx();
    unsigned int getTransactionSize();

    void setTransactionFee(const CAmount& newFee);
    CAmount getTransactionFee() const;
    std::string getstrTxComment();

    CAmountMap getTotalTransactionAmount() const;

    void reassignAmounts(const std::vector<CAmount>& out_amounts, int nChangePosRet); // needed for the subtract-fee-from-amount feature

private:
    QList<SendAssetsRecipient> recipients;
    CTransactionRef wtx;
    CAmount fee;
    std::string strTxComment;
};

#endif // RAIN_QT_WALLETMODELTRANSACTION_H
