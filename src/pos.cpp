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

int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{
    const Consensus::Params& params = Params().GetConsensus();
    // Kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low

    return std::min(nIntervalEnd - nIntervalBeginning - params.nStakeMinAge, (int64_t)params.nStakeMaxAge);
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
    //LogPrintf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString());
    return fSelected;
}


bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    const Consensus::Params& params = Params().GetConsensus();
    const CBlockIndex* pindexPrev = pindexCurrent->pprev;
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

    if(LogInstance().WillLogCategory(BCLog::COINSTAKE))
        LogPrintf("ComputeNextStakeModifier: prev modifier=%d time=%s\n", nStakeModifier, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nModifierTime));

    if (nModifierTime / params.nModifierInterval >= pindexPrev->GetBlockTime() / params.nModifierInterval)
        return true;

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
//    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
//    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

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
        //LogPrintf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n", nRound, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nSelectionIntervalStop), pindex->nHeight, pindex->GetStakeEntropyBit());

    }
    
    if(LogInstance().WillLogCategory(BCLog::COINSTAKE))
       LogPrintf( "ComputeNextStakeModifier: new modifier=%d time=%s\n", nStakeModifierNew, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pindexPrev->GetBlockTime()));

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
static bool GetKernelStakeModifier(CBlockIndex* pindexPrev, uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    // Peercoin 0.8
    const Consensus::Params& params = Params().GetConsensus();
	nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("GetKernelStakeModifier() : block not indexed");
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();

    const CBlockIndex* pindex = pindexFrom;
    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval)
    {
        if (!pindex->pnext)
        {   // reached best block; may happen if node is behind on block chain
            if (fPrintProofOfStake || (pindex->GetBlockTime() + params.nStakeMinAge - nStakeModifierSelectionInterval > GetAdjustedTime()))
                return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                    pindex->GetBlockHash().ToString().c_str(), pindex->nHeight, hashBlockFrom.ToString().c_str());
            else
                return false;
        }
        pindex = pindex->pnext;
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

bool CheckStakeKernelHash(CBlockIndex* pindexPrev, unsigned int nBits, const CBlock* blockFrom, unsigned int nTxPrevOffset, const CTransaction* txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, bool fPrintProofOfStake)
{
    const Consensus::Params& params = Params().GetConsensus();
    if (nTimeTx < txPrev->nTime)  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    unsigned int nTimeBlockFrom = blockFrom->GetBlockTime();
    if (nTimeBlockFrom + params.nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");

    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    int64_t nValueIn = txPrev->vout[prevout.n].nValue;
    
    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * GetWeight((int64_t)txPrev->nTime, (int64_t)nTimeTx) / COIN / (24 * 60 * 60);

    // We need to convert to uint512 to prevent overflow
    //base_uint<512> targetProofOfStake512(targetProofOfStake.GetHex());

    uint256 hashBlockFrom = blockFrom->GetHash();

    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64_t nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;

    if (!GetKernelStakeModifier(pindexPrev, hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake)){
        return error("CheckStakeKernelHash() : GetKernelStakeModifier failed \n");
    }

    ss << nStakeModifier;
    ss << nTimeBlockFrom << nTxPrevOffset << txPrev->nTime << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());
    if (fPrintProofOfStake)
    {
        LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%d for block from height=%d timestamp=%d \n",
            nStakeModifier, nStakeModifierHeight, nStakeModifierTime,  mapBlockIndex[hashBlockFrom]->nHeight, blockFrom->GetBlockTime());
        LogPrintf("CheckStakeKernelHash() : check modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProofOfStake=%s targetProofOfStake=%s\n",
            nStakeModifier, nTimeBlockFrom, nTxPrevOffset, txPrev->nTime, prevout.n, nTimeTx, hashProofOfStake.ToString(), (bnCoinDayWeight * bnTargetPerCoinDay).ToString());
    }

    // Now check if proof-of-stake hash meets target protocol
    //base_uint<512> hashProofOfStake512(hashProofOfStake.GetHex());

    // Now check if proof-of-stake hash meets target protocol
    LogPrintf("nHeight = %d, modifier = %d, hashProofOfStake = %s , targetProofOfStake = %s \n", pindexPrev->nHeight, nStakeModifier, hashProofOfStake.ToString(), (bnCoinDayWeight * bnTargetPerCoinDay).ToString());

    if (CBigNum(hashProofOfStake) > bnCoinDayWeight * bnTargetPerCoinDay)
        return error("CheckStakeKernelHash() :  proof-of-stake hash does not meet target at height %d \n", pindexPrev->nHeight);

    if (LogInstance().WillLogCategory(BCLog::COINSTAKE) && !fPrintProofOfStake)
    {
        LogPrintf("CheckStakeKernelHash() : using modifier=0x%016x at height=%d timestamp=%d for block from height=%d timestamp=%d \n",
            nStakeModifier, nStakeModifierHeight, nStakeModifierTime,  mapBlockIndex[hashBlockFrom]->nHeight, blockFrom->GetBlockTime());
        LogPrintf("CheckStakeKernelHash() : pass modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProofOfStake=%s targetProofOfStake=%s\n",
            nStakeModifier, nTimeBlockFrom, nTxPrevOffset, txPrev->nTime, prevout.n, nTimeTx, hashProofOfStake.ToString(), (bnCoinDayWeight * bnTargetPerCoinDay).ToString());
    }
    return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(uint32_t nTimeBlock, uint32_t nTimeTx)
{
    return (nTimeBlock == nTimeTx);
}

