// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

#include <util/strencodings.h>
#include <util/system.h>

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    //CBlockIndex will be updated with information about the proof type later
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;

    //LogPrintf("GetLastBlockIndex : block %d , is %s \n", pindex->nHeight, fProofOfStake ? "pos" : "pow");

    return pindex;
}

inline arith_uint256 GetLimit(const Consensus::Params& params, bool fProofOfStake)
{
    return fProofOfStake ? UintToArith256(params.posLimit) : UintToArith256(params.powLimit);
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params, bool fProofOfStake)
{

    unsigned int nTargetLimit = GetLimit(params, fProofOfStake).GetCompact();

    // genesis block
    if (pindexLast == NULL)
        return nTargetLimit;

    // first block
    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == NULL)
        return nTargetLimit;

    // second block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL)
        return nTargetLimit;

//    LogPrintf("GetNextWorkRequired : %d  , %s, %s \n", pindexLast->nHeight, pblock->GetHash().ToString(), fProofOfStake ? "pos" : "pow");

    return CalculateNextWorkRequired(pindexPrev, pindexPrevPrev->GetBlockTime(), params, fProofOfStake);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params, bool fProofOfStake)
{
    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < 0)
        nActualTimespan = params.nPowTargetSpacing;

	// Retarget
    const arith_uint256 bnTargetLimit = GetLimit(params, fProofOfStake);
    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    int64_t nInterval = params.nPowTargetTimespan / params.nPowTargetSpacing;
    bnNew *= ((nInterval - 1) * params.nPowTargetSpacing + nActualTimespan + nActualTimespan);
    bnNew /= ((nInterval + 1) * params.nPowTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params, bool fProofOfStake)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > GetLimit(params, fProofOfStake))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
