// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <miner.h>
#include <rpc/util.h>
#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <masternode/masternode-payments.h>
#include <masternode/masternode-sync.h>
#include <net.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <pow.h>
#include <pos.h>
#include <base58.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <timedata.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/validation.h>
#include <validation.h>

#include <wallet/wallet.h>

#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include "evo/specialtx.h"
#include "evo/cbtx.h"
#include "evo/simplifiedmns.h"
#include "evo/deterministicmns.h"

#include "llmq/quorums_blockprocessor.h"
#include "llmq/quorums_chainlocks.h"

#include <algorithm>
#include <queue>
#include <utility>
#include <key_io.h>

CScript controlscript = CScript() << ParseHex("02f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058b") << OP_CHECKSIG;

int64_t nLastCoinStakeSearchTime = 0;
uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:

    return nNewTime - nOldTime;
}

BlockAssembler::Options::Options() {
    blockMinFeeRate = CFeeRate(MAP_DEFAULT_BLOCK_MIN_TX_FEE);
    nBlockMaxWeight = DEFAULT_BLOCK_MAX_WEIGHT;
}

BlockAssembler::BlockAssembler(const CChainParams& params, const Options& options) : chainparams(params)
{
    blockMinFeeRate = options.blockMinFeeRate;
    // Limit weight to between 4K and MAX_BLOCK_WEIGHT-4K for sanity:
    nBlockMaxWeight = std::max<size_t>(4000, std::min<size_t>(MAX_BLOCK_WEIGHT - 4000, options.nBlockMaxWeight));
}

static BlockAssembler::Options DefaultOptions()
{
    // Block resource limits
    // If -blockmaxweight is not given, limit to DEFAULT_BLOCK_MAX_WEIGHT
    BlockAssembler::Options options;
    options.nBlockMaxWeight = gArgs.GetArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    CAmount n = 0;
    if (gArgs.IsArgSet("-blockmintxfee") && ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n)) {
        options.blockMinFeeRate = CFeeRate(populateMap(n));
    } else {
        options.blockMinFeeRate = CFeeRate(MAP_DEFAULT_BLOCK_MIN_TX_FEE);
    }
    return options;
}

BlockAssembler::BlockAssembler(const CChainParams& params) : BlockAssembler(params, DefaultOptions()) {}

void BlockAssembler::resetBlock()
{
    inBlock.clear();

    // Reserve space for coinbase tx
    nBlockWeight = 4000;
    nBlockSigOpsCost = 400;
    fIncludeWitness = false;

    // These counters do not include coinbase tx
    nBlockTx = 0;
    mapFees = CAmountMap();
}

Optional<int64_t> BlockAssembler::m_last_block_num_txs{nullopt};
Optional<int64_t> BlockAssembler::m_last_block_weight{nullopt};

