// Copyright (c) 2018-2019 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/deterministicmns.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>

#include <base58.h>
#include <chainparams.h>
#include <clientversion.h>
#include <core_io.h>
#include <hash.h>
#include <messagesigner.h>
#include <script/standard.h>
#include <streams.h>
#include <univalue.h>
#include <validation.h>

template <typename ProTx>
static bool CheckService(const uint256& proTxHash, const ProTx& proTx, CValidationState& state)
{
    if (!proTx.addr.IsValid()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-ipaddr");
    }
    if (Params().NetworkIDString() != CBaseChainParams::REGTEST && !proTx.addr.IsRoutable()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-ipaddr");
    }

    static int mainnetDefaultPort = CreateChainParams(CBaseChainParams::MAIN)->GetDefaultPort();
    if (Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if (proTx.addr.GetPort() != mainnetDefaultPort) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, strprintf("bad-protx-ipaddr-port %d vs %d",proTx.addr.GetPort(), mainnetDefaultPort));
        }
    } else if (proTx.addr.GetPort() == mainnetDefaultPort) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-ipaddr-port");
    }

    if (!proTx.addr.IsIPv4()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-ipaddr");
    }

    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CHashSigner::VerifyHash(::SerializeHash(proTx), keyID, proTx.vchSig, strError)) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-sig", strError);
    }
    return true;
}

template <typename ProTx>
static bool CheckStringSig(const ProTx& proTx, const CKeyID& keyID, CValidationState& state)
{
    std::string strError;
    if (!CMessageSigner::VerifyMessage(keyID, proTx.vchSig, proTx.MakeSignString(), strError)) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-sig", strError);
    }
    return true;
}

template <typename ProTx>
static bool CheckHashSig(const ProTx& proTx, const CBLSPublicKey& pubKey, CValidationState& state)
{
    if (!proTx.sig.VerifyInsecure(pubKey, ::SerializeHash(proTx))) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-sig");
    }
    return true;
}

template <typename ProTx>
static bool CheckInputsHash(const CTransaction& tx, const ProTx& proTx, CValidationState& state)
{
    uint256 inputsHash = CalcTxInputsHash(tx);
    if (inputsHash != proTx.inputsHash) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-inputs-hash");
    }

    return true;
}

bool CheckProRegTx(const CTransactionRef& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx->nType != TRANSACTION_PROVIDER_REGISTER) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-type");
    }

    CProRegTx ptx;
    if (!GetTxPayload(CTransaction(*tx), ptx)) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-version");
    }
    if (ptx.nType != 0) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-type");
    }
    if (ptx.nMode != 0) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-mode");
    }

    if (ptx.keyIDOwner.IsNull() || !ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-key-null");
    }
    if (!ptx.scriptPayout.IsPayToPubkeyHash() && !ptx.scriptPayout.IsPayToScriptHash()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payee");
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payee-dest");
    }
    // don't allow reuse of payout key for other keys (don't allow people to put the payee key onto an online server)
    if (payoutDest == DecodeDestination(EncodeDestination(PKHash(ptx.keyIDOwner))) || payoutDest == DecodeDestination(EncodeDestination(PKHash(ptx.keyIDVoting)))) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payee-reuse");
    }

    // It's allowed to set addr to 0, which will put the MN into PoSe-banned state and require a ProUpServTx to be issues later
    // If any of both is set, it must be valid however
    if (ptx.addr != CService() && !CheckService(tx->GetHash(), ptx, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (ptx.nOperatorReward > 10000) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-operator-reward");
    }

    CTxDestination collateralTxDest;
    CKeyID keyForPayloadSig;
    COutPoint collateralOutpoint;

    if (!ptx.collateralOutpoint.hash.IsNull()) {
        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin) || coin.out.nValue.GetAmount() != Params().MasternodeCollateral() || coin.out.nAsset.GetAsset() != Params().GetConsensus().masternode_asset) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral");
        }

        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }

        // Extract key from collateral. This only works for P2PK and P2PKH collaterals and will fail for P2SH.
        // Issuer of this ProRegTx must prove ownership with this key by signing the ProRegTx
		keyForPayloadSig = CKeyID(*boost::get<PKHash>(&collateralTxDest));

        if (keyForPayloadSig.IsNull()) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-pkh");
        }

        collateralOutpoint = ptx.collateralOutpoint;
    } else {
        if (ptx.collateralOutpoint.n >= tx->vout.size()) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-index");
        }
        if (tx->vout[ptx.collateralOutpoint.n].nValue.GetAmount() != Params().MasternodeCollateral()) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral");
        }
        if (tx->vout[ptx.collateralOutpoint.n].nAsset.GetAsset() != Params().GetConsensus().masternode_asset){
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-asset");
        }
        if (!ExtractDestination(tx->vout[ptx.collateralOutpoint.n].scriptPubKey, collateralTxDest)) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }

        collateralOutpoint = COutPoint(tx->GetHash(), ptx.collateralOutpoint.n);
    }

    // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
    // this check applies to internal and external collateral, but internal collaterals are not necessarely a P2PKH
    if (collateralTxDest == DecodeDestination(EncodeDestination(PKHash(ptx.keyIDOwner))) || collateralTxDest == DecodeDestination(EncodeDestination(PKHash(ptx.keyIDVoting)))) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-reuse");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);

        // only allow reusing of addresses when it's for the same collateral (which replaces the old MN)
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->collateralOutpoint != collateralOutpoint) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_DUPLICATE, "bad-protx-dup-addr");
        }

        // never allow duplicate keys, even if this ProTx would replace an existing MN
        if (mnList.HasUniqueProperty(ptx.keyIDOwner) || mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_DUPLICATE, "bad-protx-dup-key");
        }

        if (!deterministicMNManager->IsDIP3Enforced(pindexPrev->nHeight)) {
            if (ptx.keyIDOwner != ptx.keyIDVoting) {
                return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-key-not-same");
            }
        }
    }

    if (!CheckInputsHash(CTransaction(*tx), ptx, state)) {
        return false;
    }

    if (!keyForPayloadSig.IsNull()) {
        // collateral is not part of this ProRegTx, so we must verify ownership of the collateral
        if (!CheckStringSig(ptx, keyForPayloadSig, state)) {
            return false;
        }
    } else {
        // collateral is part of this ProRegTx, so we know the collateral is owned by the issuer
        if (!ptx.vchSig.empty()) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-sig");
        }
    }

    return true;
}

