// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/ismine.h>

#include <key.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <wallet/wallet.h>
#include <chain.h>
typedef std::vector<unsigned char> valtype;

namespace {

/**
 * This is an enum that tracks the execution context of a script, similar to
 * SigVersion in script/interpreter. It is separate however because we want to
 * distinguish between top-level scriptPubKey execution and P2SH redeemScript
 * execution (a distinction that has no impact on consensus rules).
 */
enum class IsMineSigVersion
{
    TOP = 0,        //!< scriptPubKey execution
    P2SH = 1,       //!< P2SH redeemScript
    WITNESS_V0 = 2, //!< P2WSH witness script execution
};

/**
 * This is an internal representation of isminetype + invalidity.
 * Its order is significant, as we return the max of all explored
 * possibilities.
 */
enum class IsMineResult
{
    NO = 0,         //!< Not ours
    WATCH_ONLY = 1, //!< Included in watch-only balance
    SPENDABLE = 2,  //!< Included in all balances
    INVALID = 3,    //!< Not spendable by anyone (uncompressed pubkey in segwit, P2SH inside P2SH or witness, witness inside witness)
    COLD = 4,       //!< Cold balances
    DELEGATED = 5,  //!< Delegated balances
    WATCH_SOLVABLE = 6 //!< htlc
};

bool PermitsUncompressed(IsMineSigVersion sigversion)
{
    return sigversion == IsMineSigVersion::TOP || sigversion == IsMineSigVersion::P2SH;
}

bool HaveKeys(const std::vector<valtype>& pubkeys, const CWallet& keystore)
{
    for (const valtype& pubkey : pubkeys) {
        CKeyID keyID = CPubKey(pubkey).GetID();
        if (!keystore.HaveKey(keyID)) return false;
    }
    return true;
}

IsMineResult IsMineInner(const CWallet& keystore, const CScript& scriptPubKey, IsMineSigVersion sigversion)
{
    IsMineResult ret = IsMineResult::NO;
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if(!Solver(scriptPubKey, whichType, vSolutions)) {
        if (keystore.HaveWatchOnly(scriptPubKey))
            ret = IsMineResult::WATCH_ONLY;
        return ret;
    }

    CKeyID keyID;
    switch (whichType)
    {
    case TX_NONSTANDARD:
    case TX_NULL_DATA:
    case TX_WITNESS_UNKNOWN:
    case TX_FEE:
        break;
    case TX_PUBKEY:
        keyID = CPubKey(vSolutions[0]).GetID();
        if (!PermitsUncompressed(sigversion) && vSolutions[0].size() != 33) {
            return IsMineResult::INVALID;
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TX_WITNESS_V0_KEYHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WPKH inside P2WSH is invalid.
            return IsMineResult::INVALID;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            // We do not support bare witness outputs unless the P2SH version of it would be
            // acceptable as well. This protects against matching before segwit activates.
            // This also applies to the P2WSH case.
            break;
        }
        ret = std::max(ret, IsMineInner(keystore, GetScriptForDestination(PKHash(uint160(vSolutions[0]))), IsMineSigVersion::WITNESS_V0));
        break;
    }
    case TX_PUBKEYHASH:
        keyID = CKeyID(uint160(vSolutions[0]));
        if (!PermitsUncompressed(sigversion)) {
            CPubKey pubkey;
            if (keystore.GetPubKey(keyID, pubkey) && !pubkey.IsCompressed()) {
                return IsMineResult::INVALID;
            }
        }
        if (keystore.HaveKey(keyID)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    case TX_SCRIPTHASH:
    {
        if (sigversion != IsMineSigVersion::TOP) {
            // P2SH inside P2WSH or P2SH is invalid.
            return IsMineResult::INVALID;
        }
        CScriptID scriptID = CScriptID(uint160(vSolutions[0]));
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, IsMineInner(keystore, subscript, IsMineSigVersion::P2SH));
        }
        break;
    }
    case TX_COLDSTAKE: {
        CKeyID stakeKeyID = CKeyID(uint160(vSolutions[0]));
        bool stakeKeyIsMine = keystore.HaveKey(stakeKeyID);
        CKeyID ownerKeyID = CKeyID(uint160(vSolutions[1]));
        bool spendKeyIsMine = keystore.HaveKey(ownerKeyID);

        if (spendKeyIsMine && stakeKeyIsMine)
            return IsMineResult::SPENDABLE;
        else if (stakeKeyIsMine)
            return IsMineResult::COLD;
        else if (spendKeyIsMine)
            return IsMineResult::DELEGATED;
        break;
    }
    case TX_WITNESS_V0_SCRIPTHASH:
    {
        if (sigversion == IsMineSigVersion::WITNESS_V0) {
            // P2WSH inside P2WSH is invalid.
            return IsMineResult::INVALID;
        }
        if (sigversion == IsMineSigVersion::TOP && !keystore.HaveCScript(CScriptID(CScript() << OP_0 << vSolutions[0]))) {
            break;
        }
        uint160 hash;
        CRIPEMD160().Write(&vSolutions[0][0], vSolutions[0].size()).Finalize(hash.begin());
        CScriptID scriptID = CScriptID(hash);
        CScript subscript;
        if (keystore.GetCScript(scriptID, subscript)) {
            ret = std::max(ret, IsMineInner(keystore, subscript, IsMineSigVersion::WITNESS_V0));
        }
        break;
    }

