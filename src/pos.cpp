// Copyright (c) 2012-2013 The PPCoin developers
// Copyright (c) 2014 The BlackCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>
#include <index/txindex.h>

#include <bignum.h>
#include <pos.h>
#include <key_io.h>
#include <logging.h>
#include <validation.h>

static const int MODIFIER_INTERVAL_RATIO = 3;

// Get time weight
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{
    // Kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low

    return std::min(nIntervalEnd - nIntervalBeginning - Params().GetConsensus().nStakeMinAge, (int64_t)Params().GetConsensus().nStakeMaxAge);
}

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifier: no generation at genesis block");
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}


static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert (nSection >= 0 && nSection < 64);
    return (Params().GetConsensus().nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (unsigned int nSection=0; nSection<64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    return nSelectionInterval;
}

static bool SelectBlockFromCandidates(std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp, std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev, const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    arith_uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*) 0;
    for (const auto& item : vSortedByTimestamp)
    {
        if (!mapBlockIndex.count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString());
        const CBlockIndex* pindex = mapBlockIndex[item.second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;
        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;
        // compute the selection hash by hashing its proof-hash and the
        // previous proof-of-stake modifier
        uint256 hashProof = pindex->IsProofOfStake()? pindex->hashProof : pindex->GetBlockHash();
        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
        arith_uint256 hashSelection = UintToArith256(Hash(ss.begin(), ss.end()));
        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;
        if (fSelected && hashSelection < hashBest)
        {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
        else if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
    }
    if (gArgs.GetBoolArg("-debug", false) && gArgs.GetBoolArg("-printstakemodifier", false))
        LogPrintf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString());
    return fSelected;
}


bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    const Consensus::Params& params = Params().GetConsensus();
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    }
    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    if (gArgs.GetBoolArg("-debug", false))
        LogPrintf("ComputeNextStakeModifier: prev modifier=0x%016x time=%s epoch=%u\n", nStakeModifier, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nModifierTime), (unsigned int)nModifierTime);
    if (nModifierTime / params.nModifierInterval >= pindexPrev->GetBlockTime() / params.nModifierInterval)
    {
        if (gArgs.GetBoolArg("-debug", false))
            LogPrintf("ComputeNextStakeModifier: no new interval keep current modifier: pindexPrev nHeight=%d nTime=%u\n", pindexPrev->nHeight, (unsigned int)pindexPrev->GetBlockTime());
        return true;
    }

    // Sort candidate blocks by timestamp
    std::vector<std::pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * params.nModifierInterval / params.nPowTargetSpacing);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / params.nModifierInterval) * params.nModifierInterval - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(std::make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;

    // Shuffle before sort
    for(int i = vSortedByTimestamp.size() - 1; i > 1; --i)
    std::swap(vSortedByTimestamp[i], vSortedByTimestamp[GetRand(i)]);

    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end(), [] (const std::pair<int64_t, uint256> &a, const std::pair<int64_t, uint256> &b)
    {
        if (a.first != b.first)
            return a.first < b.first;
        // Timestamp equals - compare block hashes
        const uint32_t *pa = a.second.GetDataPtr();
        const uint32_t *pb = b.second.GetDataPtr();
        int cnt = 256 / 32;
        do {
            --cnt;
            if (pa[cnt] != pb[cnt])
                return pa[cnt] < pb[cnt];
        } while(cnt);
            return false; // Elements are equal
    });

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    std::map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound=0; nRound < std::min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(std::make_pair(pindex->GetBlockHash(), pindex));
        if (gArgs.GetBoolArg("-debug", false) && gArgs.GetBoolArg("-printstakemodifier", false))
            LogPrintf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n",
                nRound, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nSelectionIntervalStop), pindex->nHeight, pindex->GetStakeEntropyBit());
    }


    if (gArgs.GetBoolArg("-debug", false) && gArgs.GetBoolArg("-printstakemodifier", false))
    {
        std::string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        for (const auto& item : mapSelectedBlocks)
        {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        }
        LogPrintf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap);
    }
    if (gArgs.GetBoolArg("-debug", false))
        LogPrintf("ComputeNextStakeModifier: new modifier=0x%016x time=%s\n", nStakeModifierNew, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pindexPrev->GetBlockTime()));

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
static bool GetKernelStakeModifier(CValidationState& state, CBlockIndex* pindexPrev, uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    const Consensus::Params& params = Params().GetConsensus();
    nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "block-not-found", strprintf(" %s, block %s not indexed ", __func__, hashBlockFrom.ToString()));
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();

    // we need to iterate index forward but we cannot depend on chainActive.Next()
    // because there is no guarantee that we are checking blocks in active chain.
    // So, we construct a temporary chain that we will iterate over.
    // pindexFrom - this block contains coins that are used to generate PoS
    // pindexPrev - this is a block that is previous to PoS block that we are checking, you can think of it as tip of our chain
    std::vector<CBlockIndex*> tmpChain;
    int32_t nDepth = pindexPrev->nHeight - (pindexFrom->nHeight-1); // -1 is used to also include pindexFrom
    tmpChain.reserve(nDepth);
    CBlockIndex* it = pindexPrev;
    for (int i=1; i<=nDepth && !::ChainActive().Contains(it); i++) {
        tmpChain.push_back(it);
        it = it->pprev;
    }
    std::reverse(tmpChain.begin(), tmpChain.end());
    size_t n = 0;

    const CBlockIndex* pindex = pindexFrom;
    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval)
    {
        const CBlockIndex* old_pindex = pindex;
        pindex = (!tmpChain.empty() && pindex->nHeight >= tmpChain[0]->nHeight - 1)? tmpChain[n++] : ::ChainActive().Next(pindex);
        if (n > tmpChain.size() || pindex == NULL) // check if tmpChain[n+1] exists
        {   // reached best block; may happen if node is behind on block chain
            if (fPrintProofOfStake || (old_pindex->GetBlockTime() + params.nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "reached-best-height", strprintf(" %s, reached best block %s at height %d from block %s ", __func__, old_pindex->GetBlockHash().ToString(), old_pindex->nHeight, hashBlockFrom.ToString()));
            else
                return false;
        }
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}
// Get selection interval section (in seconds)

bool CheckStakeKernelHash(CValidationState& state, unsigned int nBits, CBlockIndex* pindexPrev, const CBlockHeader& blockFrom, unsigned int nTxPrevOffset, const CTransactionRef& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProof, bool fPrintProofOfStake)
{
    unsigned int nTimeTxPrev = txPrev->nTime;
    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();

    if (nTimeTxPrev == 0)
        nTimeTxPrev = nTimeBlockFrom;

    if (nTimeTx < nTimeTxPrev)  // Transaction timestamp violation
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "time-error", strprintf(" %s, nTime violation on coinstake %s", __func__, txPrev->GetHash().ToString()));

    if (nTimeBlockFrom + Params().GetConsensus().nStakeMinAge > nTimeTx) // Min age requirement
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "min-age-error", strprintf(" %s, min age violation on coinstake %s", __func__, txPrev->GetHash().ToString()));

    arith_uint256 bnTargetPerCoinDay = arith_uint256().SetCompact(nBits);;
    int64_t nValueIn = txPrev->vout[prevout.n].nValue;
    int64_t nTimeWeight = GetWeight((int64_t)nTimeTxPrev, (int64_t)nTimeTx);
    arith_uint256 bnCoinDayWeight = arith_uint256(nValueIn) * nTimeWeight / COIN / (24 * 60 * 60);
    bnCoinDayWeight *= bnTargetPerCoinDay;

    uint64_t nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;

    if (!GetKernelStakeModifier(state, pindexPrev, blockFrom.GetHash(), nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
        return false;

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    ss << nStakeModifier;
    ss << nTimeBlockFrom << nTxPrevOffset << nTimeTxPrev << prevout.n << nTimeTx;
    hashProof = Hash(ss.begin(), ss.end());

    if (fPrintProofOfStake)
    {
        LogPrintf("%s : using modifier 0x%016x at height=%d timestamp=%s for block from height=%d timestamp=%s\n", __func__,
            nStakeModifier, nStakeModifierHeight,
            DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nStakeModifierTime),
            mapBlockIndex[blockFrom.GetHash()]->nHeight,
            DateTimeStrFormat("%Y-%m-%d %H:%M:%S", blockFrom.GetBlockTime()));
        LogPrintf("%s : check modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n", __func__,
            nStakeModifier,
            nTimeBlockFrom, nTxPrevOffset, nTimeTxPrev, prevout.n, nTimeTx,
            hashProof.ToString());
    }

    // Now check if proof-of-stake hash meets target protocol
    if (UintToArith256(hashProof) > bnCoinDayWeight)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "target-error", strprintf(" %s, proof-of-stake hash does not meet target at height %d, hashProof=%s , target = %s", __func__, pindexPrev->nHeight, UintToArith256(hashProof).ToString(), bnCoinDayWeight.ToString()));

    return true;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(CValidationState& state, CBlockIndex* pindexPrev, const CTransactionRef& tx, unsigned int nBits, uint256& hashProof, CCoinsViewCache& view)
{
    if (!tx->IsCoinStake())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "check-pos-on-non-pos", strprintf("CheckProofOfStake() :  called on non-coinstake %s ", tx->vin[0].prevout.hash.ToString()));

    // Kernel (input 0) must match the stake hash target (nBits)
    const CTxIn& txin = tx->vin[0];

    Coin coinPrev;
    {
        LOCK(cs_main);
        if(!view.GetCoin(txin.prevout, coinPrev)){
            if(!GetSpentCoinFromMainChain(pindexPrev, txin.prevout, &coinPrev)) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "no-stake-prevout", strprintf(" Stake prevout does not exist for %s ", txin.prevout.hash.ToString()));
            }
        }
    }

    if (!g_txindex)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "tx-index-disabled", strprintf("%s: transaction index is not enabled ", __func__));

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(txin.prevout.hash, postx))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "tx-index-not-found", strprintf("%s: transaction index not available ", __func__));

    // Read txPrev and header of its block
    CBlockHeader header;
    CTransactionRef txPrev;
    {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception &e) {
            return error("%s() : deserialize or I/O error in CheckProofOfStake()", __PRETTY_FUNCTION__);
        }
        if (txPrev->GetHash() != txin.prevout.hash)
            return error("%s() : txid mismatch in CheckProofOfStake()", __PRETTY_FUNCTION__);
    }

    {
        int nIn = 0;
        const CTxOut& prevOut = txPrev->vout[tx->vin[nIn].prevout.n];
        TransactionSignatureChecker checker(&(*tx), nIn, prevOut.nValue, PrecomputedTransactionData(*tx));

        if (!VerifyScript(tx->vin[nIn].scriptSig, prevOut.scriptPubKey, &(tx->vin[nIn].scriptWitness), SCRIPT_VERIFY_P2SH, checker, nullptr))
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "invalid-pos-script", strprintf(" VerifyScript failed on coinstake %s ", tx->GetHash().ToString()));
    }

    if (!CheckStakeKernelHash(state, nBits, pindexPrev, header, postx.nTxOffset + CBlockHeader::NORMAL_SERIALIZE_SIZE, txPrev, txin.prevout, tx->nTime, hashProof, gArgs.GetBoolArg("-debug", false)))
        return false;

    return true;
}

