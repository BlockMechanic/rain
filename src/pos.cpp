// Copyright (c) 2012-2013 The PPCoin developers
// Copyright (c) 2014 The BlackCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>
#include <index/txindex.h>
#include <index/disktxpos.h>

#include <pos.h>
#include <key_io.h>
#include <logging.h>
#include <validation.h>

#include <chain.h>
#include <primitives/transaction.h>
#include <consensus/validation.h>
#include <txdb.h>
#include <arith_uint256.h>
#include <hash.h>
#include <timedata.h>
#include <chainparams.h>
#include <script/sign.h>
#include <consensus/consensus.h>

#include <outputtype.h>


static const int MODIFIER_INTERVAL_RATIO = 3;

// Get time weight
int64_t GetWeight(int64_t nIntervalBeginning, int64_t nIntervalEnd)
{
    // Kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low

    return std::min(nIntervalEnd - nIntervalBeginning - Params().GetConsensus().nStakeMinAge, (int64_t)Params().GetConsensus().nStakeMaxAge);
}

uint256 ComputeStakeModifier(const CBlockIndex* pindexPrev, const uint256& kernel)
{
    if (!pindexPrev)
        return uint256(); // genesis block's modifier is 0

    CDataStream ss(SER_GETHASH, 0);
    ss << kernel << pindexPrev->nStakeModifier;
    return Hash(ss);
}

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits, const CTransactionRef& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProof, bool fPrintProofOfStake)
{
    if (nTimeTx < txPrev->nTime)  // Transaction timestamp violation
        return error(" %s, nTime violation on coinstake %s", __func__, txPrev->GetHash().ToString());

    if (txPrev->nTime + Params().GetConsensus().nStakeMinAge > nTimeTx) // Min age requirement
        return error(" %s, min age violation on coinstake %s", __func__, txPrev->GetHash().ToString());

    arith_uint256 bnTarget = arith_uint256().SetCompact(nBits);
    int64_t nValueIn = txPrev->vout[prevout.n].nValue.GetAmount();
    arith_uint256 bnWeight = arith_uint256(nValueIn) * GetWeight((int64_t)txPrev->nTime, (int64_t)nTimeTx) / COIN / (24 * 60 * 60);

    CDataStream ss(SER_GETHASH, 0);
    ss << pindexPrev->nStakeModifier << txPrev->nTime << prevout.hash << prevout.n << nTimeTx;
    hashProof = Hash(ss);

    //LogPrint(BCLog::COINSTAKE, "%s : nStakeModifier=%s, txPrev.nTime=%u, txPrev.vout.hash=%s, txPrev.vout.n=%u, nTime=%u, hashProofOfStake=%s, targetProofOfStake=%s\n", __func__, pindexPrev->nStakeModifier.GetHex().c_str(),
    //        txPrev->nTime, prevout.hash.ToString(), prevout.n, nTimeTx, hashProof.ToString(), (bnWeight * bnTarget).ToString());


    // We need to convert type so it can be compared to target
    if (UintToArith256(hashProof) > bnWeight * bnTarget){
        //LogPrint(BCLog::COINSTAKE, " %s, stake hash does not meet target. \n modifier=%s \n nPrevout=%u, nTimeTx =%d , \n hashProof=%s , \n target = %s \n", __func__, pindexPrev->nStakeModifier.GetHex().c_str(),  prevout.n, nTimeTx, UintToArith256(hashProof).ToString(), (bnWeight * bnTarget).ToString());
        return false;
    }

    return true;
}

