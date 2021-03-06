// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <issuance.h>
#include <key_io.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/system.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <spentindex.h>

#include <evo/cbtx.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <llmq/quorums_commitment.h>

#include <secp256k1_rangeproof.h>

static secp256k1_context* secp256k1_blind_context = NULL;

class RPCRawTransaction_ECC_Init {
public:
    RPCRawTransaction_ECC_Init() {
        assert(secp256k1_blind_context == NULL);

        secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        assert(ctx != NULL);

        secp256k1_blind_context = ctx;
    }

    ~RPCRawTransaction_ECC_Init() {
        secp256k1_context *ctx = secp256k1_blind_context;
        secp256k1_blind_context = NULL;

        if (ctx) {
            secp256k1_context_destroy(ctx);
        }
    }
};
static RPCRawTransaction_ECC_Init ecc_init_on_load;


std::string ValueFromAmountString(const CAmount& amount, const int8_t units)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    remainder = remainder / pow(10, 8 - units);

    if (units == 0 && remainder == 0) {
        return strprintf("%s%d", sign ? "-" : "", quotient);
    }
    else {
        return strprintf("%s%d.%0" + std::to_string(units) + "d", sign ? "-" : "", quotient, remainder);
    }
}

std::string ValueFromAmountMapString(const CAmountMap& amount, const int8_t units)
{
	std::string result;
    for(auto& i : amount){
		bool sign = i.second < 0;
		int64_t n_abs = (sign ? -i.second : i.second);
		int64_t quotient = n_abs / COIN;
		int64_t remainder = n_abs % COIN;
		remainder = remainder / pow(10, 8 - units);
		result += i.first.ToString();
		result += " : ";
		if (units == 0 && remainder == 0) {
			result += strprintf("%s%d", sign ? "-" : "", quotient);
		}
		else {
			result += strprintf("%s%d.%0" + std::to_string(units) + "d", sign ? "-" : "", quotient, remainder);
		}
		result += " \n";
	}
	return result;
}

UniValue ValueFromAmountMap(const CAmountMap& amount)
{
    UniValue result(UniValue::VOBJ);
    for(auto& i : amount){
        bool sign = i.second < 0;
        int64_t n_abs = (sign ? -i.second : i.second);
        int64_t quotient = n_abs / COIN;
        int64_t remainder = n_abs % COIN;
        result.pushKV(i.first.GetHex(), strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder));
    }
    return result;
}

UniValue ValueFromAmount(const CAmount& amount)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    return UniValue(UniValue::VNUM,
            strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder));
}

UniValue ValueFromAmount(const CAmount& amount, const int8_t units)
{
    return UniValue(UniValue::VNUM, ValueFromAmountString(amount, units));
}

std::string FormatScript(const CScript& script)
{
    std::string ret;
    CScript::const_iterator it = script.begin();
    opcodetype op;
    while (it != script.end()) {
        CScript::const_iterator it2 = it;
        std::vector<unsigned char> vch;
        if (script.GetOp(it, op, vch)) {
            if (op == OP_0) {
                ret += "0 ";
                continue;
            } else if ((op >= OP_1 && op <= OP_16) || op == OP_1NEGATE) {
                ret += strprintf("%i ", op - OP_1NEGATE - 1);
                continue;
            } else if (op >= OP_NOP && op <= OP_NOP10) {
                std::string str(GetOpName(op));
                if (str.substr(0, 3) == std::string("OP_")) {
                    ret += str.substr(3, std::string::npos) + " ";
                    continue;
                }
            }
            if (vch.size() > 0) {
                ret += strprintf("0x%x 0x%x ", HexStr(it2, it - vch.size()), HexStr(it - vch.size(), it));
            } else {
                ret += strprintf("0x%x ", HexStr(it2, it));
            }
            continue;
        }
        ret += strprintf("0x%x ", HexStr(it2, script.end()));
        break;
    }
    return ret.substr(0, ret.size() - 1);
}

