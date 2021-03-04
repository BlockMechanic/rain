// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>
#include <chainparams.h>
#include <logging.h>

typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

ScriptHash::ScriptHash(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}
ScriptHash::ScriptHash(const CScript& in, const CPubKey& blinding_pubkey_in) : uint160(Hash160(in.begin(), in.end())), blinding_pubkey(blinding_pubkey_in) {}
ScriptHash::ScriptHash(const uint160& hash, const CPubKey& blinding_pubkey_in) : uint160(hash), blinding_pubkey(blinding_pubkey_in) {}

PKHash::PKHash(const CPubKey& pubkey) : uint160(pubkey.GetID()) {}
PKHash::PKHash(const CPubKey& pubkey, const CPubKey& blinding_pubkey_in) : uint160(pubkey.GetID()), blinding_pubkey(blinding_pubkey_in) {}
PKHash::PKHash(const uint160& hash, const CPubKey& blinding_pubkey_in) : uint160(hash), blinding_pubkey(blinding_pubkey_in) {}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in, const CPubKey& blinding_pubkey_in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
    blinding_pubkey = blinding_pubkey_in;
}


const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_MULTISIG: return "multisig";
    case TX_NULL_DATA: return "nulldata";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TX_WITNESS_UNKNOWN: return "witness_unknown";
    case TX_TRUE: return "true";
    case TX_FEE: return "fee";
    case TX_COLDSTAKE: return "coldstake";
    case TX_HTLC: return "htlc";
    case TX_CLTV: return "cltv";  // CLTV HODL Freeze
    case TX_MULTISIG_CLTV: return "multisig_cltv";
    }
    return nullptr;
}

CKeyID ToKeyID(const PKHash& key_hash)
{
    return CKeyID{static_cast<uint160>(key_hash)};
}

static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::PUBLIC_KEY_SIZE + 2 && script[0] == CPubKey::PUBLIC_KEY_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::PUBLIC_KEY_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE + 2 && script[0] == CPubKey::COMPRESSED_PUBLIC_KEY_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_PUBLIC_KEY_SIZE + 1);
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