    case TX_HTLC:
    {
        // Only consider HTLC's "mine" if we own ALL the keys
        // involved.
        std::vector<valtype> keys;
        keys.push_back(vSolutions[1]);
        keys.push_back(vSolutions[3]);
        if (HaveKeys(keys, keystore) == keys.size()) {
            return std::max(ret, IsMineResult::WATCH_SOLVABLE);
        }
        break;
    }

    case TX_MULTISIG:
    case TX_MULTISIG_CLTV:
    {
        // Never treat bare multisig outputs as ours (they can still be made watchonly-though)
        if (sigversion == IsMineSigVersion::TOP) {
            break;
        }

        // Only consider transactions "mine" if we own ALL the
        // keys involved. Multi-signature transactions that are
        // partially owned (somebody else has a key that can spend
        // them) enable spend-out-from-under-you attacks, especially
        // in shared-wallet situations.
        std::vector<valtype> keys(vSolutions.begin()+1, vSolutions.begin()+vSolutions.size()-1);
        if (!PermitsUncompressed(sigversion)) {
            for (size_t i = 0; i < keys.size(); i++) {
                if (keys[i].size() != 33) {
                    return IsMineResult::INVALID;
                }
            }
        }
        if (HaveKeys(keys, keystore)) {
            ret = std::max(ret, IsMineResult::SPENDABLE);
        }
        break;
    }
    case TX_CLTV:
    {
        keyID = CKeyID(uint160(vSolutions[1]));
        if (keystore.HaveKey(keyID))
        {
			auto locked_chain = keystore.chain().lock();
            CScriptNum nFreezeLockTime(vSolutions[0], true, 5);

            LogPrintf("Found Freeze Have Key. nFreezeLockTime=%d \n", nFreezeLockTime.GetInt64());
            if (nFreezeLockTime < LOCKTIME_THRESHOLD)
            {
                // locktime is a block
                if (nFreezeLockTime > locked_chain->currentTip()->nHeight)
                    ret = std::max(ret, IsMineResult::WATCH_SOLVABLE);
                else
                    ret = std::max(ret, IsMineResult::SPENDABLE);
            }
            else
            {
                // locktime is a time
                if (nFreezeLockTime > locked_chain->currentTip()->GetMedianTimePast())
                    ret = std::max(ret, IsMineResult::WATCH_SOLVABLE);
                else
                    ret = std::max(ret, IsMineResult::SPENDABLE);
            }
        }
        else
        {
            LogPrintf("Found Freeze DONT HAVE KEY!! \n");
            ret = IsMineResult::NO;
        }
        break;
    }

    case TX_TRUE:
        if (Params().anyonecanspend_aremine) {
            return IsMineResult::SPENDABLE;
        }
    }

    if (ret == IsMineResult::NO && keystore.HaveWatchOnly(scriptPubKey)) {
        ret = std::max(ret, IsMineResult::WATCH_ONLY);
    }
    return ret;
}

} // namespace

isminetype IsMine(const CWallet& keystore, const CScript& scriptPubKey)
{
    switch (IsMineInner(keystore, scriptPubKey, IsMineSigVersion::TOP)) {
    case IsMineResult::INVALID:
    case IsMineResult::NO:
        return ISMINE_NO;
    case IsMineResult::WATCH_ONLY:
        return ISMINE_WATCH_ONLY;
    case IsMineResult::SPENDABLE:
        return ISMINE_SPENDABLE;
    case IsMineResult::DELEGATED:
        return ISMINE_SPENDABLE_DELEGATED;
    case IsMineResult::COLD:
        return ISMINE_COLD;
    case IsMineResult::WATCH_SOLVABLE:
        return ISMINE_WATCH_SOLVABLE;
    }
    assert(false);
}

isminetype IsMine(const CWallet& keystore, const CTxDestination& dest)
{
    CScript script = GetScriptForDestination(dest);
    return IsMine(keystore, script);
}
