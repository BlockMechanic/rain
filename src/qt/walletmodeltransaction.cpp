// Copyright (c) 2011-2019 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_CONFIG_H
#include <config/rain-config.h>
#endif

#include <qt/walletmodeltransaction.h>

#include <policy/policy.h>

WalletModelTransaction::WalletModelTransaction(const QList<SendAssetsRecipient> &_recipients) :
    recipients(_recipients),
    fee(0)
{
}

QList<SendAssetsRecipient> WalletModelTransaction::getRecipients() const
{
    return recipients;
}

CTransactionRef& WalletModelTransaction::getWtx()
{
    return wtx;
}

unsigned int WalletModelTransaction::getTransactionSize()
{
    return wtx ? GetVirtualTransactionSize(*wtx) : 0;
}

CAmount WalletModelTransaction::getTransactionFee() const
{
    return fee;
}

void WalletModelTransaction::setTransactionFee(const CAmount& newFee)
{
    fee = newFee;
}

void WalletModelTransaction::reassignAmounts(int nChangePosRet)
{
    const CTransaction* walletTransaction = wtx.get();
    int i = 0;
    for (QList<SendAssetsRecipient>::iterator it = recipients.begin(); it != recipients.end(); ++it)
    {
        SendAssetsRecipient& rcp = (*it);
        {
            if (i == nChangePosRet)
                i++;
            rcp.amount = walletTransaction->vout[i].nValue.GetAmount();
            i++;
        }
    }
}

CAmountMap WalletModelTransaction::getTotalTransactionAmount() const
{
    CAmountMap totalTransactionAmount;
    for (const auto &rcp : recipients)
    {
        totalTransactionAmount[rcp.asset] += rcp.amount;
    }
    return totalTransactionAmount;
}