std::unique_ptr<CBlockTemplate> BlockAssembler::CreateNewBlock(const CScript& scriptPubKeyIn, bool fProofOfStake, CWallet* pwallet, bool* pfPoSCancel, std::string strtxcomment, CScript const* commit_script)
{
    int64_t nTimeStart = GetTimeMicros();

    resetBlock();

    pblocktemplate.reset(new CBlockTemplate());

    if(!pblocktemplate.get())
        return nullptr;
    pblock = &pblocktemplate->block; // pointer for convenience

    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    assert(pindexPrev != nullptr);
    nHeight = pindexPrev->nHeight + 1;

    // Create coinbase transaction.
    CMutableTransaction coinbaseTx;
    coinbaseTx.nVersion = 1;
    coinbaseTx.vin.resize(1);
    coinbaseTx.vin[0].prevout.SetNull();
    coinbaseTx.vout.resize(1);
    coinbaseTx.vout[0].scriptPubKey = scriptPubKeyIn;

    std::string h = "Empty";

//    std::vector<unsigned char> v(h.begin(), h.end());
//    coinbaseTx.strTxComment =v;

    // Add dummy coinbase tx as first transaction
    pblock->vtx.emplace_back();
    pblocktemplate->vTxFees.push_back(CAmountMap()); // updated at end
    pblocktemplate->vTxSigOpsCost.push_back(-1); // updated at end

   CAmountMap reward = GetBlockSubsidy(nHeight, chainparams.GetConsensus(), chainparams.GetConsensus().subsidy_asset, false, 0, pindexPrev->nMoneySupply);

    if (!fProofOfStake) {
        coinbaseTx.vout[0].nAsset = reward.begin()->first;
        if(nHeight == 1){
            coinbaseTx.vout.resize(3);
            coinbaseTx.vout[0].nValue = 8000000*COIN;
            coinbaseTx.vout[1].scriptPubKey = scriptPubKeyIn;
            coinbaseTx.vout[1].nAsset = chainparams.GetConsensus().founder_asset;
            coinbaseTx.vout[1].nValue = 1000000*COIN;
            coinbaseTx.vout[2].scriptPubKey = scriptPubKeyIn;
            coinbaseTx.vout[2].nAsset = chainparams.GetConsensus().masternode_asset;
            coinbaseTx.vout[2].nValue = 1000000*COIN;
        }
        else{
            coinbaseTx.vout[0].nValue = reward.begin()->second;
        }
    }

    pblock->nBits = GetNextWorkRequired(pindexPrev, chainparams.GetConsensus(), fProofOfStake);
    pblock->nHeight =nHeight;
    pblock->prevoutStake.SetNull();

    if (fProofOfStake && pwallet)  // attempt to find a coinstake
    {
        *pfPoSCancel = true;
        CMutableTransaction txCoinStake;
        int64_t nSearchTime = txCoinStake.nTime; // search to current time
        COutPoint headerPrevout;
        if (nSearchTime > nLastCoinStakeSearchTime)
        {
            if (pwallet->CreateCoinStake(pblock->nBits, txCoinStake, headerPrevout, pblocktemplate->voutMasternodePayments, pblocktemplate->voutSuperblockPayments))
            {
                if (txCoinStake.nTime >= std::max(pindexPrev->GetMedianTimePast()+1, pindexPrev->GetBlockTime() - MAX_FUTURE_BLOCK_TIME))
                {   // make sure coinstake would meet timestamp protocol
                    // as it would be the same as the block timestamp
                    coinbaseTx.vout[0].SetEmpty();
                    coinbaseTx.vout[0].nAsset = txCoinStake.vout[1].nAsset;
                    coinbaseTx.nTime = txCoinStake.nTime;
                    pblock->vtx.push_back(MakeTransactionRef(CTransaction(txCoinStake)));
                    *pfPoSCancel = false;
                }
            }
            nLastCoinStakeSearchTime = nSearchTime;
        }
        if (*pfPoSCancel)
            return nullptr; // there is no point to continue if we failed to create coinstake
        pblock->prevoutStake = headerPrevout;
    }

    LOCK2(cs_main, mempool.cs);
    pblock->nTime = coinbaseTx.nTime;
    pblock->nVersion = 0x00000001;
    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST) ? nMedianTimePast : pblock->GetBlockTime();

        for (auto& p : chainparams.GetConsensus().llmqs) {
            CTransactionRef qcTx;
            if (llmq::quorumBlockProcessor->GetMinableCommitmentTx(p.first, nHeight, qcTx)) {
                pblock->vtx.emplace_back(qcTx);
                pblocktemplate->vTxFees.emplace_back(CAmountMap());
                pblocktemplate->vTxSigOpsCost.emplace_back(0);
                nBlockWeight += qcTx->GetTotalSize();
                ++nBlockTx;
            }
        }
    
    fIncludeWitness = true;

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;
    addPackageTxs(nPackagesSelected, nDescendantsUpdated);

    int64_t nTime1 = GetTimeMicros();

    m_last_block_num_txs = nBlockTx;
    m_last_block_weight = nBlockWeight;

    coinbaseTx.vin[0].scriptSig = CScript() << nHeight << OP_0;
    // Update coinbase transaction with additional info about masternode and governance payments,
    // get some info back to pass to getblocktemplate
    coinbaseTx.nType = TRANSACTION_COINBASE;

    CCbTx cbTx;
    cbTx.nVersion = 2;
    cbTx.nHeight = nHeight;

    CValidationState state;
    if (!CalcCbTxMerkleRootMNList(*pblock, pindexPrev, cbTx.merkleRootMNList, state)) {
        throw std::runtime_error(strprintf("%s: CalcCbTxMerkleRootMNList failed: %s", __func__, FormatStateMessage(state)));
    }

    if (!CalcCbTxMerkleRootQuorums(*pblock, pindexPrev, cbTx.merkleRootQuorums, state)) {
        throw std::runtime_error(strprintf("%s: CalcCbTxMerkleRootQuorums failed: %s", __func__, FormatStateMessage(state)));
    }

    SetTxPayload(coinbaseTx, cbTx);    

    if (!fProofOfStake)
        FillBlockPayments(coinbaseTx, nHeight, reward, pblocktemplate->voutMasternodePayments, pblocktemplate->voutSuperblockPayments, false);

    pblock->vtx[0] = MakeTransactionRef(std::move(coinbaseTx));
    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev, chainparams.GetConsensus(), fProofOfStake);
    pblocktemplate->vTxFees[0] -= mapFees;

    LogPrint(BCLog::BENCHMARK, "CreateNewBlock(): block weight: %u txs: %u fees: %ld sigops %d\n", GetBlockWeight(*pblock), nBlockTx, mapFees, nBlockSigOpsCost);

    // Fill in header
    pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
    if (pblock->IsProofOfStake())
        pblock->nTime      = pblock->vtx[1]->nTime; //same as coinstake timestamp
    if (pblock->IsProofOfWork())
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nNonce         = 0;
    pblocktemplate->vTxSigOpsCost[0] = WITNESS_SCALE_FACTOR * GetLegacySigOpCount(*pblock->vtx[0]);

    if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
        LogPrintf("%s : TestBlockValidity %s failed: %s \n", __func__, pblock->IsProofOfStake() ? "stake" : "work" ,FormatStateMessage(state));
        return nullptr;
        //throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
    }
    int64_t nTime2 = GetTimeMicros();

    LogPrint(BCLog::BENCHMARK, "CreateNewBlock() packages: %.2fms (%d packages, %d updated descendants), validity: %.2fms (total %.2fms)\n", 0.001 * (nTime1 - nTimeStart), nPackagesSelected, nDescendantsUpdated, 0.001 * (nTime2 - nTime1), 0.001 * (nTime2 - nTimeStart));

    //LogPrintf("Money Supply map size %d, \n Actual Map \n %s  \n", pindexPrev ?  pindexPrev->nMoneySupply.size() : 0, mapToString(pindexPrev->nMoneySupply));

    return std::move(pblocktemplate);
}

