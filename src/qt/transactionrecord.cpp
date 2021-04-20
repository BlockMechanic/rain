// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <chain.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <wallet/ismine.h>

#include <stdint.h>

#include <QDateTime>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction()
{
    // There are currently no cases where we hide transactions, but
    // we may want to use this in the future for things like RBF.
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const interfaces::WalletTx& wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.time;
    CAmountMap nCredit = wtx.credit;
    CAmountMap nDebit = wtx.debit;
    CAmountMap nNet = nCredit - nDebit;
    uint256 hash = wtx.tx->GetHash();
    std::map<std::string, std::string> mapValue = wtx.value_map;
    bool cbcs = wtx.is_coinstake || wtx.is_coinbase;

    bool involvesWatchAddress = false;
    isminetype fAllFromMe = ISMINE_SPENDABLE;
    bool any_from_me = false;
    std::set<CAsset> assets_issued_to_me_only;
    if (wtx.is_coinbase) {
        fAllFromMe = ISMINE_NO;
    }
    else
    {
        CAmountMap assets_received_by_me_only;
        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++)
        {
            if (wtx.tx->vout[i].IsFee()) {
                continue;
            }
            const CAsset& asset = wtx.txout_assets[i];
            if (assets_received_by_me_only.count(asset) && assets_received_by_me_only.at(asset) < 0) {
                // Already known to be received by not-me
                continue;
            }
            isminetype mine = wtx.txout_address_is_mine[i];
            if (!mine) {
                assets_received_by_me_only[asset] = -1;
            } else {
                assets_received_by_me_only[asset] += wtx.txout_amounts[i];
            }
        }

        any_from_me = false;
        for (size_t i = 0; i < wtx.tx->vin.size(); ++i)
        {
            /* Issuance detection */
            isminetype mine = wtx.txin_is_mine[i];
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
            if (mine) any_from_me = true;
            CAmountMap assets;
            assets[wtx.txin_issuance_asset[i]] = wtx.txin_issuance_asset_amount[i];
            assets[wtx.txin_issuance_token[i]] = wtx.txin_issuance_token_amount[i];
            for (const auto& asset : assets) {
                if (!asset.first.IsNull()) {
                    if (assets_received_by_me_only.count(asset.first) == 0) {
                        continue;
                    }
                    if (asset.second == assets_received_by_me_only.at(asset.first)) {
                        // Special case: collapse the chain of issue, send, receive to just an issue
                        assets_issued_to_me_only.insert(asset.first);
                        continue;
                    } else {
                        TransactionRecord sub(hash, nTime);
                        sub.involvesWatchAddress = involvesWatchAddress;
                        sub.asset = asset.first;
                        sub.debit = assets;
                        sub.type = TransactionRecord::IssuedAsset;
                        parts.append(sub);
                    }
                }
            }
        }
    }

    if (nNet > CAmountMap() || cbcs)
    {
        //
        // Credit
        //
        for(unsigned int i = 0; i < wtx.tx->vout.size(); i++)
        {
            const CTxOut& txout = wtx.tx->vout[i];
            CAsset asset;
            if(txout.nAsset.IsExplicit())
                asset = txout.nAsset.GetAsset();
            isminetype mine = wtx.txout_is_mine[i];
            if(mine)
            {
                TransactionRecord sub(hash, nTime);
                sub.asset = asset;
                //sub.address = EncodeDestination(wtx.txout_address[i]);
                if(wtx.is_coinstake) // Combine into single output for coinstake
                {
                    sub.idx = 1; // vout index
                    sub.credit = nCredit;
                    sub.debit = nDebit*-1;
                }
                else
                {
                    sub.idx = i; // vout index
                    CAmountMap m;
                    if(txout.nValue.IsExplicit())
                        m[asset] = txout.nValue.GetAmount();
                    sub.credit = m;
                }
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (wtx.txout_address_is_mine[i])
                {
                    // Received by Rain Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = EncodeDestination(wtx.txout_address[i]);
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.is_coinbase)
                {
                    // Generated
                    sub.type = TransactionRecord::Generated;
                }
                if (wtx.is_coinstake)
                {
                    // Staked
                    sub.type = TransactionRecord::Staked;
                }

                if (assets_issued_to_me_only.count(wtx.txout_assets[i])) {
                    sub.type = TransactionRecord::IssuedAsset;
                }

                parts.append(sub);
            }
        }
    }
    else
    {
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        for (const isminetype mine : wtx.txin_is_mine)
        {
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        for (const isminetype mine : wtx.txout_is_mine)
        {
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            std::string address;
            for (auto it = wtx.txout_address.begin(); it != wtx.txout_address.end(); ++it) {
                if (it != wtx.txout_address.begin()) address += ", ";
                address += EncodeDestination(*it);
            }
            CAmountMap mapChange = wtx.change;
            TransactionRecord rsub(hash, nTime);
            rsub.type = TransactionRecord::SendToSelf;
            rsub.debit = (nDebit - mapChange)*-1;
            rsub.credit = nCredit - mapChange;
            rsub.address =address;
            rsub.asset=mapChange.begin()->first;

            parts.append(rsub);
            parts.last().involvesWatchAddress = involvesWatchAddress;   // maybe pass to TransactionRecord as constructor argument
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            CAmountMap mapTxFee = nDebit - wtx.tx->GetValueOutMap();

            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.tx->vout[nOut];
                CAsset asset;
                if(txout.nAsset.IsExplicit())
                    asset = txout.nAsset.GetAsset();
                TransactionRecord sub(hash, nTime);
                sub.idx = nOut;
                sub.involvesWatchAddress = involvesWatchAddress;
                sub.asset=asset;

                if(wtx.txout_is_mine[nOut])
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                if (!std::get_if<CNoDestination>(&wtx.txout_address[nOut]))
                {
                    // Sent to Rain Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = EncodeDestination(wtx.txout_address[nOut]);
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                CAmountMap mapValue = wtx.tx->GetValueOutMap();
                /* Add fee to first output */
                if (mapTxFee > CAmountMap())
                {
                    mapValue += mapTxFee;
                    mapTxFee = CAmountMap();
                }
                sub.debit = mapValue*-1;

                parts.append(sub);
            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            TransactionRecord rsub(hash, nTime);
            rsub.type=TransactionRecord::Other;
            rsub.debit=nNet;
            rsub.credit=CAmountMap();
            parts.append(rsub);
            parts.last().involvesWatchAddress = involvesWatchAddress;
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const interfaces::WalletTxStatus& wtx, const uint256& block_hash, int numBlocks, int64_t block_time)
{
    // Determine transaction status

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d", wtx.block_height, (wtx.is_coinbase || wtx.is_coinstake) ? 1 : 0,  wtx.time_received, idx);
    status.countsForBalance = wtx.is_trusted && !(wtx.blocks_to_maturity > 0);
    status.depth = wtx.depth_in_main_chain;
    status.m_cur_block_hash = block_hash;

    const bool up_to_date = ((int64_t)QDateTime::currentMSecsSinceEpoch() / 1000 - block_time < MAX_BLOCK_TIME_GAP);
    if (up_to_date && !wtx.is_final) {
        if (wtx.lock_time < LOCKTIME_THRESHOLD) {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.lock_time - numBlocks;
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.lock_time;
        }
    }
    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated ||
            type == TransactionRecord::Staked)
    {
        if (wtx.blocks_to_maturity > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.is_in_main_chain)
            {
                status.matures_in = wtx.blocks_to_maturity;
            }
            else
            {
                status.status = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
            if (wtx.is_abandoned)
                status.status = TransactionStatus::Abandoned;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    status.needsUpdate = false;
}

bool TransactionRecord::statusUpdateNeeded(const uint256& block_hash) const
{
    assert(!block_hash.IsNull());
    return status.m_cur_block_hash != block_hash || status.needsUpdate;
}

QString TransactionRecord::getTxHash() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}

bool TransactionRecord::isCoinStake() const
{
    return type == TransactionRecord::Staked;
}
