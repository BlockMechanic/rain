// Copyright (c) 2014-2020 The Dash Core developers
// Copyright (c) 2018-2019 The RAIN developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "hash.h"
#include "protocol.h" // For strMessageMagic
#include "messagesigner.h"
#include "tinyformat.h"
#include "util/strencodings.h"
#include <util/validation.h>
#include "key_io.h"

bool CMessageSigner::GetKeysFromSecret(const std::string& strSecret, CKey& keyRet, CPubKey& pubkeyRet)
{
    keyRet = DecodeSecret(strSecret);
    if (!keyRet.IsValid())
        return false;

    pubkeyRet = keyRet.GetPubKey();
    return true;
}

uint256 CMessageSigner::GetMessageHash(const std::string& strMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;
    return ss.GetHash();
}

bool CMessageSigner::SignMessage(const std::string& strMessage, std::vector<unsigned char>& vchSigRet, const CKey& key)
{
    return CHashSigner::SignHash(GetMessageHash(strMessage), key, vchSigRet);
}

bool CMessageSigner::VerifyMessage(const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, const std::string& strMessage, std::string& strErrorRet)
{
    return VerifyMessage(pubkey.GetID(), vchSig, strMessage, strErrorRet);
}

bool CMessageSigner::VerifyMessage(const CKeyID& keyID, const std::vector<unsigned char>& vchSig, const std::string& strMessage, std::string& strErrorRet)
{
    return CHashSigner::VerifyHash(GetMessageHash(strMessage), keyID, vchSig, strErrorRet);
}

bool CHashSigner::SignHash(const uint256& hash, const CKey& key, std::vector<unsigned char>& vchSigRet)
{
    return key.SignCompact(hash, vchSigRet);
}

bool CHashSigner::VerifyHash(const uint256& hash, const CPubKey& pubkey, const std::vector<unsigned char>& vchSig, std::string& strErrorRet)
{
    return VerifyHash(hash, pubkey.GetID(), vchSig, strErrorRet);
}

bool CHashSigner::VerifyHash(const uint256& hash, const CKeyID& keyID, const std::vector<unsigned char>& vchSig, std::string& strErrorRet)
{
    CPubKey pubkeyFromSig;
    if(!pubkeyFromSig.RecoverCompact(hash, vchSig)) {
        strErrorRet = "Error recovering public key.";
        return false;
    }

    if(pubkeyFromSig.GetID() != keyID) {
        strErrorRet = strprintf("Keys don't match: pubkey=%s, pubkeyFromSig=%s, hash=%s, vchSig=%s",
                EncodeDestination(PKHash(keyID)), EncodeDestination(PKHash(pubkeyFromSig)),
                hash.ToString(), EncodeBase64(&vchSig[0], vchSig.size()));
        return false;
    }

    return true;
}
