// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <streams.h>
#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <key_io.h>

std::string COutPoint::ToString() const
{
    return strprintf("COutPoint(%s, %u)\n", hash.ToString(), n);
}

std::string COutPoint::ToStringShort() const
{
    return strprintf("%s-%u", hash.ToString().substr(0,10), n);
}

uint256 COutPoint::GetHash()
{
    return SerializeHash(*this);
}

CTxIn::CTxIn(COutPoint prevoutIn, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = prevoutIn;
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

CTxIn::CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn, uint32_t nSequenceIn)
{
    prevout = COutPoint(hashPrevTx, nOut);
    scriptSig = scriptSigIn;
    nSequence = nSequenceIn;
}

std::string CTxIn::ToString() const
{
    std::string str;
    str += "CTxIn(";
    str += prevout.ToString();
    if (prevout.IsNull())
        str += strprintf("coinbase=%s \n", HexStr(scriptSig));
    else
        str += strprintf("scriptSig=%s \n", HexStr(scriptSig));
    if (nSequence != SEQUENCE_FINAL)
        str += strprintf("nSequence=%u \n", nSequence);
    if (!assetIssuance.IsNull())
        str += strprintf("%s", assetIssuance.ToString());

    return str;
}

CTxOut::CTxOut(const CConfidentialAsset& nAssetIn, const CConfidentialValue& nValueIn, CScript scriptPubKeyIn, int nRoundsIn)
{
    nAsset = nAssetIn;
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
    nNonce.SetNull();
    nRounds = nRoundsIn;
}

uint256 CTxOut::GetHash() const
{
    return SerializeHash(*this);
}