bool CheckBlockInputPubKeyMatchesOutputPubKey(const CBlock& block, CCoinsViewCache& view) {
    Coin coinIn;
    {
        LOCK(cs_main);
        /*if(!view.GetCoin(block.prevoutStake, coinIn)) {
            return error("%s: Could not fetch prevoutStake from UTXO set", __func__);
        }*/
    }

    CTransactionRef coinstakeTx = block.vtx[1];
    if(coinstakeTx->vout.size() < 2) {
        return error("%s: coinstake transaction does not have the minimum number of outputs", __func__);
    }

    const CTxOut& txout = coinstakeTx->vout[1];

    if(coinIn.out.scriptPubKey == txout.scriptPubKey) {
        return true;
    }

    // If the input does not exactly match the output, it MUST be on P2PKH spent and P2PK out.
    CTxDestination inputAddress;
    txnouttype inputTxType=TX_NONSTANDARD;
    if(!ExtractDestination(coinIn.out.scriptPubKey, inputAddress, &inputTxType)) {
        return error("%s: Could not extract address from input", __func__);
    }

    if(inputTxType != TX_PUBKEYHASH || inputAddress.type() != typeid(CKeyID)) {
        return error("%s: non-exact match input must be P2PKH", __func__);
    }

    CTxDestination outputAddress;
    txnouttype outputTxType=TX_NONSTANDARD;
    if(!ExtractDestination(txout.scriptPubKey, outputAddress, &outputTxType)) {
        return error("%s: Could not extract address from output", __func__);
    }

    if(outputTxType != TX_PUBKEY || outputAddress.type() != typeid(CKeyID)) {
        return error("%s: non-exact match output must be P2PK", __func__);
    }

    if(boost::get<PKHash>(&inputAddress) != boost::get<PKHash>(&outputAddress)) {
        return error("%s: input P2PKH pubkey does not match output P2PK pubkey", __func__);
    }

    return true;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(CBlockIndex* pindexPrev, CValidationState& state, const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake, CCoinsViewCache& view)
{
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString());

    // Kernel (input 0) must match the stake hash target (nBits)
    const CTxIn& txin = tx.vin[0];

    Coin coinPrev;

    if(!view.GetCoin(txin.prevout, coinPrev)){
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "Stake prevout does not exist", strprintf(" %s ", txin.prevout.hash.ToString()));
    }

    if(pindexPrev->nHeight + 1 - coinPrev.nHeight < COINBASE_MATURITY){
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "Stake prevout is not mature", strprintf("expecting %i and only matured to %i", COINBASE_MATURITY, pindexPrev->nHeight + 1 - coinPrev.nHeight));
    }

    // Read txPrev and its block
    uint256 hashBlock;
    CTransactionRef txPrevref;
    CBlock block;

    if (!GetTransaction(txin.prevout.hash, txPrevref, Params().GetConsensus(), hashBlock))
       return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "read txPrev failed", strprintf(" failed to get tx %s \n",  txin.prevout.hash.ToString()));  // previous transaction not in main chain, may occur during initial download

    if (mapBlockIndex.count(hashBlock) == 0)
       return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "CheckProofOfStake() : unable to find block ", " block does not exist"); // block not found

    if (!ReadBlockFromDisk(block, mapBlockIndex[hashBlock], Params().GetConsensus()))
       return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID,"failed to read block", "CheckProofOfStake() : INFO: read block failed \n"); // unable to read block of previous transaction

    // Verify signature
    if (!VerifySignature(coinPrev, txin.prevout.hash, tx, 0, 0))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "VerifySignature failed", strprintf(" VerifySignature failed on coinstake %s ", tx.GetHash().ToString()));

    if (!CheckStakeKernelHash(pindexPrev, nBits, &block, txin.prevout.n + CBlockHeader::NORMAL_SERIALIZE_SIZE, txPrevref.get(), txin.prevout, tx.nTime, hashProofOfStake, LogInstance().WillLogCategory(BCLog::COINSTAKE)))
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "check kernel failed on coinstake", strprintf(" check kernel failed on coinstake %s, hashProof=%s ",  tx.GetHash().ToString(), hashProofOfStake.ToString()));

    return true;
}