bool CheckProUpServTx(const CTransactionRef& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx->nType != TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpServTx ptx;
    if (!GetTxPayload(CTransaction(*tx), ptx)) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-version");
    }

    if (!CheckService(ptx.proTxHash, ptx, state)) {
        // pass the state returned by the function above
        return false;
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto mn = mnList.GetMN(ptx.proTxHash);
        if (!mn) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-hash");
        }

        // don't allow updating to addresses already used by other MNs
        if (mnList.HasUniqueProperty(ptx.addr) && mnList.GetUniquePropertyMN(ptx.addr)->proTxHash != ptx.proTxHash) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_DUPLICATE, "bad-protx-dup-addr");
        }

        if (ptx.scriptOperatorPayout != CScript()) {
            if (mn->nOperatorReward == 0) {
                // don't allow to set operator reward payee in case no operatorReward was set
                return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-operator-payee");
            }
            if (!ptx.scriptOperatorPayout.IsPayToPubkeyHash() && !ptx.scriptOperatorPayout.IsPayToScriptHash()) {
                return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-operator-payee");
            }
        }

        // we can only check the signature if pindexPrev != nullptr and the MN is known
        if (!CheckInputsHash(CTransaction(*tx), ptx, state)) {
            // pass the state returned by the function above
            return false;
        }
        if (!CheckHashSig(ptx, mn->pdmnState->pubKeyOperator.Get(), state)) {
            // pass the state returned by the function above
            return false;
        }
    }

    return true;
}

bool CheckProUpRegTx(const CTransactionRef& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx->nType != TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpRegTx ptx;
    if (!GetTxPayload(CTransaction(*tx), ptx)) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-version");
    }
    if (ptx.nMode != 0) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-mode");
    }

    if (!ptx.pubKeyOperator.IsValid() || ptx.keyIDVoting.IsNull()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-key-null");
    }
    if (!ptx.scriptPayout.IsPayToPubkeyHash() && !ptx.scriptPayout.IsPayToScriptHash()) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payee");
    }

    CTxDestination payoutDest;
    if (!ExtractDestination(ptx.scriptPayout, payoutDest)) {
        // should not happen as we checked script types before
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payee-dest");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-hash");
        }

        // don't allow reuse of payee key for other keys (don't allow people to put the payee key onto an online server)
        if (payoutDest == DecodeDestination(EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner))) || payoutDest == DecodeDestination(EncodeDestination(PKHash(ptx.keyIDVoting)))) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payee-reuse");
        }

        Coin coin;
        if (!GetUTXOCoin(dmn->collateralOutpoint, coin)) {
            // this should never happen (there would be no dmn otherwise)
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral");
        }

        // don't allow reuse of collateral key for other keys (don't allow people to put the collateral key onto an online server)
        CTxDestination collateralTxDest;
        if (!ExtractDestination(coin.out.scriptPubKey, collateralTxDest)) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-dest");
        }        
        if (collateralTxDest == DecodeDestination(EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner))) || collateralTxDest == DecodeDestination(EncodeDestination(PKHash(ptx.keyIDVoting)))) {
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-collateral-reuse");
        }

        if (mnList.HasUniqueProperty(ptx.pubKeyOperator)) {
            auto otherDmn = mnList.GetUniquePropertyMN(ptx.pubKeyOperator);
            if (ptx.proTxHash != otherDmn->proTxHash) {
                return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_DUPLICATE, "bad-protx-dup-key");
            }
        }

        if (!deterministicMNManager->IsDIP3Enforced(pindexPrev->nHeight)) {
            if (dmn->pdmnState->keyIDOwner != ptx.keyIDVoting) {
                return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-key-not-same");
            }
        }

        if (!CheckInputsHash(CTransaction(*tx), ptx, state)) {
            // pass the state returned by the function above
            return false;
        }
        if (!CheckHashSig(ptx, dmn->pdmnState->keyIDOwner, state)) {
            // pass the state returned by the function above
            return false;
        }
    }

    return true;
}

