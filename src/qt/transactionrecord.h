// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_TRANSACTIONRECORD_H
#define RAIN_QT_TRANSACTIONRECORD_H

#include <amount.h>
#include <primitives/asset.h>
#include "script/script.h"
#include <uint256.h>
#include <key_io.h>

#include <QList>
#include <QString>

namespace interfaces {
class Node;
class Wallet;
struct WalletTx;
struct WalletTxStatus;
}

/** UI model for transaction status. The transaction status is the part of a transaction that will change over time.
 */
class TransactionStatus
{
public:
    TransactionStatus():
        countsForBalance(false), sortKey(""),
        matures_in(0), status(Unconfirmed), depth(0), open_for(0), cur_num_blocks(-1)
    { }

    enum Status {
        Confirmed,          /**< Have 6 or more confirmations (normal tx) or fully mature (mined tx) **/
        /// Normal (sent/received) transactions
        OpenUntilDate,      /**< Transaction not yet final, waiting for date */
        OpenUntilBlock,     /**< Transaction not yet final, waiting for block */
        Unconfirmed,        /**< Not yet mined into a block **/
        Confirming,         /**< Confirmed, but waiting for the recommended number of confirmations **/
        Conflicted,         /**< Conflicts with other transaction or mempool **/
        Abandoned,          /**< Abandoned from the wallet **/
        /// Generated (mined) transactions
        Immature,           /**< Mined but waiting for maturity */
        NotAccepted         /**< Mined but not accepted */
    };

    /// Transaction counts towards available balance
    bool countsForBalance;
    /// Sorting key based on status
    std::string sortKey;
    /// Label
    QString label;

    /** @name Generated (mined) transactions
       @{*/
    int matures_in;
    /**@}*/

    /** @name Reported status
       @{*/
    Status status;
    qint64 depth;
    qint64 open_for; /**< Timestamp if status==OpenUntilDate, otherwise number
                      of additional blocks that need to be mined before
                      finalization */
    /**@}*/

    /** Current number of blocks (to know whether cached status is still valid) */
    int cur_num_blocks;

    /** Current number of blocks based on the headers chain */
    int cur_num_blocks_headers_chain;

    /** true if transaction has been validated (false == spv) */
    bool fValidated;

    bool needsUpdate;
};

/** UI model for a transaction. A core transaction can be represented by multiple UI transactions if it has
    multiple outputs.
 */
class TransactionRecord
{
public:
    enum Type
    {
        Other,
        Generated,
        Staked,
        SendToAddress,
        SendToOther,
        RecvWithAddress,
        MNReward,
        RecvFromOther,
        SendToSelf,
        Fee,
        IssuedAsset,
        StakeDelegated, // Received cold stake (owner)
        StakeHot, // Staked via a delegated P2CS.
        P2CSDelegation, // Non-spendable P2CS, staker side.
        P2CSDelegationSent, // Non-spendable P2CS delegated utxo. (coin-owner transferred ownership to external wallet)
        P2CSDelegationSentOwner, // Spendable P2CS delegated utxo. (coin-owner)
        P2CSUnlockOwner, // Coin-owner spent the delegated utxo
        P2CSUnlockStaker // Staker watching the owner spent the delegated utxo
    };

    /** Number of confirmation recommended for accepting a transaction */
    static const int RecommendedNumConfirmations = 6;

    TransactionRecord():
            hash(), time(0), type(Other), address(""), debit(CAmountMap()), credit(CAmountMap()), idx(0)
    {
        txDest = DecodeDestination(address);
    }

    TransactionRecord(uint256 _hash, qint64 _time):
            hash(_hash), time(_time), type(Other), address(""), debit(CAmountMap()), credit(CAmountMap()), idx(0)
    {
        txDest = DecodeDestination(address);
    }

    TransactionRecord(uint256 _hash, qint64 _time,
                Type _type, const std::string &_address,
                const CAmountMap& _debit, const CAmountMap& _credit, const CAsset& _asset):
            hash(_hash), time(_time), type(_type), address(_address), debit(_debit), credit(_credit),
            asset(_asset),
            idx(0)
    {
        txDest = DecodeDestination(address);
    }

    /** Decompose CWallet transaction to model transaction records.
     */
    static bool showTransaction();
    static QList<TransactionRecord> decomposeTransaction(const interfaces::WalletTx& wtx, interfaces::Wallet& wallet);

    /// Helpers
    static bool ExtractAddress(const CScript& scriptPubKey, bool fColdStake, std::string& addressStr);
    static void loadHotOrColdStakeOrContract(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx,
                                            TransactionRecord& record, bool isContract = false);
    static void loadUnlockColdStake(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx, TransactionRecord& record);

    /** @name Immutable transaction attributes
      @{*/
    uint256 hash;
    qint64 time;
    Type type;
    std::string address;
    CTxDestination txDest;
    CAmountMap debit;
    CAmountMap credit;
    CAsset asset;
    std::string txcomment;
    unsigned int size;
    /**@}*/

    /** Subtransaction index, for sort key */
    int idx;

    /** Status: can change with block chain update */
    TransactionStatus status;

    /** Whether the transaction was sent/received with a watch-only address */
    bool involvesWatchAddress;

    /** Return the unique identifier for this transaction (part) */
    QString getTxHash() const;

    /** Return the output index of the subtransaction  */
    int getOutputIndex() const;

    /** Update status from core wallet tx.
     */
    void updateStatus(const interfaces::WalletTxStatus& wtx, int numBlocks, int64_t block_time);

    /** Return whether a status update is needed.
     */
    bool statusUpdateNeeded(int numBlocks) const;
    /** Return transaction status
     */
    std::string statusToString();

    /** Return true if the tx is a coinstake
     */
    bool isCoinStake() const;

    /** Return true if the tx is a any cold staking type tx.
     */
    bool isAnyColdStakingType() const;

    /** Return true if the tx hash is null and/or if the size is 0
     */
    bool isNull() const;

};

#endif // RAIN_QT_TRANSACTIONRECORD_H