static bool MatchPayToColdStaking(const CScript& script, valtype& stakerPubKeyHash, valtype& ownerPubKeyHash)
{
    if (script.IsPayToColdStaking()) {
        stakerPubKeyHash = valtype(script.begin () + 6, script.begin() + 26);
        ownerPubKeyHash = valtype(script.begin () + 28, script.begin() + 48);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}

bool MatchMultisig(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
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

bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char>>& vSolutionsRet)
{
    // Templates
    static std::multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Freeze tx using CLTV ; nFreezeLockTime CLTV DROP (0x21 pubkeys) checksig
        mTemplates.insert(std::make_pair(TX_CLTV, CScript() << OP_BIGINTEGER << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_PUBKEYS << OP_CHECKSIG));
        mTemplates.insert(std::make_pair(TX_MULTISIG_CLTV, CScript() << OP_U32INT << OP_CHECKLOCKTIMEVERIFY << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));

        // HTLC where sender requests preimage of a hash
        {
            const opcodetype accepted_hashers[] = {OP_SHA256, OP_RIPEMD160};
            const opcodetype accepted_timeout_ops[] = {OP_CHECKLOCKTIMEVERIFY, OP_CHECKSEQUENCEVERIFY};

            for(auto hasher: accepted_hashers) {
                for(auto timeout_op: accepted_timeout_ops) {
                    mTemplates.insert(std::make_pair(TX_HTLC, CScript()
                        << hasher << OP_ANYDATA << OP_EQUAL
                        << OP_IF
                        <<     OP_PUBKEY
                        << OP_ELSE
                        <<     OP_ANYDATA << timeout_op << OP_DROP << OP_PUBKEY
                        << OP_ENDIF
                        << OP_CHECKSIG
                    ));
                }
            }
        }

    }

    vSolutionsRet.clear();

    if (scriptPubKey == CScript() << OP_TRUE) { // to be reviewed
        typeRet = TX_TRUE;
        return true;
    }

    // Fee outputs are for elements-style transactions only
    if (scriptPubKey == CScript()) {
        typeRet = TX_FEE;
        return true;
    }

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            typeRet = TX_WITNESS_V0_KEYHASH;
            vSolutionsRet.push_back(witnessprogram);
            return true;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            typeRet = TX_WITNESS_V0_SCRIPTHASH;
            vSolutionsRet.push_back(witnessprogram);
            return true;
        }
        if (witnessversion != 0) {
            typeRet = TX_WITNESS_UNKNOWN;
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return true;
        }
        typeRet = TX_NONSTANDARD;
        return false;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin()+1)) {
        typeRet = TX_NULL_DATA;
        return true;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        typeRet =  TX_PUBKEY;
        return true;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        typeRet =  TX_PUBKEYHASH;
        return true;
    }

    std::vector<unsigned char> data1;
    if (MatchPayToColdStaking(scriptPubKey, data, data1)) {
        typeRet = TX_COLDSTAKE;
        vSolutionsRet.push_back(std::move(data));
        vSolutionsRet.push_back(std::move(data1));
        return true;
    }

    unsigned int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        typeRet =  TX_MULTISIG;
        return true;
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
                if (typeRet == TX_MULTISIG || typeRet == TX_MULTISIG_CLTV)
                {
                    // Additional checks for TX_MULTISIG:
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


    vSolutionsRet.clear();
    typeRet = TX_NONSTANDARD;
    return false;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet, txnouttype *typeRet, bool fColdStake)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    if(typeRet){
        *typeRet = whichType;
    }

    if (whichType == TX_PUBKEY) {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    else if (whichType == TX_PUBKEYHASH)
    {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    }
    else if (whichType == TX_SCRIPTHASH)
    {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TX_WITNESS_V0_KEYHASH) {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_V0_SCRIPTHASH) {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_UNKNOWN) {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    } else if (whichType == TX_COLDSTAKE) {
        addressRet = PKHash(uint160(vSolutions[!fColdStake]));
        return true;
    } else if (whichType == TX_CLTV) {
    	CPubKey pubKey(vSolutions[1]);
	if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    std::vector<valtype> vSolutions;
    if (!Solver(scriptPubKey, typeRet, vSolutions))
        return false;
    if (typeRet == TX_NULL_DATA){
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG || typeRet == TX_MULTISIG_CLTV)
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

    } else if (typeRet == TX_COLDSTAKE)
    {
        if (vSolutions.size() < 2)
            return false;
        nRequiredRet = 2;
        addressRet.push_back(PKHash(uint160(vSolutions[0])));
        addressRet.push_back(PKHash(uint160(vSolutions[1])));
        return true;

    }
    else if (typeRet == TX_HTLC)
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

namespace
{
class CScriptVisitor : public boost::static_visitor<bool>
{
private:
    CScript *script;
public:
    explicit CScriptVisitor(CScript *scriptin) { script = scriptin; }

    bool operator()(const CNoDestination &dest) const {
        script->clear();
        return false;
    }

    bool operator()(const PKHash &keyID) const {
        script->clear();
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const ScriptHash &scriptID) const {
        script->clear();
        *script << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }

    bool operator()(const WitnessV0KeyHash& id) const
    {
        script->clear();
        *script << OP_0 << ToByteVector(id);
        return true;
    }

    bool operator()(const WitnessV0ScriptHash& id) const
    {
        script->clear();
        *script << OP_0 << ToByteVector(id);
        return true;
    }

    bool operator()(const WitnessUnknown& id) const
    {
        script->clear();
        *script << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
        return true;
    }

    bool operator()(const NullData& id) const
    {
        script->clear();
        *script << OP_RETURN;
        for (const auto& push : id.null_data) {
            *script << push;
        }
        return true;
    }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    CScript script;

    boost::apply_visitor(CScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForStakeDelegation(const CKeyID& stakingKey, const CKeyID& spendingKey)
{
    CScript script;
    script << OP_DUP << OP_HASH160 << OP_ROT <<
            OP_IF << OP_CHECKCOLDSTAKEVERIFY << ToByteVector(stakingKey) <<
            OP_ELSE << ToByteVector(spendingKey) << OP_ENDIF <<
            OP_EQUALVERIFY << OP_CHECKSIG;
    return script;
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
    txnouttype typ;
    std::vector<std::vector<unsigned char> > vSolutions;
    if (Solver(redeemscript, typ, vSolutions)) {
        if (typ == TX_PUBKEY) {
            return GetScriptForDestination(WitnessV0KeyHash(Hash160(vSolutions[0].begin(), vSolutions[0].end())));
        } else if (typ == TX_PUBKEYHASH) {
            return GetScriptForDestination(WitnessV0KeyHash(vSolutions[0]));
        }
    }
    return GetScriptForDestination(WitnessV0ScriptHash(redeemscript));
}

bool IsValidDestination(const CTxDestination& dest) {
    return dest.which() != 0;
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