void BlockAssembler::onlyUnconfirmed(CTxMemPool::setEntries& testSet)
{
    for (CTxMemPool::setEntries::iterator iit = testSet.begin(); iit != testSet.end(); ) {
        // Only test txs not already in the block
        if (inBlock.count(*iit)) {
            testSet.erase(iit++);
        }
        else {
            iit++;
        }
    }
}

bool BlockAssembler::TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const
{
    // TODO: switch to weight-based accounting for packages instead of vsize-based accounting.
    if (nBlockWeight + WITNESS_SCALE_FACTOR * packageSize >= nBlockMaxWeight)
        return false;
    if (nBlockSigOpsCost + packageSigOpsCost >= MAX_BLOCK_SIGOPS_COST)
        return false;
    return true;
}

// Perform transaction-level checks before adding to block:
// - transaction finality (locktime)
// - premature witness (in case segwit transactions are added to mempool before
//   segwit activation)
bool BlockAssembler::TestPackageTransactions(const CTxMemPool::setEntries& package)
{
    for (CTxMemPool::txiter it : package) {
        if (!IsFinalTx(it->GetTx(), nHeight, nLockTimeCutoff))
            return false;
       if (!llmq::chainLocksHandler->IsTxSafeForMining(it->GetTx().GetHash())) {
            return false;
        }
        if (!fIncludeWitness && it->GetTx().HasWitness())
            return false;
    }
    return true;
}

void BlockAssembler::AddToBlock(CTxMemPool::txiter iter)
{
    pblock->vtx.emplace_back(iter->GetSharedTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOpsCost.push_back(iter->GetSigOpCost());
    nBlockWeight += iter->GetTxWeight();
    ++nBlockTx;
    nBlockSigOpsCost += iter->GetSigOpCost();
    mapFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = gArgs.GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority) {
        LogPrintf("fee %s txid %s\n", CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString(), iter->GetTx().GetHash().ToString());
    }
}

