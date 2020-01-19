// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crosschain/interblockchain.h>
#include <protocol.h>
#include <chainparams.h>
#include <util/strencodings.h>

#include <boost/foreach.hpp>

void CIbtp::LoadMsgStart()
{
	//need to find a dynamic method of adding supported networks
    vChains.push_back(SChain("rain", "RAIN", 0x47, 0x24, 0xa1, 0xb6, 23373));
    vChains.push_back(SChain("talk", "TALK", 0xf7, 0xba, 0xd4, 0xa8, 8372));
    vChains.push_back(SChain("bzedge", "BZE", 0x24, 0xe9, 0x27, 0x64, 1990));
    vChains.push_back(SChain("Codex", "CDX", 0xf7, 0xba, 0xd4, 0xaa, 8384));
}

bool CIbtp::IsIbtpChain(const unsigned char msgStart[], std::string& chainName)
{
    bool bFound = false;
    BOOST_FOREACH(SChain p, vChains)
    {
        unsigned char pchMsg[4] = { p.pchMessageOne, p.pchMessageTwo, p.pchMessageThree, p.pchMessageFour };
        if(memcmp(msgStart, pchMsg, sizeof(pchMsg)) == 0)
        {
            bFound = true;
            chainName = p.sChainName;
            LogPrintf("Found Supported chain: %s\n", p.sChainName.c_str());
        }
    }
    return bFound;
}
 //need to specify addresses for burn
 //need to specify either floating or fixed exchange rates 
 // need to specify limit on the amounts that can be transferred
 // need to specify time-frame, ie blocks when it can be done for each coin 
