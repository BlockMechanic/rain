// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef HAVE_CONFIG_H
#include <config/rain-config.h>
#endif

#include <qt/transactiondesc.h>

#include <qt/rainunits.h>
#include <qt/guiutil.h>
#include <qt/paymentserver.h>
#include <qt/transactionrecord.h>

#include <consensus/consensus.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <validation.h>
#include <script/script.h>
#include <timedata.h>
#include <util/system.h>
#include <policy/policy.h>
#include <wallet/ismine.h>

#include "wallet/db.h"
#include "wallet/wallet.h"

#include <confidential_validation.cpp>

#include <stdint.h>
#include <string>

QString TransactionDesc::FormatTxStatus(const interfaces::WalletTx& wtx, const interfaces::WalletTxStatus& status, bool inMempool, int numBlocks, int64_t adjustedTime)
{
    if (!status.is_final)
    {
        if (wtx.tx->nLockTime < LOCKTIME_THRESHOLD)
            return tr("Open for %n more block(s)", "", wtx.tx->nLockTime - numBlocks);
        else
            return tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx.tx->nLockTime));
    }
    else
    {
        int nDepth = status.depth_in_main_chain;
        if (nDepth < 0)
            return tr("conflicted with a transaction with %1 confirmations").arg(-nDepth);
        else if (nDepth == 0)
            return tr("0/unconfirmed, %1").arg((inMempool ? tr("in memory pool") : tr("not in memory pool"))) + (status.is_abandoned ? ", "+tr("abandoned") : "");
        else if (nDepth < 6)
            return tr("%1/unconfirmed").arg(nDepth);
        else
            return tr("%1 confirmations").arg(nDepth);
    }
}

