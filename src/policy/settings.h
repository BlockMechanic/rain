// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_POLICY_SETTINGS_H
#define RAIN_POLICY_SETTINGS_H

#include <policy/policy.h>

class CFeeRate;
class CTransaction;

// Policy settings which are configurable at runtime.
extern CFeeRate incrementalRelayFee;
extern CFeeRate dustRelayFee;
extern unsigned int nBytesPerSigOp;
extern bool fIsBareMultisigStd;

static inline bool IsStandardTx(const CTransaction& tx, std::string& reason)
{
    return IsStandardTx(tx, ::fIsBareMultisigStd, ::dustRelayFee, reason);
}

#endif // RAIN_POLICY_SETTINGS_H