bool CheckKernel(CBlockIndex* pindexPrev, const CBlock* block, const COutPoint& prevout, CCoinsViewCache& view)
{
    uint256 hashProofOfStake;

    //not found in cache (shouldn't happen during staking, only during verification which does not use cache)
    Coin coinPrev;
    {
        LOCK(cs_main);
        if(!view.GetCoin(prevout, coinPrev)){
            if(!GetSpentCoinFromMainChain(pindexPrev, prevout, &coinPrev)) {
                return error("CheckKernel(): Could not find coin and it was not at the tip");
            }
        }
    }
    if(pindexPrev->nHeight + 1 - coinPrev.nHeight < COINBASE_MATURITY){
        return error("CheckKernel(): Coin not matured");
    }
    CBlockIndex* blockFrom = pindexPrev->GetAncestor(coinPrev.nHeight);
    if(!blockFrom) {
        return error("CheckKernel(): Could not find block");
    }
    if(coinPrev.IsSpent()){
        return error("CheckKernel(): Coin is spent");
    }

    CDiskTxPos postx;
    if (g_txindex) {
        g_txindex->GetTxPos(prevout.hash, postx);
    }

    uint256 hashBlock;
    CTransactionRef txPrevref;
    if (!GetTransaction(prevout.hash, txPrevref, Params().GetConsensus(), hashBlock, blockFrom))
       return error(strprintf(" failed to get tx %s \n", prevout.hash.ToString()).c_str());  // previous transaction not in main chain, may occur during initial download

    const CTransaction *txPrev = txPrevref.get();

    // Read txPrev and its block
    CBlock dblock;
    if (!ReadBlockFromDisk(dblock, blockFrom, Params().GetConsensus()))
       return error("CheckProofOfStake() : INFO: read block failed \n");  // block not found


    return CheckStakeKernelHash(pindexPrev, block->nBits, &dblock, postx.nTxOffset, txPrev, prevout, block->vtx[1]->nTime, hashProofOfStake, LogInstance().WillLogCategory(BCLog::COINSTAKE));
}
