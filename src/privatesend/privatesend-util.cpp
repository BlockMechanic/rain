// Copyright (c) 2014-2019 The Rain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <privatesend/privatesend-util.h>
#include <wallet/wallet.h>

CKeyHolder::CKeyHolder(CWallet* pwallet)
{	
	reservedest->GetReservedDestination(pwallet->m_default_change_type, dest, true);
}

void CKeyHolder::KeepKey()
{
    reservedest->KeepDestination();
}

void CKeyHolder::ReturnKey()
{
    reservedest->ReturnDestination();
}

CScript CKeyHolder::GetScriptForDestination() const
{
    return ::GetScriptForDestination(dest);
}


CScript CKeyHolderStorage::AddKey(CWallet* pwallet)
{
    auto keyHolderPtr = std::unique_ptr<CKeyHolder>(new CKeyHolder(pwallet));
    auto script = keyHolderPtr->GetScriptForDestination();

    LOCK(cs_storage);
    storage.emplace_back(std::move(keyHolderPtr));
    LogPrintf("CKeyHolderStorage::%s -- storage size %lld\n", __func__, storage.size());
    return script;
}

void CKeyHolderStorage::KeepAll()
{
    std::vector<std::unique_ptr<CKeyHolder> > tmp;
    {
        // don't hold cs_storage while calling KeepKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (!tmp.empty()) {
        for (auto& key : tmp) {
            key->KeepKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys kept\n", __func__, tmp.size());
    }
}

void CKeyHolderStorage::ReturnAll()
{
    std::vector<std::unique_ptr<CKeyHolder> > tmp;
    {
        // don't hold cs_storage while calling ReturnKey(), which might lock cs_wallet
        LOCK(cs_storage);
        std::swap(storage, tmp);
    }

    if (!tmp.empty()) {
        for (auto& key : tmp) {
            key->ReturnKey();
        }
        LogPrintf("CKeyHolderStorage::%s -- %lld keys returned\n", __func__, tmp.size());
    }
}