bool CheckKernel(unsigned int nBits, uint32_t nTime, const COutPoint& prevout)
{
    CBlockIndex* pindexPrev = ::ChainActive().Tip();
    uint256 hashProof;
    const Consensus::Params& params = Params().GetConsensus();

    if (!g_txindex)
        return error("%s : transaction index is not enabled", __func__);

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(prevout.hash, postx))
        return error("%s : transaction index not available for %s", __func__, prevout.hash.ToString());

    // Check that the coin is mature
    CBlockHeader header;
    CTransactionRef txPrev;
    {
        CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception &e) {
            return error("%s() : deserialize or I/O error in CheckKernel()", __func__);
        }
        if (txPrev->GetHash() != prevout.hash)
            return error("%s() : txid mismatch in CheckKernel()", __func__);
    }

    // Get the block header from the coin
    if (header.GetBlockTime() + params.nStakeMinAge > nTime)
        return error(" %s, stake prevout is not mature in block %s", __func__, header.GetHash().ToString());

    return CheckStakeKernelHash(pindexPrev, nBits, txPrev, prevout, nTime, hashProof, gArgs.GetBoolArg("-debug", false));
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(CBlockIndex* pindexPrev, const CTransactionRef& tx, unsigned int nBits, uint256& hashProof, CCoinsViewCache& view)
{
    if (!tx->IsCoinStake())
        return error("CheckProofOfStake() :  called on non-coinstake %s ", tx->vin[0].prevout.hash.ToString());

    // Kernel (input 0) must match the stake hash target (nBits)
    const CTxIn& txin = tx->vin[0];

    if (!g_txindex)
        return error("%s: transaction index is not enabled ", __func__);

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!pblocktree->ReadTxIndex(txin.prevout.hash, postx))
        return error("%s: transaction index not available ", __func__);

    // Check that the coin is mature
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

    // Verify signature
    {
        unsigned int nIn = 0;
        const CTxOut& prevOut = txPrev->vout[tx->vin[nIn].prevout.n];
        const CScriptWitness* pScriptWitness = (tx->witness.vtxinwit.size() > nIn ? &tx->witness.vtxinwit[nIn].scriptWitness : nullptr);
        TransactionSignatureChecker checker(&(*tx), nIn, prevOut.nValue, PrecomputedTransactionData(*tx));
        ScriptError serror = SCRIPT_ERR_OK;

        if (!VerifyScript(tx->vin[nIn].scriptSig, prevOut.scriptPubKey, pScriptWitness, SCRIPT_VERIFY_P2SH, checker, &serror))
            return error(" VerifyScript failed on coinstake %s %s", tx->GetHash().ToString(), ScriptErrorString(serror));
    }

    return CheckStakeKernelHash(pindexPrev, nBits, txPrev, txin.prevout, tx->nTime, hashProof, gArgs.GetBoolArg("-debug", false));
}


bool CheckRecoveredPubKeyFromBlockSignature(CBlockIndex* pindexPrev, const CBlockHeader& block, CCoinsViewCache& view) {

    if (block.IsProofOfWork())
        return true;

    Coin coinPrev;
    {
        LOCK(cs_main);
        if(!view.GetCoin(block.prevoutStake, coinPrev)){
            if(!GetSpentCoinFromMainChain(pindexPrev, block.prevoutStake, &coinPrev)) {
                return error("CheckRecoveredPubKeyFromBlockSignature(): Could not find %s and it was not at the tip", block.prevoutStake.hash.GetHex());
            }
        }
    }
    uint256 hash = block.GetHashWithoutSign();
    CPubKey pubkey;

    if(block.vchBlockSig.empty()) {
        return error("CheckRecoveredPubKeyFromBlockSignature(): Signature is empty\n");
    }

    for(uint8_t recid = 0; recid <= 3; ++recid) {
        for(uint8_t compressed = 0; compressed < 2; ++compressed) {
            if(!pubkey.RecoverLaxDER(hash, block.vchBlockSig, recid, compressed)) {
                continue;
            }
            std::vector<valtype> vSolutions;
            TxoutType whichType = Solver(coinPrev.out.scriptPubKey, vSolutions);
            CTxDestination address;
            if(ExtractDestination(coinPrev.out.scriptPubKey, address)){
                //CKeyID keyid = pubkey.GetID();

                LogPrint(BCLog::COINSTAKE, "%s type = %s , destination  = %s\n", __func__, GetTxnOutputType(whichType), EncodeDestination(GetDestinationForKey(pubkey, OutputType::BECH32)));

                if ((whichType == TxoutType::PUBKEY || whichType == TxoutType::PUBKEYHASH)) {
                    if(PKHash(pubkey) == std::get<PKHash>(address)) {
                        return true;
                    }
                    //else{
                    //    LogPrint(BCLog::COINSTAKE, " %s key mismatch  \n", EncodeDestination(address));
                    //}
                }
                else if (whichType == TxoutType::WITNESS_V0_KEYHASH){
                    if(WitnessV0KeyHash(pubkey) == std::get<WitnessV0KeyHash>(address)) {
                        return true;
                    }
                    //else{
                    //    LogPrint(BCLog::COINSTAKE, " %s key mismatch  \n", EncodeDestination(address));
                    //}
                }
                else
                    LogPrint(BCLog::COINSTAKE, " %s", "unsupported type \n");
            }
            else
                LogPrint(BCLog::COINSTAKE, " %s", "could not extract dest \n");
        }
    }
    return false;
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
    LogPrint(BCLog::COINSTAKE, "GetStakeEntropyBit: hashBlock=%s nEntropyBit=%u\n", block.GetHash().ToString().c_str(), nEntropyBit);
    return nEntropyBit;
}
