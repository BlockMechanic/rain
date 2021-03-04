// Copyright (c) 2018-2019 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_SPECIALTX_H
#define RAIN_SPECIALTX_H

#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>
#include <logging.h>
#include <messagesigner.h>
#include <wallet/wallet.h>
#include <wallet/coincontrol.h>
#include <consensus/validation.h>

class CBlock;
class CBlockIndex;

bool CheckSpecialTx(const CTransactionRef& tx, const CBlockIndex* pindexPrev, CValidationState& state);
bool ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& state, bool fJustCheck, bool fCheckCbTxMerleRoots);
bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex);

template <typename T>
inline bool GetTxPayload(const std::vector<unsigned char>& payload, T& obj)
{
    CDataStream ds(payload, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ds >> obj;
    } catch (std::exception& e) {
        return false;
    }
    return ds.empty();
}
template <typename T>
inline bool GetTxPayload(const CMutableTransaction& tx, T& obj)
{
    return GetTxPayload(tx.vExtraPayload, obj);
}
template <typename T>
inline bool GetTxPayload(const CTransaction& tx, T& obj)
{
    return GetTxPayload(tx.vExtraPayload, obj);
}

template <typename T>
void SetTxPayload(CMutableTransaction& tx, const T& payload)
{
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.vExtraPayload.assign(ds.begin(), ds.end());
}

uint256 CalcTxInputsHash(const CTransaction& tx);

template<typename SpecialTxPayload>
static void FundSpecialTx(CMutableTransaction& tx, SpecialTxPayload payload)
{
	// resize so that fee calculation is correct
	payload.signature.resize(65);

	CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
	ds << payload;
	tx.vExtraPayload.assign(ds.begin(), ds.end());

	static CTxOut dummyTxOut(CAsset(), 0, CScript() << OP_RETURN);
	bool dummyTxOutAdded = false;
	if (tx.vout.empty()) {
		// add dummy txout as FundTransaction requires at least one output
		tx.vout.push_back(dummyTxOut);
		dummyTxOutAdded = true;
	}

	CAmountMap nFee;
	CFeeRate feeRate = CFeeRate(0);
	int nChangePos = -1;
	std::string strFailReason;
	std::set<int> setSubtractFeeFromOutputs;
	    CCoinControl coinControl;
	if (!pwalletMain()->FundTransaction(tx, nFee, nChangePos, strFailReason, false, setSubtractFeeFromOutputs, coinControl)){
		LogPrintf("%s : Error  %s \n" ,__func__, strFailReason);
		return;
	}

	if (dummyTxOutAdded && tx.vout.size() > 1) {
		// FundTransaction added a change output, so we don't need the dummy txout anymore
		// Removing it results in slight overpayment of fees, but we ignore this for now (as it's a very low amount)
		std::vector<CTxOut>::iterator it = std::find(tx.vout.begin(), tx.vout.end(), dummyTxOut);
		assert(it != tx.vout.end());
		tx.vout.erase(it);
	}
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayload(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
	// payload.inputsHash = CalcTxInputsHash(tx);
	// TODO: consider adding inputs to payload

	payload.signature.clear();

	uint256 hash = ::SerializeHash(payload);
	if (!CHashSigner::SignHash(hash, key, payload.signature)) {
		LogPrintf("%s : Error  failed to sign special tx \n" ,__func__);
		return;
	}
}

template <typename SpecialTxPayload>
static bool CheckInputsHashAndSig(const CTransaction &tx, const SpecialTxPayload& payload, const CKeyID &keyId, CValidationState& state)
{
	uint256 inputsHash = CalcTxInputsHash(tx);
	// if (inputsHash != proTx.inputsHash)
	//    return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");
	// TODO: consider adding inputs to payload

	std::string strError;
	if (!CHashSigner::VerifyHash(::SerializeHash(payload), keyId, payload.signature, strError))
		state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-special-tx-sig");

	return true;
}

#endif //RAIN_SPECIALTX_H