int BlockAssembler::UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded,
        indexed_modified_transaction_set &mapModifiedTx)
{
    int nDescendantsUpdated = 0;
    for (CTxMemPool::txiter it : alreadyAdded) {
        CTxMemPool::setEntries descendants;
        mempool.CalculateDescendants(it, descendants);
        // Insert all descendants (not yet in block) into the modified set
        for (CTxMemPool::txiter desc : descendants) {
            if (alreadyAdded.count(desc))
                continue;
            ++nDescendantsUpdated;
            modtxiter mit = mapModifiedTx.find(desc);
            if (mit == mapModifiedTx.end()) {
                CTxMemPoolModifiedEntry modEntry(desc);
                modEntry.nSizeWithAncestors -= it->GetTxSize();
                modEntry.mapModFeesWithAncestors -= it->GetModifiedFee();
                modEntry.nSigOpCostWithAncestors -= it->GetSigOpCost();
                mapModifiedTx.insert(modEntry);
            } else {
                mapModifiedTx.modify(mit, update_for_parent_inclusion(it));
            }
        }
    }
    return nDescendantsUpdated;
}

// Skip entries in mapTx that are already in a block or are present
// in mapModifiedTx (which implies that the mapTx ancestor state is
// stale due to ancestor inclusion in the block)
// Also skip transactions that we've already failed to add. This can happen if
// we consider a transaction in mapModifiedTx and it fails: we can then
// potentially consider it again while walking mapTx.  It's currently
// guaranteed to fail again, but as a belt-and-suspenders check we put it in
// failedTx and avoid re-evaluation, since the re-evaluation would be using
// cached size/sigops/fee values that are not actually correct.
bool BlockAssembler::SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx)
{
    assert (it != mempool.mapTx.end());
    return mapModifiedTx.count(it) || inBlock.count(it) || failedTx.count(it);
}

void BlockAssembler::SortForBlock(const CTxMemPool::setEntries& package, std::vector<CTxMemPool::txiter>& sortedEntries)
{
    // Sort package by ancestor count
    // If a transaction A depends on transaction B, then A's ancestor count
    // must be greater than B's.  So this is sufficient to validly order the
    // transactions for block inclusion.
    sortedEntries.clear();
    sortedEntries.insert(sortedEntries.begin(), package.begin(), package.end());
    std::sort(sortedEntries.begin(), sortedEntries.end(), CompareTxIterByAncestorCount());
}

