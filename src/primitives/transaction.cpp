// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <assert.h>
//#include <key_io.h>

bool ExtractCoinStakeInt64(const std::vector<uint8_t> &vData, DataOutputTypes get_type, CAmount &out)
{
    if (vData.size() < 5) { // First 4 bytes will be height
        return false;
    }
    uint64_t nv = 0;
    size_t nb = 0;
    size_t ofs = 4;
    while (ofs < vData.size()) {
        uint8_t current_type = vData[ofs];
        if (current_type == DataOutputTypes::DO_SMSG_DIFFICULTY) {
            ofs += 5;
        } else
        if (current_type == DataOutputTypes::DO_SMSG_FEE) {
            ofs++;
            if  (0 != part::GetVarInt(vData, ofs, nv, nb)) {
                return false;
            }
            if (get_type == current_type) {
                out = nv;
                return true;
            }
            ofs += nb;
        } else {
            break; // Unknown identifier byte
        }
    }
    return false;
}

bool ExtractCoinStakeUint32(const std::vector<uint8_t> &vData, DataOutputTypes get_type, uint32_t &out)
{
    if (vData.size() < 5) { // First 4 bytes will be height
        return false;
    }
    uint64_t nv = 0;
    size_t nb = 0;
    size_t ofs = 4;
    while (ofs < vData.size()) {
        uint8_t current_type = vData[ofs];
        if (current_type == DataOutputTypes::DO_SMSG_DIFFICULTY) {
            if (vData.size() < ofs+5) {
                return false;
            }
            if (get_type == current_type) {
                memcpy(&out, &vData[ofs + 1], 4);
                out = le32toh(out);
                return true;
            }
            ofs += 5;
        } else
        if (current_type == DataOutputTypes::DO_SMSG_FEE) {
            ofs++;
            if  (0 != part::GetVarInt(vData, ofs, nv, nb)) {
                return false;
            }
            ofs += nb;
        } else {
            break; // Unknown identifier byte
        }
    }
    return false;
}

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

CTxOut::CTxOut(const CConfidentialAsset& nAssetIn, const CConfidentialValue& nValueIn, CScript scriptPubKeyIn)
{
    nAsset = nAssetIn;
    nValue = nValueIn;
    scriptPubKey = scriptPubKeyIn;
    nNonce.SetNull();
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
        strAsset += strprintf("%s", nAsset.GetAsset().ToString(false));
    if (nAsset.IsCommitment())
        strAsset += std::string("CONFIDENTIAL \n");

    //CTxDestination dst;
    //std::string address ="";
    //if (ExtractDestination(scriptPubKey, dst))
    //    address = EncodeDestination(dst);

    return strprintf("CTxOut \n%s \n(nValue=%s, scriptPubKey=%s)\n", strAsset, (nValue.IsExplicit() ? strprintf("%d.%08d", nValue.GetAmount() / COIN, nValue.GetAmount() % COIN) : std::string("CONFIDENTIAL")), scriptPubKey.ToString());
}

std::string CTxData::ToString() const
{
	std::string ret ="";
	
	if(!vData.empty())
	   ret += strprintf("CTxData( Data= %s \n )", HexStr(vData));

	if(!contract.IsEmpty())
	   ret += strprintf("CTxData( Contract = %s \n )", contract.ToString());	

	if(!chainid.IsEmpty())
	   ret += strprintf("CTxData( ChainID = %s \n )", chainid.ToString());	

    return ret;
}

CMutableTransaction::CMutableTransaction() : nVersion(CTransaction::CURRENT_VERSION), nTime(GetTime()), nLockTime(0) {}
CMutableTransaction::CMutableTransaction(const CTransaction& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(tx.vin), vout(tx.vout), data(tx.data), nLockTime(tx.nLockTime), witness(tx.witness) {}

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

CTransaction::CTransaction(const CMutableTransaction& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(tx.vin), vout(tx.vout), data(tx.data), nLockTime(tx.nLockTime), witness(tx.witness), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}
CTransaction::CTransaction(CMutableTransaction&& tx) : nVersion(tx.nVersion), nTime(tx.nTime), vin(std::move(tx.vin)), vout(std::move(tx.vout)), data(std::move(tx.data)), nLockTime(tx.nLockTime), witness(std::move(tx.witness)), hash{ComputeHash()}, m_witness_hash{ComputeWitnessHash()} {}

bool CTransaction::IsCoinStake() const
{
    if (vin.empty())
        return false;

    // ppcoin: the coin stake transaction is marked with the first output empty
    if (vin[0].prevout.IsNull())
        return false;

    return (vout.size() >= 2 && vout[0].IsEmpty());
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

CAmount CTransaction::GetTotalSMSGFees() const
{
    CAmount smsg_fees = 0;
	if (!data.IsType(OUTPUT_DATA)) {
		return smsg_fees;
	}

	if (data.vData.size() < 25 || data.vData[0] != DO_FUND_MSG) {
		return smsg_fees;
	}
	size_t n = (data.vData.size()-1) / 24;
	for (size_t k = 0; k < n; ++k) {
		uint32_t nAmount;
		memcpy(&nAmount, &data.vData[1+k*24+20], 4);
		nAmount = le32toh(nAmount);
		smsg_fees += nAmount;
	}
    
    return smsg_fees;
}

unsigned int CTransaction::GetTotalSize() const
{
    return ::GetSerializeSize(*this, PROTOCOL_VERSION);
}

std::string CTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, nTime=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
        nTime,
        vin.size(),
        vout.size(),
        nLockTime);
    for (const auto& tx_in : vin)
        str +=tx_in.ToString() + "\n";
    for (const auto& tx_in : witness.vtxinwit)
        str += "    " + tx_in.scriptWitness.ToString() + "\n";
    for (const auto& tx_out : vout)
        str += "    " + tx_out.ToString() + "\n";
    str += "    " + data.ToString() + "\n";
    return str;
}

std::string CMutableTransaction::ToString() const
{
    std::string str;
    str += strprintf("CTransaction(hash=%s, ver=%d, nTime=%d, vin.size=%u, vout.size=%u, nLockTime=%u)\n",
        GetHash().ToString().substr(0,10),
        nVersion,
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
    str += "    " + data.ToString() + "\n";

    return str;
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

//bool SignID(CChainID& id, CKey& key){
//	return key.Sign(id.GetHashWithoutSign(), id.vchIDSignature);
//}
