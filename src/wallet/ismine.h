// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_WALLET_ISMINE_H
#define RAIN_WALLET_ISMINE_H

#include <script/standard.h>

#include <stdint.h>
#include <bitset>

class CWallet;
class CScript;

/** IsMine() return codes */
enum isminetype : unsigned int
{
    ISMINE_NO         = 0,
    ISMINE_SPENDABLE  = 1 << 1,
    ISMINE_USED       = 1 << 2,
    ISMINE_COLD = 1 << 3,
    ISMINE_SPENDABLE_DELEGATED = 1 << 4,
    //! Indicates that we dont know how to create a scriptSig that would solve this if we were given the appropriate private keys
    ISMINE_WATCH_UNSOLVABLE = 1 << 5,
    //! Indicates that we know how to create a scriptSig that would solve this if we were given the appropriate private keys
    ISMINE_WATCH_SOLVABLE = 1 << 6,
    ISMINE_WATCH_ONLY = ISMINE_WATCH_SOLVABLE | ISMINE_WATCH_UNSOLVABLE,
    ISMINE_SPENDABLE_ALL = ISMINE_SPENDABLE_DELEGATED | ISMINE_SPENDABLE,
    ISMINE_SPENDABLE_STAKEABLE = ISMINE_SPENDABLE_DELEGATED | ISMINE_COLD,
    ISMINE_ALL = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE | ISMINE_COLD | ISMINE_SPENDABLE_DELEGATED,
    ISMINE_ALL_USED   = ISMINE_ALL | ISMINE_USED,
    ISMINE_ENUM_ELEMENTS,
};
/** used for bitflags of isminetype */
typedef uint8_t isminefilter;

isminetype IsMine(const CWallet& wallet, const CScript& scriptPubKey);
isminetype IsMine(const CWallet& wallet, const CTxDestination& dest);

/**
 * Cachable amount subdivided into watchonly and spendable parts.
 */
struct CachableAmount
{
    // NO and ALL are never (supposed to be) cached
    std::bitset<ISMINE_ENUM_ELEMENTS> m_cached;
    CAmountMap m_value[ISMINE_ENUM_ELEMENTS];
    inline void Reset()
    {
        m_cached.reset();
    }
    void Set(isminefilter filter, CAmountMap value)
    {
        m_cached.set(filter);
        m_value[filter] = value;
    }
};

#endif // RAIN_WALLET_ISMINE_H