// This transaction selection algorithm orders the mempool based
// on feerate of a transaction including all unconfirmed ancestors.
// Since we don't remove transactions from the mempool as we select them
// for block inclusion, we need an alternate method of updating the feerate
// of a transaction with its not-yet-selected ancestors as we go.
// This is accomplished by walking the in-mempool descendants of selected
// transactions and storing a temporary modified state in mapModifiedTxs.
// Each time through the loop, we compare the best transaction in
// mapModifiedTxs with the next transaction in the mempool to decide what
// transaction package to work on next.
void BlockAssembler::addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated)
{
    // mapModifiedTx will store sorted packages after they are modified
    // because some of their txs are already in the block
    indexed_modified_transaction_set mapModifiedTx;
    // Keep track of entries that failed inclusion, to avoid duplicate work
    CTxMemPool::setEntries failedTx;

    // Start by adding all descendants of previously added txs to mapModifiedTx
    // and modifying them for their already included ancestors
    UpdatePackagesForAdded(inBlock, mapModifiedTx);

    CTxMemPool::indexed_transaction_set::index<ancestor_score>::type::iterator mi = mempool.mapTx.get<ancestor_score>().begin();
    CTxMemPool::txiter iter;

    // Limit the number of attempts to add transactions to the block when it is
    // close to full; this is just a simple heuristic to finish quickly if the
    // mempool has a lot of entries.
    const int64_t MAX_CONSECUTIVE_FAILURES = 1000;
    int64_t nConsecutiveFailed = 0;

    while (mi != mempool.mapTx.get<ancestor_score>().end() || !mapModifiedTx.empty())
    {
        // First try to find a new transaction in mapTx to evaluate.
        if (mi != mempool.mapTx.get<ancestor_score>().end() &&
                SkipMapTxEntry(mempool.mapTx.project<0>(mi), mapModifiedTx, failedTx)) {
            ++mi;
            continue;
        }

        // Now that mi is not stale, determine which transaction to evaluate:
        // the next entry from mapTx, or the best from mapModifiedTx?
        bool fUsingModified = false;

        modtxscoreiter modit = mapModifiedTx.get<ancestor_score>().begin();
        if (mi == mempool.mapTx.get<ancestor_score>().end()) {
            // We're out of entries in mapTx; use the entry from mapModifiedTx
            iter = modit->iter;
            fUsingModified = true;
        } else {
            // Try to compare the mapTx entry to the mapModifiedTx entry
            iter = mempool.mapTx.project<0>(mi);
            if (modit != mapModifiedTx.get<ancestor_score>().end() &&
                    CompareTxMemPoolEntryByAncestorFee()(*modit, CTxMemPoolModifiedEntry(iter))) {
                // The best entry in mapModifiedTx has higher score
                // than the one from mapTx.
                // Switch which transaction (package) to consider
                iter = modit->iter;
                fUsingModified = true;
            } else {
                // Either no entry in mapModifiedTx, or it's worse than mapTx.
                // Increment mi for the next loop iteration.
                ++mi;
            }
        }

        // We skip mapTx entries that are inBlock, and mapModifiedTx shouldn't
        // contain anything that is inBlock.
        assert(!inBlock.count(iter));

        uint64_t packageSize = iter->GetSizeWithAncestors();
        CAmountMap packageFees = iter->GetModFeesWithAncestors();
        int64_t packageSigOpsCost = iter->GetSigOpCostWithAncestors();
        if (fUsingModified) {
            packageSize = modit->nSizeWithAncestors;
            packageFees = modit->mapModFeesWithAncestors;
            packageSigOpsCost = modit->nSigOpCostWithAncestors;
        }

        if (packageFees < blockMinFeeRate.GetFee(packageSize)) {
            // Everything else we might consider has a lower fee rate
            return;
        }

        if (!TestPackage(packageSize, packageSigOpsCost)) {
            if (fUsingModified) {
                // Since we always look at the best entry in mapModifiedTx,
                // we must erase failed entries so that we can consider the
                // next best entry on the next loop iteration
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }

            ++nConsecutiveFailed;

            if (nConsecutiveFailed > MAX_CONSECUTIVE_FAILURES && nBlockWeight >
                    nBlockMaxWeight - 4000) {
                // Give up if we're close to full and haven't succeeded in a while
                break;
            }
            continue;
        }

        CTxMemPool::setEntries ancestors;
        uint64_t nNoLimit = std::numeric_limits<uint64_t>::max();
        std::string dummy;
        mempool.CalculateMemPoolAncestors(*iter, ancestors, nNoLimit, nNoLimit, nNoLimit, nNoLimit, dummy, false);

        onlyUnconfirmed(ancestors);
        ancestors.insert(iter);

        // Test if all tx's are Final
        if (!TestPackageTransactions(ancestors)) {
            if (fUsingModified) {
                mapModifiedTx.get<ancestor_score>().erase(modit);
                failedTx.insert(iter);
            }
            continue;
        }

        // This transaction will make it in; reset the failed counter.
        nConsecutiveFailed = 0;

        // Package can be added. Sort the entries in a valid order.
        std::vector<CTxMemPool::txiter> sortedEntries;
        SortForBlock(ancestors, sortedEntries);

        for (size_t i=0; i<sortedEntries.size(); ++i) {
            AddToBlock(sortedEntries[i]);
            // Erase from the modified set, if present
            mapModifiedTx.erase(sortedEntries[i]);
        }

        ++nPackagesSelected;

        // Update transactions that depend on each of these
        nDescendantsUpdated += UpdatePackagesForAdded(ancestors, mapModifiedTx);
    }
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}


bool CheckStake(std::shared_ptr<CBlock> pblock, CWallet& wallet)
{
    LogPrint(BCLog::COINSTAKE, "STAKE BLOCK \n%s\n", pblock->ToString());
    CAmountMap mapout = pblock->vtx[1]->GetValueOutMap();
    LogPrint(BCLog::COINSTAKE, "out \n %s \n", mapToString(mapout));

    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != ::ChainActive().Tip()->GetBlockHash())
            return error("CheckStake() : generated block is stale");
    }

    auto locked_chain = wallet.chain().lock();

    {
        LOCK(wallet.cs_wallet);
        for(const CTxIn& vin : pblock->vtx[1]->vin) {
            if (wallet.IsSpent(*locked_chain,vin.prevout.hash, vin.prevout.n)) {
                return error("CheckStake() : generated block became invalid due to stake UTXO being spent");
            }
        }
    }

    // Process this block the same as if we had received it from another node
    if (!ProcessNewBlock(Params(), pblock, true, nullptr))
        return error("CheckStake() : ProcessBlock, block not accepted");

    return true;
}

