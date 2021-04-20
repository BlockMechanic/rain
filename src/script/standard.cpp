// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>
#include <hash.h>
#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>

#include <string>
#include <logging.h>
typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : BaseHash(Hash160(in)) {}
CScriptID::CScriptID(const ScriptHash& in) : BaseHash(static_cast<uint160>(in)) {}

ScriptHash::ScriptHash(const CScript& in) : BaseHash(Hash160(in)) {}
ScriptHash::ScriptHash(const CScriptID& in) : BaseHash(static_cast<uint160>(in)) {}

ScriptHash::ScriptHash(const CScript& in, const CPubKey& blinding_pubkey_in) : BaseHash(Hash160(in)), blinding_pubkey(blinding_pubkey_in) {}
ScriptHash::ScriptHash(const uint160& hash, const CPubKey& blinding_pubkey_in) : BaseHash(static_cast<uint160>(hash)), blinding_pubkey(blinding_pubkey_in) {}

PKHash::PKHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
PKHash::PKHash(const CKeyID& pubkey_id) : BaseHash(pubkey_id) {}

PKHash::PKHash(const CPubKey& pubkey, const CPubKey& blinding_pubkey_in) : BaseHash(pubkey.GetID()), blinding_pubkey(blinding_pubkey_in) {}
PKHash::PKHash(const uint160& hash, const CPubKey& blinding_pubkey_in) : BaseHash(static_cast<uint160>(hash)), blinding_pubkey(blinding_pubkey_in) {}

WitnessV0KeyHash::WitnessV0KeyHash(const CPubKey& pubkey) : BaseHash(pubkey.GetID()) {}
WitnessV0KeyHash::WitnessV0KeyHash(const PKHash& pubkey_hash) : BaseHash(static_cast<uint160>(pubkey_hash)) {}


CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}

