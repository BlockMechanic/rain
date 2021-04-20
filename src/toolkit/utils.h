// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_TOOLKIT_UTILS_H
#define RAIN_TOOLKIT_UTILS_H

class CWallet;


#include <string>


enum CTxType{
	
	
}

bool run(std::string& line, int64_t count);


void PublickeyFromString(const std::string &ks, CPubKey &out, CWallet * pwallet=nullptr);


#endif



