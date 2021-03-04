// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>
#include <policy/policy.h>

#include <tinyformat.h>

const std::string CURRENCY_UNIT = "XQM";

CFeeRate::CFeeRate(const CAmountMap& nFeePaid, size_t nBytes_)
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nSize > 0)
        nSatoshisPerK = nFeePaid * 1000 / nSize;
    else
        nSatoshisPerK = CAmountMap();
}

CAmountMap CFeeRate::GetFee(size_t nBytes_) const
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);
    
    CAmount rr = nSize / 1000 ;
    CAmountMap nFee = nSatoshisPerK * rr;
    CAmountMap tmp = populateMap(1); 

    if (nFee == CAmountMap() && nSize != 0) {
        if (nSatoshisPerK > CAmountMap())
            nFee = tmp;
        if (nSatoshisPerK < CAmountMap())
            nFee = tmp*-1;
    }

    return nFee;
}

std::string CFeeRate::ToString() const
{
    std::string str="";
    for(auto& a : nSatoshisPerK){
        str += a.first.GetHex() + " : " + strprintf("%d.%08d %s/kB", a.second / COIN, a.second % COIN, CURRENCY_UNIT);
    }
    return str;
}