QString TransactionDesc::toHTML(interfaces::Node& node, interfaces::Wallet& wallet, TransactionRecord *rec, int unit)
{
    int numBlocks;
    int64_t adjustedTime;
    interfaces::WalletTxStatus status;
    interfaces::WalletOrderForm orderForm;
    bool inMempool;
    interfaces::WalletTx wtx = wallet.getWalletTxDetails(rec->hash, status, orderForm, inMempool, numBlocks, adjustedTime);

    QString strHTML;

    strHTML.reserve(4000);
    strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

    int64_t nTime = wtx.time;
    CAmountMap mapCredit = wtx.credit;
    CAmountMap mapDebit = wtx.debit;
    CAmountMap mapNet = mapCredit - mapDebit;

//    LogPrintf("mapCredit %s\n mapDebit%s\n mapNet%s\n", mapCredit, mapDebit, mapNet);

    strHTML += "<b>" + tr("Status") + ":</b> " + FormatTxStatus(wtx, status, inMempool, numBlocks, adjustedTime);
    strHTML += "<br>";

    strHTML += "<b>" + tr("SPV only") + ":</b> " + (!rec->status.fValidated ? "yes" : "no") + "<br>";
    strHTML += "<b>" + tr("Date") + ":</b> " + (nTime ? GUIUtil::dateTimeStr(nTime) : "") + "<br>";

    //
    // From
    //
    if (wtx.is_coinbase)
    {
        strHTML += "<b>" + tr("Source") + ":</b> " + tr("Generated") + "<br>";
    }
    if (wtx.is_coinstake)
    {
        strHTML += "<b>" + tr("Source") + ":</b> " + tr("Staked") + "<br>";
    }
    else if (wtx.value_map.count("from") && !wtx.value_map["from"].empty())
    {
        // Online transaction
        strHTML += "<b>" + tr("From") + ":</b> " + GUIUtil::HtmlEscape(wtx.value_map["from"]) + "<br>";
    }
    else
    {
        // Offline transaction
        if (mapNet > CAmountMap())
        {
            // Credit
            CTxDestination address = DecodeDestination(rec->address);
            if (IsValidDestination(address)) {
                std::string name;
                isminetype ismine;
                if (wallet.getAddress(address, &name, &ismine, /* purpose= */ nullptr))
                {
                    strHTML += "<b>" + tr("From") + ":</b> " + tr("unknown") + "<br>";
                    strHTML += "<b>" + tr("To") + ":</b> ";
                    strHTML += GUIUtil::HtmlEscape(rec->address);
                    QString addressOwned = ismine == ISMINE_SPENDABLE ? tr("own address") : tr("watch-only");
                    if (!name.empty())
                        strHTML += " (" + addressOwned + ", " + tr("label") + ": " + GUIUtil::HtmlEscape(name) + ")";
                    else
                        strHTML += " (" + addressOwned + ")";
                    strHTML += "<br>";
                }
            }
        }
    }

    //
    // To
    //
    if (wtx.value_map.count("to") && !wtx.value_map["to"].empty())
    {
        // Online transaction
        std::string strAddress = wtx.value_map["to"];
        strHTML += "<b>" + tr("To") + ":</b> ";
        CTxDestination dest = DecodeDestination(strAddress);
        std::string name;
        if (wallet.getAddress(
                dest, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr) && !name.empty())
            strHTML += GUIUtil::HtmlEscape(name) + " ";
        strHTML += GUIUtil::HtmlEscape(strAddress) + "<br>";
    }

    //
    // Amount
    //
    if (wtx.is_coinbase && mapCredit == CAmountMap())
    {
        //
        // Coinbase
        //
        CAmountMap mapUnmatured;
        for (const CTxOut& txout : wtx.tx->vout)
            mapUnmatured += wallet.getCredit(txout, ISMINE_ALL);
        strHTML += "<b>" + tr("Credit") + ":</b> ";
        if (status.is_in_main_chain)
            strHTML += GUIUtil::formatMultiAssetAmount(mapUnmatured,unit, RainUnits::separatorAlways, "\n") + " (" + tr("matures in %n more block(s)", "", status.blocks_to_maturity) + ")";
        else
            strHTML += "(" + tr("not accepted") + ")";
        strHTML += "<br>";
    }
    else if (mapNet > CAmountMap())
    {
        //
        // Credit
        //
        strHTML += "<b>" + tr("Credit") + ":</b> " + GUIUtil::formatMultiAssetAmount(mapNet,unit, RainUnits::separatorAlways, "\n") + "<br>";
    }
    else
    {
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        for (const isminetype mine : wtx.txin_is_mine)
        {
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        for (const isminetype mine : wtx.txout_is_mine)
        {
            if(fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMe)
        {
            if(fAllFromMe & ISMINE_WATCH_ONLY)
                strHTML += "<b>" + tr("From") + ":</b> " + tr("watch-only") + "<br>";

            //
            // Debit
            //
            auto mine = wtx.txout_is_mine.begin();
            for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i)
            {
                const CTxOut& txout = wtx.tx->vout[i];
                // Ignore change
                isminetype toSelf = *(mine++);
                CAsset asset = txout.nAsset.GetAsset();
                if ((toSelf == ISMINE_SPENDABLE) && (fAllFromMe == ISMINE_SPENDABLE))
                    continue;

                if (!wtx.value_map.count("to") || wtx.value_map["to"].empty())
                {
                    // Offline transaction
                    CTxDestination address;
                    if (ExtractDestination(txout.scriptPubKey, address))
                    {
                        strHTML += "<b>" + tr("To") + ":</b> ";
                        std::string name;
                        if (wallet.getAddress(
                                address, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr) && !name.empty())
                            strHTML += GUIUtil::HtmlEscape(name) + " ";
                        strHTML += GUIUtil::HtmlEscape(EncodeDestination(address));
                        if(toSelf == ISMINE_SPENDABLE)
                            strHTML += " (own address)";
                        else if(toSelf & ISMINE_WATCH_ONLY)
                            strHTML += " (watch-only)";
                        strHTML += "<br>";
                    }
                }

                strHTML += "<b>" + tr("Debit") + ":</b> \n" + QString::fromStdString(asset.getName()) + " : "+ RainUnits::formatHtmlWithUnit(unit, -wtx.txout_amounts[i]) + "<br>";
                if(toSelf)
                    strHTML += "<b>" + tr("Credit") + ":</b> \n" + QString::fromStdString(asset.getName()) + " : "+ RainUnits::formatHtmlWithUnit(unit, wtx.txout_amounts[i]) + "<br>";
            }

            if (fAllToMe)
            {
                // Payment to self
                CAmountMap mapChange = wtx.change;
                CAmountMap mapValue = mapCredit - mapChange;
                strHTML += "<b>" + tr("Total debit") + ":</b> " + GUIUtil::formatMultiAssetAmount(mapValue*-1,unit, RainUnits::separatorAlways, "\n") + "<br>";
                strHTML += "<b>" + tr("Total credit") + ":</b> " + GUIUtil::formatMultiAssetAmount(mapValue,unit, RainUnits::separatorAlways, "\n") + "<br>";
            }

            CAmountMap mapTxFee = mapDebit - wtx.tx->GetValueOutMap();
            if (mapTxFee > CAmountMap())
                strHTML += "<b>" + tr("Transaction fee") + ":</b> " +  GUIUtil::formatMultiAssetAmount(mapTxFee*-1,unit, RainUnits::separatorAlways, "\n") + "<br>";
        }
        else
        {

            //
            // Mixed debit transaction
            //
            auto mine = wtx.txin_is_mine.begin();
            for (const CTxIn& txin : wtx.tx->vin) {
                if (*(mine++)) {
                    strHTML += "<b>" + tr("Debit") + ":</b> " + GUIUtil::formatMultiAssetAmount(wallet.getDebit(txin, ISMINE_ALL)*-1,unit, RainUnits::separatorAlways, "\n") + "<br>";
                }
            }
            mine = wtx.txout_is_mine.begin();
            for (const CTxOut& txout : wtx.tx->vout) {
                if (*(mine++)) {
                    strHTML += "<b>" + tr("Credit") + ":</b> " + GUIUtil::formatMultiAssetAmount(wallet.getCredit(txout, ISMINE_ALL),unit, RainUnits::separatorAlways, "\n") + "<br>";
                }
            }
        }
    }

    strHTML += "<b>" + tr("Net amount") + ":</b> " + GUIUtil::formatMultiAssetAmount(mapNet,unit, RainUnits::separatorAlways, "\n") + "<br>";

    //
    // Message
    //
    if (wtx.value_map.count("message") && !wtx.value_map["message"].empty())
        strHTML += "<br><b>" + tr("Message") + ":</b><br>" + GUIUtil::HtmlEscape(wtx.value_map["message"], true) + "<br>";
    if (wtx.value_map.count("comment") && !wtx.value_map["comment"].empty())
        strHTML += "<br><b>" + tr("Comment") + ":</b><br>" + GUIUtil::HtmlEscape(wtx.value_map["comment"], true) + "<br>";

/*    if (!wtx.tx->strTxComment.empty()){
		std::string s(wtx.tx->strTxComment.begin(), wtx.tx->strTxComment.end());
        strHTML += "<br><b>" + tr("Tx Comment") + ":</b><br>" + GUIUtil::HtmlEscape(s, true) + "<br>";
	}*/

    strHTML += "<b>" + tr("Transaction ID") + ":</b> " + rec->getTxHash() + "<br>";
    strHTML += "<b>" + tr("Transaction total size") + ":</b> " + QString::number(wtx.tx->GetTotalSize()) + " bytes<br>";
    strHTML += "<b>" + tr("Transaction virtual size") + ":</b> " + QString::number(GetVirtualTransactionSize(*wtx.tx)) + " bytes<br>";
    strHTML += "<b>" + tr("Output index") + ":</b> " + QString::number(rec->getOutputIndex()) + "<br>";

    // Message from normal rain:URI (rain:123...?message=example)
    for (const std::pair<std::string, std::string>& r : orderForm)
        if (r.first == "Message")
            strHTML += "<br><b>" + tr("Message") + ":</b><br>" + GUIUtil::HtmlEscape(r.second, true) + "<br>";

    if (wtx.is_coinbase)
    {
        quint32 numBlocksToMaturity = COINBASE_MATURITY +  1;
        strHTML += "<br>" + tr("Generated coins must mature %1 blocks before they can be spent.").arg(QString::number(numBlocksToMaturity)) + "<br>";
    }

    //
    // Debug view
    //
    if (node.getLogCategories() != BCLog::NONE)
    {
        strHTML += "<hr><br>" + tr("Debug information") + "<br><br>";
        for (const CTxIn& txin : wtx.tx->vin)
            if(wallet.txinIsMine(txin))
                strHTML += "<b>" + tr("Debit") + ":</b> " + GUIUtil::formatMultiAssetAmount(wallet.getDebit(txin, ISMINE_ALL)*-1,unit, RainUnits::separatorAlways, "\n") + "<br>";
        for (const CTxOut& txout : wtx.tx->vout)
            if(wallet.txoutIsMine(txout))
                strHTML += "<b>" + tr("Credit") + ":</b> " + GUIUtil::formatMultiAssetAmount(wallet.getCredit(txout, ISMINE_ALL),unit, RainUnits::separatorAlways, "\n") + "<br>";

        strHTML += "<br><b>" + tr("Transaction") + ":</b><br>";
        strHTML += GUIUtil::HtmlEscape(wtx.tx->ToString(), true);

        strHTML += "<br><b>" + tr("Inputs") + ":</b>";
        strHTML += "<ul>";

        for (const CTxIn& txin : wtx.tx->vin)
        {
            COutPoint prevout = txin.prevout;

            Coin prev;
            if(node.getUnspentOutput(prevout, prev))
            {
                {
                    strHTML += "<li>";
                    const CTxOut &vout = prev.out;
                    CTxDestination address;
                    if (ExtractDestination(vout.scriptPubKey, address))
                    {
                        std::string name;
                        if (wallet.getAddress(address, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr) && !name.empty())
                            strHTML += GUIUtil::HtmlEscape(name) + " ";
                        strHTML += QString::fromStdString(EncodeDestination(address));
                    }
                    strHTML = strHTML + " " + tr("Asset") + " : " + QString::fromStdString(vout.nAsset.GetAsset().getName());
                    strHTML = strHTML + " " + tr("Amount") + "=" + RainUnits::formatHtmlWithUnit(unit, vout.nValue.GetAmount());
                    strHTML = strHTML + " IsMine=" + (wallet.txoutIsMine(vout) & ISMINE_SPENDABLE ? tr("true") : tr("false")) + "</li>";
                    strHTML = strHTML + " IsWatchOnly=" + (wallet.txoutIsMine(vout) & ISMINE_WATCH_ONLY ? tr("true") : tr("false")) + "</li>";
                }
            }
        }

        strHTML += "</ul>";
    }

    strHTML += "</font></html>";
    return strHTML;
}
