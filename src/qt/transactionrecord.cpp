// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <chain.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <policy/policy.h>
#include <wallet/ismine.h>
#include <wallet/wallet.h>

#include <confidential_validation.h>
#include <stdint.h>

#include <QDateTime>
#include <QDebug>
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
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const interfaces::WalletTx& wtx, interfaces::Wallet& wallet)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.time;
    CAmountMap nCredit = wtx.credit;
    CAmountMap nDebit = wtx.debit;
    CAmountMap nNet = nCredit - nDebit;
    uint256 hash = wtx.tx->GetHash();
    //std::string txcomment = wtx.tx->strTxComment;
//    std::string txcomment(wtx.tx->strTxComment.begin(), wtx.tx->strTxComment.end());
    std::map<std::string, std::string> mapValue = wtx.value_map;
    bool cbcs = false;
    cbcs = wtx.is_coinstake || wtx.is_coinbase;
    const CWalletTx* m_wtx = wallet.getWallet()->GetWalletTx(wtx.tx->GetHash());

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
            const CAsset& asset = wtx.tx->vout[i].nAsset.GetAsset();
            isminetype mine = wtx.txout_is_mine[i];
            if(mine)
            {
                TransactionRecord sub(hash, nTime);
                sub.asset = asset;
                sub.address = EncodeDestination(wtx.txout_address[i]);
                if(wtx.is_coinstake) // Combine into single output for coinstake
                {
                    sub.idx = 1; // vout index
                    sub.credit = nNet;
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
//                sub.txcomment = txcomment;
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
                else if (wtx.is_coinstake)
                {
                    sub.type = TransactionRecord::Staked;
                    if (isminetype mine = wtx.txout_address_is_mine[1]) {
                        // Check for cold stakes.
                        if (wtx.tx->HasP2CSOutputs()) {
                            sub.credit = nCredit;
                            sub.debit = nDebit;
                            loadHotOrColdStakeOrContract(wallet, wtx, sub);
                            parts.append(sub);
                            return parts;
                        } else {
                            // watch stake reward
                            sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                            sub.type = TransactionRecord::Staked;
                            sub.credit = nCredit;
                            sub.debit = nDebit;
                        }
                    } else {
                        //Masternode reward
                        CTxDestination destMN;
                        int nIndexMN = wtx.tx->vout.size() - 1;
                        if (ExtractDestination(wtx.tx->vout[nIndexMN].scriptPubKey, destMN) && wallet.isMine(destMN)) {
                            isminetype mine = wallet.txoutIsMine(wtx.tx->vout[nIndexMN]);
                            sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                            sub.type = TransactionRecord::MNReward;
                            sub.asset = wtx.tx->vout[nIndexMN].nAsset.GetAsset();
                            sub.address = EncodeDestination(destMN);
                            CAmountMap thy;
                            thy[wtx.tx->vout[nIndexMN].nAsset.GetAsset()] = wtx.tx->vout[nIndexMN].nValue.GetAmount();
                            sub.credit = thy;
                        }
                    }
                    parts.append(sub);
                }  
                else if (wtx.tx->HasP2CSOutputs()) {
                    // Delegate tx.
                    sub.credit = nCredit;
                    sub.debit = nDebit;
                    loadHotOrColdStakeOrContract(wallet, wtx, sub, true);
                    parts.append(sub);
                    return parts;
                } 
                else if (m_wtx->HasP2CSInputs()) {
                    // Delegation unlocked
                    loadUnlockColdStake(wallet, wtx, sub);
                    parts.append(sub);
                    return parts;
                }

                if (assets_issued_to_me_only.count(wtx.txout_assets[i])) {
                    sub.type = TransactionRecord::IssuedAsset;
                }

                parts.append(sub);
                //if(wtx.is_coinstake)
                //    break; // Single output for coinstake
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
            CAmountMap mapChange = wtx.change;
            TransactionRecord rsub(hash, nTime);
            rsub.type = TransactionRecord::SendToSelf;
            rsub.debit = (nDebit - mapChange)*-1;
            rsub.credit = nCredit - mapChange;
            rsub.address ="";
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
                const CAsset& asset = wtx.txout_assets[nOut];
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

                if (!boost::get<CNoDestination>(&wtx.txout_address[nOut]))
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
            // will need to deal with this and assets
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

void TransactionRecord::loadUnlockColdStake(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx, TransactionRecord& record)
{
    record.involvesWatchAddress = false;
    const CWalletTx* m_wtx = wallet.getWallet()->GetWalletTx(wtx.tx->GetHash());

    // Get the p2cs
    const CScript* p2csScript = nullptr;
    bool isSpendable = false;

    for (const auto &input : wtx.tx->vin) {
        const CWalletTx* tx = wallet.getWallet()->GetWalletTx(input.prevout.hash);
        if (tx && tx->tx->vout[input.prevout.n].scriptPubKey.IsPayToColdStaking()) {
            p2csScript = &tx->tx->vout[input.prevout.n].scriptPubKey;
            isSpendable = wallet.txinIsMine(input) & ISMINE_SPENDABLE_ALL;
            break;
        }
    }

    if (isSpendable) {
        // owner unlocked the cold stake
        record.type = TransactionRecord::P2CSUnlockOwner;
        record.debit = (m_wtx->GetStakeDelegationDebit())*-1;
        record.credit = wtx.credit;
    } else {
        // hot node watching the unlock
        record.type = TransactionRecord::P2CSUnlockStaker;
        record.debit = (m_wtx->GetColdStakingDebit())*-1;
        record.credit = (m_wtx->GetColdStakingCredit())*-1;
    }

    // Extract and set the owner address
    ExtractAddress(*p2csScript, false, record.address);
}

void TransactionRecord::loadHotOrColdStakeOrContract(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx,
        TransactionRecord& record,
        bool isContract)
{
    record.involvesWatchAddress = false;
    const CWalletTx* m_wtx = wallet.getWallet()->GetWalletTx(wtx.tx->GetHash());

    // Get the p2cs
    CTxOut p2csUtxo;
    for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++) {
        const CTxOut &txout = wtx.tx->vout[nOut];
        if (txout.scriptPubKey.IsPayToColdStaking()) {
            p2csUtxo = txout;
            break;
        }
    }

    bool isSpendable = (wallet.txoutIsMine(p2csUtxo) & ISMINE_SPENDABLE_DELEGATED);
    bool isFromMe = wallet.getWallet()->IsFromMe(*wtx.tx.get());

    if (isContract) {
        if (isSpendable && isFromMe) {
            // Wallet delegating balance
            record.type = TransactionRecord::P2CSDelegationSentOwner;
        } else if (isFromMe){
            // Wallet delegating balance and transfering ownership
            record.type = TransactionRecord::P2CSDelegationSent;
        } else {
            // Wallet receiving a delegation
            record.type = TransactionRecord::P2CSDelegation;
        }
    } else {
        // Stake
        if (isSpendable) {
            // Offline wallet receiving an stake due a delegation
            record.type = TransactionRecord::StakeDelegated;
            record.credit = m_wtx->GetStakeDelegationCredit();
            record.debit = (m_wtx->GetDebit(ISMINE_SPENDABLE_DELEGATED))*-1;
        } else {
            // Online wallet receiving an stake due a received utxo delegation that won a block.
            record.type = TransactionRecord::StakeHot;
        }
    }

    // Extract and set the owner address
    ExtractAddress(p2csUtxo.scriptPubKey, false, record.address);
}

bool TransactionRecord::ExtractAddress(const CScript& scriptPubKey, bool fColdStake, std::string& addressStr) {
    CTxDestination address;
    txnouttype *whichType = nullptr;
    if (!ExtractDestination(scriptPubKey, address, whichType, fColdStake)) {
        // this shouldn't happen..
        addressStr = "No available address";
        return false;
    } else {
        addressStr = EncodeDestination(
                address,
                (fColdStake ? CChainParams::STAKING_ADDRESS : CChainParams::PUBKEY_ADDRESS)
        );
        return true;
    }
}


void TransactionRecord::updateStatus(const interfaces::WalletTxStatus& wtx, int numBlocks, int64_t adjustedTime)
{
    // Determine transaction status

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d", wtx.block_height, (wtx.is_coinbase || wtx.is_coinstake) ? 1 : 0,  wtx.time_received, idx);
    status.countsForBalance = wtx.is_trusted && !(wtx.blocks_to_maturity > 0);
    status.depth = wtx.depth_in_main_chain;
    status.cur_num_blocks = numBlocks;
//    status.cur_num_blocks_headers_chain = numHeaders;

    const bool up_to_date = ((int64_t)QDateTime::currentMSecsSinceEpoch() / 1000 - adjustedTime < MAX_BLOCK_TIME_GAP);
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
            type == TransactionRecord::MNReward ||
            type == TransactionRecord::StakeDelegated ||
            type == TransactionRecord::StakeHot)
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

bool TransactionRecord::statusUpdateNeeded(int numBlocks) const
{
    return (status.cur_num_blocks != numBlocks || status.needsUpdate);
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
    return (type == TransactionRecord::Staked || type == TransactionRecord::Generated);
}

bool TransactionRecord::isAnyColdStakingType() const
{
    return (type == TransactionRecord::P2CSDelegation || type == TransactionRecord::P2CSDelegationSent
            || type == TransactionRecord::P2CSDelegationSentOwner
            || type == TransactionRecord::StakeDelegated || type == TransactionRecord::StakeHot
            || type == TransactionRecord::P2CSUnlockOwner || type == TransactionRecord::P2CSUnlockStaker);
}

bool TransactionRecord::isNull() const
{
    return hash.IsNull() || size == 0;
}

std::string TransactionRecord::statusToString(){
    switch (status.status){
        case TransactionStatus::Confirmed:
            return "Confirmed";
        case TransactionStatus::OpenUntilDate:
            return "OpenUntilDate";
        case TransactionStatus::OpenUntilBlock:
            return "OpenUntilBlock";
        case TransactionStatus::Unconfirmed:
            return "Unconfirmed";
        case TransactionStatus::Confirming:
            return "Confirming";
        case TransactionStatus::Conflicted:
            return "Conflicted";
        case TransactionStatus::Immature:
            return "Immature";
        case TransactionStatus::NotAccepted:
            return "Not Accepted";
        default:
            return "No status";
    }
}
