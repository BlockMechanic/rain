// Copyright (c) 2017-2017 The Elements Core developers
// Copyright (c) 2017-2017 Block Mechanic
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/assetsdir.h>
#include <chainparams.h>

#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <util/translation.h>
#include <base58.h>

#include <policy/policy.h>
#include <clientversion.h>
#include <random.h>
#include <streams.h>

#include <key.h>
#include <key_io.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>

// GLOBAL:
std::vector<CAsset> _initAssets(std::string& signingkey)
{
    std::vector<CAsset> tmp;
/*
    CKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(signingkey, data)) {
        const std::vector<unsigned char>& privkey_prefix = std::vector<unsigned char>(1,128);
        if ((data.size() == 32 + privkey_prefix.size() || (data.size() == 33 + privkey_prefix.size() && data.back() == 1)) &&
            std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
            bool compressed = data.size() == 33 + privkey_prefix.size();
            key.Set(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32, compressed);
        }
    }
    if (!data.empty()) {
        memory_cleanse(data.data(), data.size());
    }

	if (!key.IsValid())
		return tmp;

	CPubKey pubKey = key.GetPubKey();
	if(!key.VerifyPubKey(pubKey))
		return tmp;

	if (!pubKey.IsValid())
		return tmp;

	CScript address = GetScriptForDestination(PKHash(pubKey));
*/
    return tmp;
}