CKeyID ToKeyID(const WitnessV0KeyHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

std::string GetTxnOutputType(TxoutType t)
{
    switch (t) {
    case TxoutType::NONSTANDARD: return "nonstandard";
    case TxoutType::PUBKEY: return "pubkey";
    case TxoutType::PUBKEYHASH: return "pubkeyhash";
    case TxoutType::SCRIPTHASH: return "scripthash";
    case TxoutType::MULTISIG: return "multisig";
    case TxoutType::NULL_DATA: return "nulldata";
    case TxoutType::WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TxoutType::WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TxoutType::WITNESS_V1_TAPROOT: return "witness_v1_taproot";
    case TxoutType::WITNESS_UNKNOWN: return "witness_unknown";
    case TxoutType::FEE: return "fee";
    case TxoutType::HTLC: return "htlc";
    case TxoutType::CLTV: return "cltv";  // CLTV HODL Freeze
    case TxoutType::MULTISIG_CLTV: return "multisig_cltv";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::SIZE + 2 && script[0] == CPubKey::SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_SIZE + 2 && script[0] == CPubKey::COMPRESSED_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}

static bool MatchPayToPubkeyHash(const CScript& script, valtype& pubkeyhash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20 && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash = valtype(script.begin () + 3, script.begin() + 23);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}

static bool MatchMultisig(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;
    return (it + 1 == script.end());
}

static bool MatchFreezeCLTV(const CScript &script, std::vector<valtype> &pubkeys)
{
    // Freeze tx using CLTV ; nFreezeLockTime CLTV DROP (0x21 pubkeys) checksig
    // {TX_CLTV, CScript() << OP_BIGINTEGER << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_PUBKEYS << OP_CHECKSIG},

    if ((script.size() < 1) || script.back() != OP_CHECKSIG)
    {
        return false;
    }

    valtype data;
    opcodetype opcode;
    CScript::const_iterator s = script.begin();
    script.GetOp(s, opcode, data);

    try
    {
        // extracting the bignum in a try-catch because if the provided number is not
        // a big int CScriptNum will raise an error.
        CScriptNum nLockFreezeTime(data, true, 5);

        pubkeys.emplace_back(data);
        if ((*s != OP_CHECKLOCKTIMEVERIFY) || (*(s + 1) != OP_DROP))
        {
            return false;
        }

        // starting from pubkeys (4/5 byte nlock time + 1 OP_CLTV + 1 OP_DROP)
        s = s + 2;
        if (!script.GetOp(s, opcode, data))
        {
            return false;
        }
        if (!CPubKey::ValidSize(data))
        {
            return false;
        }
        pubkeys.emplace_back(std::move(data));
        // after key extraction we should still have one byte which represent OP_CHECKSIG
        return (s + 1 == script.end());
    }
    catch (scriptnum_error &)
    {
        return false;
    }
}

static bool MatchTemplates(const CScript& scriptPubKey, TxoutType &typeRet, std::vector<std::vector<unsigned char>>& vSolutionsRet){ //this is a combo for htlc and cltv
    // Templates
    static std::multimap<TxoutType, CScript> mTemplates;
    if (mTemplates.empty())
    {
        mTemplates.insert(std::make_pair(TxoutType::MULTISIG_CLTV, CScript() << OP_U32INT << OP_CHECKLOCKTIMEVERIFY << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));

        // HTLC where sender requests preimage of a hash
        {
            const opcodetype accepted_hashers[] = {OP_SHA256, OP_RIPEMD160};
            const opcodetype accepted_timeout_ops[] = {OP_CHECKLOCKTIMEVERIFY, OP_CHECKSEQUENCEVERIFY};

            for(auto hasher: accepted_hashers) {
                for(auto timeout_op: accepted_timeout_ops) {
                    mTemplates.insert(std::make_pair(TxoutType::HTLC, CScript() << hasher << OP_ANYDATA << OP_EQUAL << OP_IF
                        << OP_PUBKEY << OP_ELSE << OP_ANYDATA << timeout_op << OP_DROP << OP_PUBKEY << OP_ENDIF << OP_CHECKSIG
                    ));
                }
            }
        }

    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    for(const auto& tplate: mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        std::vector<unsigned char> vch1, vch2;

        // Compare
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TxoutType::MULTISIG || typeRet == TxoutType::MULTISIG_CLTV)
                {
                    // Additional checks for TxoutType::MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }
                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_ANYDATA)
            {
                if (vch1.empty()
                    && opcode1 != OP_0 && opcode1 < OP_1 && opcode1 > OP_16)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(valtype(1, n));
                }
                else
                    break;
            }
            else if (opcode2 == OP_BIGINTEGER)
            {
                try {
                    CScriptNum n(vch1, true, 5);

                    LogPrintf("Freeze Solver BIGINT=%d \n", n.GetInt64());
                    // if try reaches here without scriptnum_error
                    // then vch1 is a valid bigint
                    vSolutionsRet.push_back(vch1);
                } catch (scriptnum_error&) {
//                  LogPrintf("Freeze Solver BIGINT ERROR! %s \n", ::ScriptToAsmStr(CScript(vch1)));
                    break;
                } // end try/catch
            }
            else if (opcode2 == OP_U32INT)
            {
                CScriptNum sn(0);
                try {
                    sn = CScriptNum(vch1, true, 5);
                } catch (scriptnum_error) {
                    break;
                }
                // 0 CLTV is pointless, so expect at least height 1
                if (sn < 1 || sn > std::numeric_limits<uint32_t>::max()) {
                    break;
                }
            }
            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }


}

TxoutType Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet)
{
    vSolutionsRet.clear();

    // Fee outputs are for elements-style transactions only
    if (scriptPubKey == CScript()) {
        return TxoutType::FEE;
    }

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return TxoutType::SCRIPTHASH;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TxoutType::WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TxoutType::WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_V1_TAPROOT;
        }
        if (witnessversion != 0) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TxoutType::WITNESS_UNKNOWN;
        }
        return TxoutType::NONSTANDARD;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        return TxoutType::NULL_DATA;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEY;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TxoutType::PUBKEYHASH;
    }
    std::vector<std::vector<unsigned char>> keys;

    if (MatchFreezeCLTV(scriptPubKey, keys))
    {
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        return TxoutType::CLTV;
    }

    unsigned int required;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        return TxoutType::MULTISIG;
    }

    vSolutionsRet.clear();
    return TxoutType::NONSTANDARD;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    TxoutType whichType = Solver(scriptPubKey, vSolutions);

    switch (whichType) {
    case TxoutType::PUBKEY: {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    case TxoutType::PUBKEYHASH: {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::SCRIPTHASH: {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    }
    case TxoutType::WITNESS_V0_KEYHASH: {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_V0_SCRIPTHASH: {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    }
    case TxoutType::WITNESS_UNKNOWN:
    case TxoutType::WITNESS_V1_TAPROOT: {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    }
    case TxoutType::CLTV:
    {
        CPubKey pubKey(vSolutions[1]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    case TxoutType::MULTISIG:
    case TxoutType::MULTISIG_CLTV:
    case TxoutType::NULL_DATA:
    case TxoutType::NONSTANDARD:
    case TxoutType::FEE:
        return false;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

bool ExtractDestinations(const CScript& scriptPubKey, TxoutType& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    std::vector<valtype> vSolutions;
    typeRet = Solver(scriptPubKey, vSolutions);
    if (typeRet == TxoutType::NONSTANDARD) {
        return false;
    } else if (typeRet == TxoutType::NULL_DATA) {
        // This is data, not addresses
        return false;
    }

    if (typeRet == TxoutType::MULTISIG || typeRet == TxoutType::MULTISIG_CLTV)
    {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size()-1; i++)
        {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = PKHash(pubKey);
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;
    }
    else if (typeRet == TxoutType::HTLC)
    {
        // Seller
        {
            CPubKey pubKey(vSolutions[1]);
            if (pubKey.IsValid()) {
                CTxDestination address = PKHash(pubKey);
                addressRet.push_back(address);
            }
        }
        // Refund
        {
            CPubKey pubKey(vSolutions[3]);
            if (pubKey.IsValid()) {
                CTxDestination address = PKHash(pubKey);
                addressRet.push_back(address);
            }
        }
    }
    else
    {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
           return false;
        addressRet.push_back(address);
    }

    return true;
}

namespace {
class CScriptVisitor
{
public:
    CScript operator()(const CNoDestination& dest) const
    {
        return CScript();
    }

    CScript operator()(const PKHash& keyID) const
    {
        return CScript() << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    CScript operator()(const ScriptHash& scriptID) const
    {
        return CScript() << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
    }

    CScript operator()(const WitnessV0KeyHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessV0ScriptHash& id) const
    {
        return CScript() << OP_0 << ToByteVector(id);
    }

    CScript operator()(const WitnessUnknown& id) const
    {
        return CScript() << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
    }

   CScript operator()(const NullData& id) const
    {
        CScript mret;
        mret << OP_RETURN;
        for (const auto& push : id.null_data) {
            mret << push;
        }
        return mret;
    }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    return std::visit(CScriptVisitor(), dest);
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys, const int64_t cltv_height, const int64_t cltv_time)
{
    CScript script;

    if (cltv_height > 0) {
        if (cltv_time) {
            throw std::invalid_argument("cannot lock for both height and time");
        }
        if (cltv_height >= LOCKTIME_THRESHOLD) {
            throw std::invalid_argument("requested lock height is beyond locktime threshold");
        }
        script << cltv_height << OP_CHECKLOCKTIMEVERIFY;
    } else if (cltv_time) {
        if (cltv_time < LOCKTIME_THRESHOLD || cltv_time > std::numeric_limits<uint32_t>::max()) {
            throw std::invalid_argument("requested lock time is outside of valid range");
        }
        script << cltv_time << OP_CHECKLOCKTIMEVERIFY;
    }

    script << CScript::EncodeOP_N(nRequired);
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

CScript GetScriptForWitness(const CScript& redeemscript)
{
    std::vector<std::vector<unsigned char> > vSolutions;
    TxoutType typ = Solver(redeemscript, vSolutions);
    if (typ == TxoutType::PUBKEY) {
        return GetScriptForDestination(WitnessV0KeyHash(Hash160(vSolutions[0])));
    } else if (typ == TxoutType::PUBKEYHASH) {
      //  return GetScriptForDestination(WitnessV0KeyHash(vSolutions[0]));
    }

    return GetScriptForDestination(WitnessV0ScriptHash(redeemscript));
}

bool IsValidDestination(const CTxDestination& dest) {
    return dest.index() != 0;
}

CScript GetScriptForCLTV(CScriptNum nFreezeLockTime, const CPubKey& pubKey)
{
    // TODO Perhaps add limit tests for nLockTime eg. 10 year max lock
    return CScript() << nFreezeLockTime << OP_CHECKLOCKTIMEVERIFY << OP_DROP << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;

}

CScript GetScriptForCoinFlip(const CPubKey& p1pubKey, const CPubKey& p2pubKey, uint256 & p1image, uint256& p2image)
{

 return CScript()   << OP_DUP << OP_SHA256 << std::vector<unsigned char>(p1image.begin(), p1image.end())  << OP_EQUALVERIFY << OP_SIZE << OP_TOALTSTACK << OP_DROP
                    << OP_DUP << OP_SHA256 << std::vector<unsigned char>(p2image.begin(), p2image.end())  << OP_EQUALVERIFY << OP_SIZE << OP_TOALTSTACK << OP_DROP
                    << OP_FROMALTSTACK << OP_16 << OP_NUMEQUAL << OP_FROMALTSTACK<< OP_16 << OP_NUMEQUAL << OP_ADD << OP_1
                    << OP_EQUAL<< OP_IF << std::vector<unsigned char>(p1pubKey.begin(), p1pubKey.end()) << OP_ELSE << std::vector<unsigned char>(p2pubKey.begin(), p2pubKey.end()) << OP_ENDIF << OP_CHECKSIGVERIFY;
}

CScript GetScriptForPvP(CScriptNum nFreezeLockTime, const CPubKey& p1pubKey, const CPubKey& p2pubKey, const CPubKey& recoverypubKey, uint256 & p1image, uint256& p2image)
{
    //to do
    //use map of keys and hashes for participants
    return CScript() << OP_IF << OP_SIZE << 20 << OP_EQUALVERIFY << OP_SHA256 << std::vector<unsigned char>(p1image.begin(), p1image.end())
                     << OP_EQUALVERIFY << std::vector<unsigned char>(p1pubKey.begin(), p1pubKey.end()) << OP_ELSE
                     << OP_IF << OP_SIZE << 20 << OP_EQUALVERIFY << OP_SHA256 << std::vector<unsigned char>(p2image.begin(), p2image.end())
                     << OP_EQUALVERIFY  << std::vector<unsigned char>(p2pubKey.begin(), p2pubKey.end()) << OP_EQUALVERIFY << OP_ELSE
                     << nFreezeLockTime << OP_CHECKSEQUENCEVERIFY << OP_VERIFY << std::vector<unsigned char>(recoverypubKey.begin(), recoverypubKey.end())
                     << OP_ENDIF << OP_ENDIF << OP_CHECKSIG;
}

// Many thanks to Andrew Stone for this
CScript GetBinaryOptionScript(CScriptNum snum, const CPubKey& p1pubKey, const CPubKey& p2pubKey, const CPubKey& oraclepubKey){

    CScript script;
	// push the oracle's public key onto the stack
	script << std::vector<unsigned char>(oraclepubKey.begin(), oraclepubKey.end())
	// verify the data and signature that the spend script left on the stack with the pubkey we just pushed (our new opcode).
	<< OP_CHECKDATASIGVERIFY // if it verifies, the oracle's data is left on the stack so push our binary option strike price onto the stack to compare
	<< snum	// compare the price we just pushed with the oracle’s data
	<< OP_LESSTHAN	// depending on the result, we'll allow payment to one or the other of our participants
	<< OP_IF	  // the normal beginning of a P2PKH tx
	<< OP_DUP  // Duplicating the winner's public key
	<< OP_HASH160 // turn it into a bitcoin address
	<< std::vector<unsigned char>(p1pubKey.GetID().begin(), p1pubKey.GetID().end())
	<< OP_ELSE
	<< OP_DUP
	<< OP_HASH160
	<< std::vector<unsigned char>(p2pubKey.GetID().begin(), p2pubKey.GetID().end())
	<< OP_ENDIF	// The normal end of a P2PKH tx
	<< OP_EQUALVERIFY // compare the bitcoin address against the hashed pub key
	<< OP_CHECKSIGVERIFY; // validate the transaction matches the sig

    return script;
}

CScript GetScriptForHTLC(const CPubKey& seller,
                         const CPubKey& refund,
                         const std::vector<unsigned char> image,
                         uint32_t timeout,
                         opcodetype hasher_type,
                         opcodetype timeout_type)
{
    CScript script;

    script << OP_IF;
    script <<   hasher_type << image << OP_EQUALVERIFY << ToByteVector(seller);
    script << OP_ELSE;

    if (timeout <= 16) {
        script << CScript::EncodeOP_N(timeout);
    } else {
        script << CScriptNum(timeout);
    }

    script <<   timeout_type << OP_DROP << ToByteVector(refund);
    script << OP_ENDIF;
    script << OP_CHECKSIG;

    return script;
}

bool IsSimpleCLTV(const CScript& script, int64_t& cltv_height, int64_t& cltv_time)
{
    CScript::const_iterator pc = script.begin();
    opcodetype opcode;
    std::vector<unsigned char> vch;

    cltv_height = 0;
    cltv_time = 0;

    if (!(script.GetOp(pc, opcode, vch)
       && opcode <= OP_PUSHDATA4
       && script.GetOp(pc, opcode)
       && opcode != OP_CHECKLOCKTIMEVERIFY)) {
        return false;
    }

    CScriptNum sn(0);
    try {
        sn = CScriptNum(vch, true, 5);
    } catch (scriptnum_error) {
        return false;
    }
    if (sn < 0 || sn > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    if (sn < LOCKTIME_THRESHOLD) {
        cltv_height = sn.GetInt64();
    } else {
        cltv_time = sn.GetInt64();
    }
    return true;
}

CScript GetNormalStealthScript (std::vector<unsigned char> vch, CScript scriptPubKey) {
    CScript script;
    script << vch << OP_DROP << scriptPubKey;
	return script;
}

CScript GetScriptForP2ES(std::vector<std::pair<std::string,CPubKey> > parties, const CPubKey& oraclepubKey ){

    CScript script;

    for(unsigned int i = 0; i < parties.size(); i++){
        script << OP_DUP << std::vector<unsigned char>(parties[i].first.begin(), parties[i].first.end()) << OP_EQUAL << OP_IF << OP_DROP << std::vector<unsigned char>(parties[i].first.begin(), parties[i].first.end()) << std::vector<unsigned char>(oraclepubKey.begin(), oraclepubKey.end()) << OP_CHECKDATASIGVERIFY << std::vector<unsigned char>(parties[i].second.begin(), parties[i].second.end());

        if(i == parties.size() -1)
            script << OP_ENDIF;
        else
            script << OP_ELSE;
    }

    script << OP_ENDIF << OP_ENDIF << OP_CHECKSIG;

	return script;
}
/*
CScript GetScriptForP2ES(std::map<int, std::map<std::string,CScript> > parties, const CPubKey& oraclepubKey ){
    CScript script;
    script
    << OP_DUP << 706c617965723177696e73 << OP_EQUAL << OP_IF << OP_DROP << 706c617965723177696e73 << std::vector<unsigned char>(oraclepubKey.begin(), oraclepubKey.end()) << OP_CHECKDATASIGVERIFY
    << OP_DUP << OP_HASH160 << 44a45625a1fda976376e7d59d27fc621f9c9d382 << OP_ELSE
    << OP_DUP << 706c617965723277696e73 << OP_EQUAL << OP_IF << OP_DROP << 706c617965723277696e73 << std::vector<unsigned char>(oraclepubKey.begin(), oraclepubKey.end()) << OP_CHECKDATASIGVERIFY
    << OP_DUP << OP_HASH160 << 9383fa6588a176c2592cb2f4008d779293246adb << OP_ELSE
    << OP_DUP << 706c617965723377696e73 << OP_EQUAL << OP_IF << OP_DROP << 706c617965723377696e73 << std::vector<unsigned char>(oraclepubKey.begin(), oraclepubKey.end()) << OP_CHECKDATASIGVERIFY
    << OP_DUP << OP_HASH160 << 9011100d12d0537232692b3c113be5a8f5053955
    << OP_ENDIF
    << OP_ENDIF << OP_ENDIF << OP_EQUALVERIFY << OP_CHECKSIG;
	return script;

}*/