// stake minter thread
void ThreadStakeMiner(CWallet *pwallet)
{
    LogPrintf("ThreadStakeMiner started\n");
    unsigned int nExtraNonce = 0;

    // Make this thread recognisable as the mining thread
    std::string threadName = "stake";
    if(pwallet && pwallet->GetName() != "")
    {
        threadName = threadName + "-" + pwallet->GetName();
    }
    util::ThreadRename(threadName.c_str());

    //ReserveDestination reservekey(pwallet);
    CScript dummyscript;
    nLastCoinStakeSearchTime = GetTime();  // only initialized at startup

    bool fTryToSync = true;

    try
    {
        while (true)
        {
            while (pwallet->IsLocked() || (!gArgs.GetBoolArg("-emergencystaking", false) && !masternodeSync.IsSynced()))
            {
                LogPrintf("%s : Not staking \n", __func__);

                if(pwallet->IsLocked())
                    LogPrintf("%s : Wallet Locked \n", __func__);

                if(!masternodeSync.IsSynced())
                    LogPrintf("%s : %s \n", __func__, masternodeSync.GetSyncStatus());

                UninterruptibleSleep(std::chrono::seconds{60});
            }
            //don't disable PoS mining for no connections if in regtest mode
            if(!gArgs.GetBoolArg("-emergencystaking", false)) {
                LogPrint(BCLog::COINSTAKE, "Emergeny Staking Disabled \n");
                while (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0 || ::ChainstateActive().IsInitialBlockDownload()) {
                    fTryToSync = true;
                    LogPrintf("%s : Trying to sync \n", __func__);
                    UninterruptibleSleep(std::chrono::seconds{60});
                }
                if (fTryToSync) {
                    fTryToSync = false;
                    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) < 3 || ::ChainActive().Tip()->GetBlockTime() < GetTime() - 10 * 60) {
                        LogPrintf("%s : Insufficient nodes \n", __func__);
                        UninterruptibleSleep(std::chrono::seconds{60});
                        continue;
                    }
                }
            }
            //
            // Create new block
            //
            if(pwallet->HaveAvailableCoinsForStaking())
            {
                bool fPoSCancel = false;
                // First just create an empty block. No need to process transactions until we know we can create a block
                std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(dummyscript, true, pwallet, &fPoSCancel));
                if (!pblocktemplate.get())
                {
                    if (fPoSCancel == true)
                    {
                        UninterruptibleSleep(std::chrono::milliseconds{500});
                        continue;
                    }
                    LogPrintf("Error in ThreadStakeMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                    return;
                }
                CBlock *pblock = &pblocktemplate->block;
                IncrementExtraNonce(pblock, ::ChainActive().Tip(), nExtraNonce);

                // if proof-of-stake block found then process block
                if (pblock->IsProofOfStake())
                {
                    pblock->nTime = pblock->vtx[1]->nTime;
                    std::shared_ptr<CBlock> shared_pblock = std::make_shared<CBlock>(*pblock);
                    if (!SignBlock(shared_pblock, pwallet))
                    {
                        LogPrint(BCLog::COINSTAKE, "PoSMiner(): failed to sign PoS block");
                        return;
                    }

                    LogPrintf("CPUMiner : proof-of-stake block found %s\n", pblock->GetHash().ToString());
                    CheckStake(shared_pblock, *pwallet);
                }
            }
            else {
                LogPrintf("PoSMiner(): no available coins\n");
                UninterruptibleSleep(std::chrono::seconds{600});
            }
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("ThreadStakeMiner terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("ThreadStakeMiner runtime error: %s\n", e.what());
        return;
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "ThreadStakeMiner()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "ThreadStakeMiner()");
    }
    LogPrintf("ThreadStakeMiner exiting\n");
}

void Stake(bool fStake, CWallet *pwallet, boost::thread_group*& stakeThread)
{
    if (stakeThread != nullptr)
    {
        stakeThread->interrupt_all();
        delete stakeThread;
        stakeThread = nullptr;
    }
    if(fStake)
    {
        stakeThread = new boost::thread_group();
        stakeThread->create_thread(boost::bind(&ThreadStakeMiner, pwallet));
    }

}