const std::map<unsigned char, std::string> mapSigHashTypes = {
    {static_cast<unsigned char>(SIGHASH_ALL), std::string("ALL")},
    {static_cast<unsigned char>(SIGHASH_ALL|SIGHASH_ANYONECANPAY), std::string("ALL|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_NONE), std::string("NONE")},
    {static_cast<unsigned char>(SIGHASH_NONE|SIGHASH_ANYONECANPAY), std::string("NONE|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_SINGLE), std::string("SINGLE")},
    {static_cast<unsigned char>(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY), std::string("SINGLE|ANYONECANPAY")},
};

std::string SighashToStr(unsigned char sighash_type)
{
    const auto& it = mapSigHashTypes.find(sighash_type);
    if (it == mapSigHashTypes.end()) return "";
    return it->second;
}

/**
 * Create the assembly string representation of a CScript object.
 * @param[in] script    CScript object to convert into the asm string representation.
 * @param[in] fAttemptSighashDecode    Whether to attempt to decode sighash types on data within the script that matches the format
 *                                     of a signature. Only pass true for scripts you believe could contain signatures. For example,
 *                                     pass false, or omit the this argument (defaults to false), for scriptPubKeys.
 */
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode)
{
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    CScript::const_iterator pc = script.begin();
    while (pc < script.end()) {
        if (!str.empty()) {
            str += " ";
        }
        if (!script.GetOp(pc, opcode, vch)) {
            str += "[error]";
            return str;
        }
        if (0 <= opcode && opcode <= OP_PUSHDATA4) {
            if (vch.size() <= static_cast<std::vector<unsigned char>::size_type>(4)) {
                str += strprintf("%d", CScriptNum(vch, false).getint());
            } else {
                // the IsUnspendable check makes sure not to try to decode OP_RETURN data that may match the format of a signature
                if (fAttemptSighashDecode && !script.IsUnspendable()) {
                    std::string strSigHashDecode;
                    // goal: only attempt to decode a defined sighash type from data that looks like a signature within a scriptSig.
                    // this won't decode correctly formatted public keys in Pubkey or Multisig scripts due to
                    // the restrictions on the pubkey formats (see IsCompressedOrUncompressedPubKey) being incongruous with the
                    // checks in CheckSignatureEncoding.
                    if (CheckSignatureEncoding(vch, SCRIPT_VERIFY_STRICTENC, nullptr)) {
                        const unsigned char chSigHashType = vch.back();
                        if (mapSigHashTypes.count(chSigHashType)) {
                            strSigHashDecode = "[" + mapSigHashTypes.find(chSigHashType)->second + "]";
                            vch.pop_back(); // remove the sighash type byte. it will be replaced by the decode.
                        }
                    }
                    str += HexStr(vch) + strSigHashDecode;
                } else {
                    str += HexStr(vch);
                }
            }
        } else {
            str += GetOpName(opcode);
        }
    }
    return str;
}

std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | serializeFlags);
    ssTx << tx;
    return HexStr(ssTx.begin(), ssTx.end());
}

void ScriptToUniv(const CScript& script, UniValue& out, bool include_address)
{
    out.pushKV("asm", ScriptToAsmStr(script));
    out.pushKV("hex", HexStr(script.begin(), script.end()));

    std::vector<std::vector<unsigned char>> solns;
    bool ok = false;
    txnouttype type;
    if(Solver(script, type, solns))
        ok=true;
    out.pushKV("type", GetTxnOutputType(type));

    CTxDestination address;
    if (include_address && ExtractDestination(script, address)) {
        out.pushKV("address", EncodeDestination(address));
    }
}

void ScriptPubKeyToUniv(const CScript& scriptPubKey,
                        UniValue& out, bool fIncludeHex)
{
    txnouttype type;
    std::vector<CTxDestination> addresses;
    int nRequired;

    out.pushKV("asm", ScriptToAsmStr(scriptPubKey));
    if (fIncludeHex)
        out.pushKV("hex", HexStr(scriptPubKey.begin(), scriptPubKey.end()));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired)) {
        out.pushKV("type", GetTxnOutputType(type));
        return;
    }

    out.pushKV("reqSigs", nRequired);
    out.pushKV("type", GetTxnOutputType(type));

    UniValue a(UniValue::VARR);
    for (const CTxDestination& addr : addresses) {
        a.push_back(EncodeDestination(addr));
    }
    out.pushKV("addresses", a);
}