bool CheckKernel(CValidationState& state, unsigned int nBits, uint32_t nTime, const COutPoint& prevout)
{
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    uint256 hashProof;
    const Consensus::Params& params = Params().GetConsensus();

    if (!g_txindex)
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "tx-index-disabled", strprintf("%s: transaction index is not enabled ", __func__));

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(prevout.hash, postx))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "tx-index-not-found", strprintf("%s: transaction index not available ", __func__));

    // Read txPrev and header of its block
    CBlockHeader header;
    CTransactionRef txPrev;
    {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception &e) {
            return error("%s() : deserialize or I/O error in CheckKernel()", __PRETTY_FUNCTION__);
        }
        if (txPrev->GetHash() != prevout.hash)
            return error("%s() : txid mismatch in CheckKernel()", __PRETTY_FUNCTION__);
    }

    // Try finding the previous transaction in database
    uint256 hashBlock;
    CTransactionRef txPrevDummy;
    if (!GetTransaction(prevout.hash, txPrevDummy, params, hashBlock)) {
        LogPrintf("CheckKernel() : could not find previous transaction %s\n", prevout.hash.ToString());
        return false;
    }
    if (mapBlockIndex.count(hashBlock) == 0) {
        LogPrintf("CheckKernel() : could not find block of previous transaction %s\n", hashBlock.ToString());
        return false;
    }

    // Check minimum age requirement
    if (header.GetBlockTime() + params.nStakeMinAge > nTime) {
        // LogPrintf("CheckKernel() : stake prevout is not mature in block %s\n", hashBlock.ToString());
        return false;
    }

    return CheckStakeKernelHash(state, nBits, pindexPrev, header, postx.nTxOffset + CBlockHeader::NORMAL_SERIALIZE_SIZE, txPrev, prevout, nTime, hashProof, gArgs.GetBoolArg("-debug", false));
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(uint32_t nTimeBlock, uint32_t nTimeTx)
{
    return (nTimeBlock == nTimeTx);
}

// entropy bit for stake modifier if chosen by modifier
unsigned int GetStakeEntropyBit(const CBlock& block)
{
    unsigned int nEntropyBit = ((UintToArith256(block.GetHash()).GetLow64()) & 1llu);
    if (gArgs.GetBoolArg("-printstakemodifier", false))
        LogPrintf("GetStakeEntropyBit: hashBlock=%s nEntropyBit=%u\n", block.GetHash().ToString().c_str(), nEntropyBit);

    return nEntropyBit;
}