//////////////////////////////////////////////////////////////////////////////
//
// Proof of Work miner
//

double dHashesPerMin = 0.0;
int64_t nHPSTimerStart = 0;

void static RainMiner(std::shared_ptr<CTxDestination> coinbase_script)
{
    const CChainParams& chainparams = Params();
    LogPrintf("RainMiner started\n");
    util::ThreadRename("miner");

    unsigned int nExtraNonce = 0;

    try {
        // Throw an error if no script was provided.  This can happen
        // due to some internal error but also if the keypool is empty.
        // In the latter case, already the pointer is nullptr.

        while (true) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    if(!g_connman)
                        break;

                    if (g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL) == 0)
                        break;

                    if (::ChainstateActive().IsInitialBlockDownload())
                        break;
                     UninterruptibleSleep(std::chrono::milliseconds{1000});
                } while (true);

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = ::ChainActive().Tip();
            static std::unique_ptr<CBlockTemplate> pblocktemplate;

            pblocktemplate = BlockAssembler(Params()).CreateNewBlock(GetScriptForDestination(*coinbase_script));
            if (!pblocktemplate.get())
                throw std::runtime_error(strprintf("%s: Couldn't create new block", __func__));

            CBlock *pblock = &pblocktemplate->block;
            {
                LOCK(cs_main);
                IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
            }

            LogPrintf("Running RainMiner with %u transactions in block \n", pblock->vtx.size());

            //
            // Search
            //
            int64_t nStart = GetTime();
            arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
            uint256 testHash;
            for (;;)
            {
                unsigned int nHashesDone = 0;
                unsigned int nNonceFound = (unsigned int) -1;

                for(int i=0;i<1;i++){
                    pblock->nNonce=pblock->nNonce+1;
                    testHash=pblock->GetHash();
                    nHashesDone++;
                    //LogPrintf("testHash %s\n", testHash.ToString().c_str());
                    //LogPrintf("Hash Target %s\n", hashTarget.ToString().c_str());

                    if(UintToArith256(testHash)<hashTarget){
                        nNonceFound=pblock->nNonce;
                        LogPrintf("Found Hash %s\n", testHash.ToString().c_str());
                        // Found a solution
                        assert(testHash == pblock->GetHash());

                        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
                        if (!ProcessNewBlock(Params(), shared_pblock, true, nullptr))
                            throw std::runtime_error(strprintf("%s: ProcessNewBlock, block not accepted:", __func__));
                        LogPrintf("Found Hash %s\n", shared_pblock->ToString().c_str());
                        break;
                    }
                }

                // Meter hashes/sec
                static int64_t nHashCounter;
                if (nHPSTimerStart == 0)
                {
                    nHPSTimerStart = GetTimeMillis();
                    nHashCounter = 0;
                }
                else
                    nHashCounter += nHashesDone;
                if (GetTimeMillis() - nHPSTimerStart > 4000*60)
                {
                    static RecursiveMutex cs;
                    {
                        LOCK(cs);
                        if (GetTimeMillis() - nHPSTimerStart > 4000*60)
                        {
                            dHashesPerMin = 1000.0 * nHashCounter *60 / (GetTimeMillis() - nHPSTimerStart);
                            nHPSTimerStart = GetTimeMillis();
                            nHashCounter = 0;
                            static int64_t nLogTime;
                            if (GetTime() - nLogTime > 30 * 60)
                            {
                                nLogTime = GetTime();
                                LogPrintf("hashmeter %6.0f khash/s\n", dHashesPerMin/1000.0);
                            }
                        }
                    }
                }

                // Check for stop or if block needs to be rebuilt
                boost::this_thread::interruption_point();
                // Regtest mode doesn't require peers

                if (nNonceFound >= 0xffff0000)
                    break;
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (pindexPrev != ::ChainActive().Tip())
                    break;

                // Update nTime every few seconds
                UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
            }
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("RainMiner terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("RainMiner runtime error: %s\n", e.what());
        return;
    }
}

void GenerateRains(bool fGenerate, int nThreads, std::shared_ptr<CTxDestination> coinbase_script)
{
    static boost::thread_group* minerThreads = nullptr;

    if (nThreads < 0)
        nThreads = GetNumCores();

    if (minerThreads != nullptr)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = nullptr;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&RainMiner, boost::cref(coinbase_script)));
}