bool CheckProUpRevTx(const CTransactionRef& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx->nType != TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-type");
    }

    CProUpRevTx ptx;
    if (!GetTxPayload(CTransaction(*tx), ptx)) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-payload");
    }

    if (ptx.nVersion == 0 || ptx.nVersion > CProRegTx::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-version");
    }

    // ptx.nReason < CProUpRevTx::REASON_NOT_SPECIFIED is always `false` since
    // ptx.nReason is unsigned and CProUpRevTx::REASON_NOT_SPECIFIED == 0
    if (ptx.nReason > CProUpRevTx::REASON_LAST) {
        return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-reason");
    }

    if (pindexPrev) {
        auto mnList = deterministicMNManager->GetListForBlock(pindexPrev);
        auto dmn = mnList.GetMN(ptx.proTxHash);
        if (!dmn)
            return state.Invalid(ValidationInvalidReason::BADPROTX, false, REJECT_INVALID, "bad-protx-hash");

        if (!CheckInputsHash(CTransaction(*tx), ptx, state)) {
            // pass the state returned by the function above
            return false;
        }
        if (!CheckHashSig(ptx, dmn->pdmnState->pubKeyOperator.Get(), state)) {
            // pass the state returned by the function above
            return false;
        }
    }

    return true;
}

std::string CProRegTx::MakeSignString() const
{
    std::string s;

    // We only include the important stuff in the string form...

    CTxDestination destPayout;
    std::string strPayout;
    if (ExtractDestination(scriptPayout, destPayout)) {
        strPayout = EncodeDestination(destPayout);
    } else {
        strPayout = HexStr(scriptPayout.begin(), scriptPayout.end());
    }

    s += strPayout + "|";
    s += strprintf("%d", nOperatorReward) + "|";
    s += EncodeDestination(PKHash(keyIDOwner)) + "|";
    s += EncodeDestination(PKHash(keyIDVoting)) + "|";

    // ... and also the full hash of the payload as a protection agains malleability and replays
    s += ::SerializeHash(*this).ToString();

    return s;
}

std::string CProRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProRegTx(nVersion=%d, collateralOutpoint=%s, addr=%s, nOperatorReward=%f, ownerAddress=%s, pubKeyOperator=%s, votingAddress=%s, scriptPayout=%s)",
        nVersion, collateralOutpoint.ToStringShort(), addr.ToString(), (double)nOperatorReward / 100, EncodeDestination(PKHash(keyIDOwner)), pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), payee);
}

std::string CProUpServTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProUpServTx(nVersion=%d, proTxHash=%s, addr=%s, operatorPayoutAddress=%s)",
        nVersion, proTxHash.ToString(), addr.ToString(), payee);
}

std::string CProUpRegTx::ToString() const
{
    CTxDestination dest;
    std::string payee = "unknown";
    if (ExtractDestination(scriptPayout, dest)) {
        payee = EncodeDestination(dest);
    }

    return strprintf("CProUpRegTx(nVersion=%d, proTxHash=%s, pubKeyOperator=%s, votingAddress=%s, payoutAddress=%s)",
        nVersion, proTxHash.ToString(), pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), payee);
}

std::string CProUpRevTx::ToString() const
{
    return strprintf("CProUpRevTx(nVersion=%d, proTxHash=%s, nReason=%d)",
        nVersion, proTxHash.ToString(), nReason);
}