void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, const CSpentIndexTxInfo* ptxSpentInfo, bool include_hex, int serialize_flags)
{
    uint256 txid = tx.GetHash();
    entry.pushKV("txid", txid.GetHex());
    entry.pushKV("hash", tx.GetHash().GetHex());
    entry.pushKV("withash", tx.GetWitnessOnlyHash().GetHex());
    entry.pushKV("version", tx.nVersion);
    entry.pushKV("type", tx.nType);
    entry.pushKV("time", (int64_t)tx.nTime);
    entry.pushKV("size", (int)::GetSerializeSize(tx, PROTOCOL_VERSION));
    entry.pushKV("locktime", (int64_t)tx.nLockTime);

    UniValue vin(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase())
            in.pushKV("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
        else {
            in.pushKV("txid", txin.prevout.hash.GetHex());
            in.pushKV("vout", (int64_t)txin.prevout.n);
            UniValue o(UniValue::VOBJ);
            o.pushKV("asm", ScriptToAsmStr(txin.scriptSig, true));
            o.pushKV("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
            in.pushKV("scriptSig", o);

            // Add address and value info if spentindex enabled
            if (ptxSpentInfo != nullptr) {
                CSpentIndexKey spentKey(txin.prevout.hash, txin.prevout.n);
                auto it = ptxSpentInfo->mSpentInfo.find(spentKey);
                if (it != ptxSpentInfo->mSpentInfo.end()) {
                    auto spentInfo = it->second;
                    in.pushKV("value", ValueFromAmount(spentInfo.satoshis));
                    in.pushKV("valueSat", spentInfo.satoshis);
                    if (spentInfo.addressType == 1) {
                        in.pushKV("address", EncodeDestination(PKHash(CKeyID(spentInfo.addressHash))));
                    } else if (spentInfo.addressType == 2) {
                        in.pushKV("address", EncodeDestination(ScriptHash(CScriptID(spentInfo.addressHash))));
                    }
                }
            }
        }
        in.pushKV("sequence", (int64_t)txin.nSequence);

        if (tx.witness.vtxinwit.size() > i) {
            const CScriptWitness &scriptWitness = tx.witness.vtxinwit[i].scriptWitness;
            if (!scriptWitness.IsNull()) {
                UniValue txinwitness(UniValue::VARR);
                for (const auto &item : scriptWitness.stack) {
                    txinwitness.push_back(HexStr(item.begin(), item.end()));
                }
                in.pushKV("txinwitness", txinwitness);
            }
        }

        // ELEMENTS:
        const CAssetIssuance& issuance = txin.assetIssuance;
        if (!issuance.IsNull()) {
            UniValue issue(UniValue::VOBJ);
            issue.pushKV("assetBlindingNonce", issuance.assetBlindingNonce.GetHex());
            CAsset asset;
            CAsset token;
            uint256 entropy;
            if (issuance.assetBlindingNonce.IsNull()) {
                GenerateAssetEntropy(entropy, txin.prevout, issuance.assetEntropy);
                issue.pushKV("assetEntropy", entropy.GetHex());
                CalculateAsset(asset, entropy);
                CalculateReissuanceToken(token, entropy, issuance.nAmount.IsCommitment());
                issue.pushKV("isreissuance", false);
                issue.pushKV("token", token.GetHex());
            }
            else {
                issue.pushKV("assetEntropy", issuance.assetEntropy.GetHex());
                issue.pushKV("isreissuance", true);
                CalculateAsset(asset, issuance.assetEntropy);
            }
            issue.pushKV("asset", asset.GetHex());

            if (issuance.nAmount.IsExplicit()) {
                issue.pushKV("assetamount", ValueFromAmount(issuance.nAmount.GetAmount()));
            } else if (issuance.nAmount.IsCommitment()) {
                issue.pushKV("assetamountcommitment", HexStr(issuance.nAmount.vchCommitment));
            }
            if (issuance.nInflationKeys.IsExplicit()) {
                issue.pushKV("tokenamount", ValueFromAmount(issuance.nInflationKeys.GetAmount()));
            } else if (issuance.nInflationKeys.IsCommitment()) {
                issue.pushKV("tokenamountcommitment", HexStr(issuance.nInflationKeys.vchCommitment));
            }
            in.pushKV("issuance", issue);
        }
        // END ELEMENTS

        vin.push_back(in);
    }
    entry.pushKV("vin", vin);

    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];

        UniValue out(UniValue::VOBJ);

        if (txout.nValue.IsExplicit()) {
            out.pushKV("value", ValueFromAmount(txout.nValue.GetAmount()));
        } else {
            int exp;
            int mantissa;
            uint64_t minv;
            uint64_t maxv;
            const CTxOutWitness* ptxoutwit = tx.witness.vtxoutwit.size() <= i? NULL: &tx.witness.vtxoutwit[i];
            if (ptxoutwit) {
                if (ptxoutwit->vchRangeproof.size() && secp256k1_rangeproof_info(secp256k1_blind_context, &exp, &mantissa, &minv, &maxv, &ptxoutwit->vchRangeproof[0], ptxoutwit->vchRangeproof.size())) {
                    if (exp == -1) {
                        out.pushKV("value", ValueFromAmount((CAmount)minv));
                    } else {
                        out.pushKV("value-minimum", ValueFromAmount((CAmount)minv));
                        out.pushKV("value-maximum", ValueFromAmount((CAmount)maxv));
                    }
                    out.pushKV("ct-exponent", exp);
                    out.pushKV("ct-bits", mantissa);
                }

                if (ptxoutwit->vchSurjectionproof.size()) {
                    out.pushKV("surjectionproof", HexStr(ptxoutwit->vchSurjectionproof));
                }
            }
            out.pushKV("valuecommitment", txout.nValue.GetHex());
        }
        if (txout.nAsset.IsExplicit()) {
            out.pushKV("asset", txout.nAsset.GetAsset().getName());
        } else {
            out.pushKV("assetcommitment", txout.nAsset.GetHex());
        }

        out.pushKV("commitmentnonce", txout.nNonce.GetHex());
        CPubKey pubkey(txout.nNonce.vchCommitment);
        out.pushKV("commitmentnonce_fully_valid", pubkey.IsFullyValid());
        out.pushKV("n", (int64_t)i);

        UniValue o(UniValue::VOBJ);
        ScriptPubKeyToUniv(txout.scriptPubKey, o, true);
        out.pushKV("scriptPubKey", o);

        // Add spent information if spentindex is enabled
        if (ptxSpentInfo != nullptr) {
            CSpentIndexKey spentKey(txid, i);
            auto it = ptxSpentInfo->mSpentInfo.find(spentKey);
            if (it != ptxSpentInfo->mSpentInfo.end()) {
                auto spentInfo = it->second;
                out.pushKV("spentTxId", spentInfo.txid.GetHex());
                out.pushKV("spentIndex", (int)spentInfo.inputIndex);
                out.pushKV("spentHeight", spentInfo.blockHeight);
            }
        }
        vout.push_back(out);
    }
    entry.pushKV("vout", vout);

    if (!tx.vExtraPayload.empty()) {
        entry.pushKV("extraPayloadSize", (int)tx.vExtraPayload.size());
        entry.pushKV("extraPayload", HexStr(tx.vExtraPayload));
    }

    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        CProRegTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proRegTx", obj);
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        CProUpServTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proUpServTx", obj);
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        CProUpRegTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proUpRegTx", obj);
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        CProUpRevTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proUpRevTx", obj);
        }
    } else if (tx.nType == TRANSACTION_COINBASE) {
        CCbTx cbTx;
        if (GetTxPayload(tx, cbTx)) {
            UniValue obj;
            cbTx.ToJson(obj);
            entry.pushKV("cbTx", obj);
        }
    } else if (tx.nType == TRANSACTION_QUORUM_COMMITMENT) {
        llmq::CFinalCommitmentTxPayload qcTx;
        if (GetTxPayload(tx, qcTx)) {
            UniValue obj;
            qcTx.ToJson(obj);
            entry.pushKV("qcTx", obj);
        }
    }
    //std::string srs (reinterpret_cast<const char *> (tx.strTxComment),
    //                 sizeof (tx.strTxComment) / sizeof (tx.strTxComment[0]));
//    std::string s(tx.strTxComment.begin(), tx.strTxComment.end());
//    entry.pushKV("TxComment", s);

    if (!hashBlock.IsNull())
        entry.pushKV("blockhash", hashBlock.GetHex());

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx, serialize_flags)); // The hex-encoded transaction. Used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}