std::string CTxOut::ToString() const
{
    std::string strAsset;
    if (IsEmpty()) return "CTxOut(empty)";

    if (nAsset.IsExplicit())
        strAsset += strprintf("%s", nAsset.GetAsset().ToString());
    if (nAsset.IsCommitment())
        strAsset += std::string("CONFIDENTIAL \n");

    CTxDestination dst;
    std::string address ="";
    if (ExtractDestination(scriptPubKey, dst))
        address = EncodeDestination(dst);

    return strprintf("CTxOut \n Asset = %s\n(nValue=%s, scriptPubKey=%s, Address=%s)\n", strAsset, (nValue.IsExplicit() ? strprintf("%d.%08d", nValue.GetAmount() / COIN, nValue.GetAmount() % COIN) : std::string("CONFIDENTIAL")), scriptPubKey.ToString(), address);
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nTime(GetTime()), nLockTime(0), nType(TRANSACTION_NORMAL) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(tx.vin), vout(tx.vout), nLockTime(tx.nLockTime), nType(tx.nType), vExtraPayload(tx.vExtraPayload), witness(tx.witness) {}

uint256 CMutableTransaction::GetHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeHash() const
{
    return SerializeHash(*this, SER_GETHASH, SERIALIZE_TRANSACTION_NO_WITNESS);
}

uint256 CTransaction::ComputeWitnessHash() const
{
    if (!HasWitness()) {
        return hash;
    }
    return SerializeHash(*this, SER_GETHASH, 0);
}

uint256 CTransaction::GetWitnessOnlyHash() const
{
    std::vector<uint256> leaves;
    leaves.reserve(std::max(vin.size(), vout.size()));
    /* Inputs */
    for (size_t i = 0; i < vin.size(); ++i) {
        // Input has no witness OR is null input(coinbase)
        const CTxInWitness& txinwit = (witness.vtxinwit.size() <= i || vin[i].prevout.IsNull()) ? CTxInWitness() : witness.vtxinwit[i];
        leaves.push_back(txinwit.GetHash());
    }
    uint256 hashIn = ComputeFastMerkleRoot(leaves);
    leaves.clear();
    /* Outputs */
    for (size_t i = 0; i < vout.size(); ++i) {
        const CTxOutWitness& txoutwit = witness.vtxoutwit.size() <= i ? CTxOutWitness() : witness.vtxoutwit[i];
        leaves.push_back(txoutwit.GetHash());
    }
    uint256 hashOut = ComputeFastMerkleRoot(leaves);
    leaves.clear();
    /* Combined */
    leaves.push_back(hashIn);
    leaves.push_back(hashOut);
    return ComputeFastMerkleRoot(leaves);
}

/* For backward compatibility, the hash is initialized to 0. TODO: remove the need for this default constructor entirely. */
CTransaction::CTransaction() : nVersion(CTransaction::CURRENT_VERSION), nTime(0), vin(), vout(), nLockTime(0), nType(TRANSACTION_NORMAL), hash{}, m_witness_hash{} {}
CTransaction::CTransaction(const CMutableTransaction& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(tx.vin), vout(tx.vout), nLockTime(tx.nLockTime), nType(tx.nType), vExtraPayload(tx.vExtraPayload), witness(tx.witness), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction&& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(std::move(tx.vin)), vout(std::move(tx.vout)), nLockTime(tx.nLockTime), nType(tx.nType), vExtraPayload(tx.vExtraPayload), witness(std::move(tx.witness)), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

bool CTransaction::IsCoinStake() const
{
    if (vin.empty())
        return false;

    // ppcoin: the coin stake transaction is marked with the first output empty
    if (vin[0].prevout.IsNull())
        return false;

    return (vout.size() >= 2 && vout[0].IsEmpty());
}

bool CTransaction::CheckColdStake(const CScript& script) const
{

    // tx is a coinstake tx
    if (!IsCoinStake())
        return false;

    // all inputs have the same scriptSig
    CScript firstScript = vin[0].scriptSig;
    if (vin.size() > 1) {
        for (unsigned int i=1; i<vin.size(); i++)
            if (vin[i].scriptSig != firstScript) return false;
    }

    // all outputs except first (coinstake marker) and last (masternode payout)
    // have the same pubKeyScript and it matches the script we are spending
    if (vout[1].scriptPubKey != script) return false;
    if (vin.size() > 3) {
        for (unsigned int i=2; i<vout.size()-1; i++)
            if (vout[i].scriptPubKey != script) return false;
    }

    return true;
}

bool CTransaction::HasP2CSOutputs() const
{
    for(const CTxOut& txout : vout) {
        if (txout.scriptPubKey.IsPayToColdStaking())
            return true;
    }
    return false;
}

CAmountMap CTransaction::GetValueOutMap() const {

    CAmountMap values;
    for (const auto& tx_out : vout) {
        if (tx_out.nValue.IsExplicit() && tx_out.nAsset.IsExplicit()) {
            CAmountMap m;
            m[tx_out.nAsset.GetAsset()] = tx_out.nValue.GetAmount();
            values += m;
            if (!MoneyRange(tx_out.nValue.GetAmount()) || !MoneyRange(values[tx_out.nAsset.GetAsset()]))
                throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }
    return values;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, PROTOCOL_VERSION);
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, type=%d, nTime=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString(),
        nVersion,
        nType,
        nTime,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str +=tx_in.ToString() + "\n";
    for (const auto& tx_in : witness.vtxinwit)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str +=tx_out.ToString() + "\n";
//    std::string s(strTxComment.begin(), strTxComment.end());
//    str += "strTxComment: " + s + "\n";
    return str;
}

std::string CMutableTransaction::ToString() const
{
    std::string str;
    str += strprintf("CMutableTransaction(hash=%s, ver=%d, type=%d,  nTime=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        nType,
        nTime,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str += tx_in.ToString() + "\n";
    for (const auto& tx_in : witness.vtxinwit)
        str += tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += tx_out.ToString() + "\n";
//    std::string s(strTxComment.begin(), strTxComment.end());
//    str += "strTxComment: " + s + "\n";
    return str;
}

bool CMutableTransaction::CheckColdStake(const CScript& script) const
{

    // tx is a coinstake tx
    if (!IsCoinStake())
        return false;

    // all inputs have the same scriptSig
    CScript firstScript = vin[0].scriptSig;
    if (vin.size() > 1) {
        for (unsigned int i=1; i<vin.size(); i++)
            if (vin[i].scriptSig != firstScript) return false;
    }

    // all outputs except first (coinstake marker) and last (masternode payout)
    // have the same pubKeyScript and it matches the script we are spending
    if (vout[1].scriptPubKey != script) return false;
    if (vin.size() > 3) {
        for (unsigned int i=2; i<vout.size()-1; i++)
            if (vout[i].scriptPubKey != script) return false;
    }

    return true;
}

bool CMutableTransaction::IsCoinStake() const
{
    if (vin.empty())
        return false;

    // ppcoin: the coin stake transaction is marked with the first output empty
    if (vin[0].prevout.IsNull())
        return false;

    return (vout.size() >= 2 && vout[0].IsEmpty());
}
