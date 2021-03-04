// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_KEY_IO_H
#define RAIN_KEY_IO_H

#include <chainparams.h>
#include <key.h>
#include <pubkey.h>
#include <script/standard.h>

#include <string>

CKey DecodeSecret(const std::string& str);
std::string EncodeSecret(const CKey& key);

CExtKey DecodeExtKey(const std::string& str);
std::string EncodeExtKey(const CExtKey& extkey);
CExtPubKey DecodeExtPubKey(const std::string& str);
std::string EncodeExtPubKey(const CExtPubKey& extpubkey);

std::string EncodeDestination(const CTxDestination& dest, bool fColdStake = false);
CTxDestination DecodeDestination(const std::string& str, bool fColdStake = false);
bool IsValidDestinationString(const std::string& str, bool fColdStake = false);
bool IsValidDestinationString(const std::string& str, const CChainParams& params, bool fColdStake = false);
bool IsStakingAddress(const std::string& str);
#endif // RAIN_KEY_IO_H
