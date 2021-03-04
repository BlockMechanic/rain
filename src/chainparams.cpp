// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <issuance.h>
#include <consensus/consensus.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <crypto/sha256.h>
#include <versionbitsinfo.h>
#include <arith_uint256.h>
#include <policy/fees.h>
#include <assert.h>
#include <primitives/assetsdir.h>
#include <streams.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread.hpp>

// Safer for users if they load incorrect parameters via arguments.
static std::vector<unsigned char> CommitToArguments(const Consensus::Params& params, const std::string& networkID, const CScript& signblockscript)
{
    CSHA256 sha2;
    unsigned char commitment[32];
    sha2.Write((const unsigned char*)networkID.c_str(), networkID.length());
    sha2.Write((const unsigned char*)HexStr(signblockscript).c_str(), HexStr(signblockscript).length());
    sha2.Finalize(commitment);
    return std::vector<unsigned char>(commitment, commitment + 32);
}

static CBlock CreateGenesisBlock(const CScript& genesisScriptSig, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, Consensus::Params& consensus)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.nTime = nTime;
    txNew.vin.resize(1);
    txNew.vin[0].scriptSig = genesisScriptSig;
    //txNew.nType = TRANSACTION_COINBASE;

    AssetMetadata basemeta;
    basemeta.nVersion = 1;
    basemeta.setName("Rain");
    basemeta.setShortName("RAIN");
    basemeta.flags = AssetMetadata::AssetFlags::ASSET_TRANSFERABLE | AssetMetadata::AssetFlags::ASSET_CONVERTABLE | AssetMetadata::AssetFlags::ASSET_STAKEABLE;
    CAsset baseasset = CAsset(basemeta);

    AssetMetadata masternode;
    masternode.nVersion = 0;
    masternode.setName("Masternode");
    masternode.setShortName("XMNA");
    masternode.flags = AssetMetadata::AssetFlags::ASSET_TRANSFERABLE | AssetMetadata::AssetFlags::ASSET_CONVERTABLE;
    CAsset masternodeasset = CAsset(masternode);

    AssetMetadata founder;
    founder.nVersion = 0;
    founder.setName("Xequium");
    founder.setShortName("XQUM");
    founder.flags = AssetMetadata::AssetFlags::ASSET_TRANSFERABLE | AssetMetadata::AssetFlags::ASSET_CONVERTABLE | AssetMetadata::AssetFlags::ASSET_STAKEABLE | AssetMetadata::AssetFlags::ASSET_LIMITED | AssetMetadata::AssetFlags::ASSET_RESTRICTED;
    CAsset founderasset = CAsset(founder);
   
    consensus.subsidy_asset = baseasset;
    consensus.masternode_asset = masternodeasset;
    consensus.founder_asset = founderasset;
    
    txNew.vout.push_back(CTxOut(baseasset, 1*COIN, genesisOutputScript));
    txNew.vout.push_back(CTxOut(masternodeasset, 1*COIN, genesisOutputScript));
    txNew.vout.push_back(CTxOut(founderasset, 1*COIN, genesisOutputScript));

    txNew.nLockTime = 0;
//    std::string txcomment = "www.coindesk.com/bitcoin-is-a-form-of-money-d-c-federal-court-rules-bloomberg";
//    std::copy(txcomment.begin(), txcomment.end(), std::back_inserter(txNew.strTxComment));
//    txNew.strTxComment = txcomment.c_str();

//    size_t nSize = ::GetSerializeSize(txNew.vout[0].nAsset, PROTOCOL_VERSION);
//    std::cout << "size : " << nSize  <<"/"  << txNew.vout[0].nAsset.vchCommitment.size()  << " is valid = "  << (txNew.vout[0].nAsset.IsValid() ? "yes" : " no ") <<std::endl;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nHeight  = 0;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, Consensus::Params& consensus)
{
    const char* pszTimestamp = "www.coindesk.com/bitcoin-is-a-form-of-money-d-c-federal-court-rules-bloomberg";
    const CScript genesisScriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    std::vector<unsigned char> addrdata(ParseHex("76a914913a02f203207febf92a5fb659be964bddfa7c3988ac"));
    const CScript genesisOutputScript(addrdata.begin(), addrdata.end());
    return CreateGenesisBlock(genesisScriptSig, genesisOutputScript, nTime, nNonce, nBits, nVersion, consensus);
}


void static MineNewGenesisBlock(const Consensus::Params& consensus,CBlock &genesis)
{
    //fPrintToConsole = true;
    std::cout << "Searching for genesis block...\n";
    arith_uint256 hashTarget = UintToArith256(consensus.powLimit);

    while(true) {
        arith_uint256 thash = UintToArith256(genesis.GetHash());
        std::cout << "testHash = " << thash.ToString().c_str() << "\n";
        std::cout << "Hash Target = " << hashTarget.ToString().c_str() << "\n";

      if (thash <= hashTarget)
            break;
        if ((genesis.nNonce & 0xFFF) == 0)
            std::cout << strprintf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), hashTarget.ToString().c_str());

        ++genesis.nNonce;
        if (genesis.nNonce == 0) {
            std::cout << ("NONCE WRAPPED, incrementing time\n");
            ++genesis.nTime;
        }
    }
    std::cout << "genesis.nTime = " << genesis.nTime << "\n";
    std::cout << "genesis.nNonce = " << genesis.nNonce<< "\n";
    std::cout << "genesis.nBits = " << genesis.nBits<< "\n";
    std::cout << "genesis.GetHash = " << genesis.GetHash().ToString().c_str()<< "\n";
    std::cout << "genesis.hashMerkleRoot = " << genesis.hashMerkleRoot.ToString().c_str()<< "\n";
//    std::cout << "genesis block \n" << genesis.ToString()<< std::endl;

    exit(1);
}

static Consensus::LLMQParams llmq50_60 = {
        .type = Consensus::LLMQ_50_60,
        .name = "llmq_50_60",
        .size = 50,
        .minSize = 40,
        .threshold = 30,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
        .dkgBadVotesThreshold = 40,

        .signingActiveQuorumCount = 24, // a full day worth of LLMQs

        .keepOldConnections = 25,
        .recoveryMembers = 25,
};

static Consensus::LLMQParams llmq400_60 = {
        .type = Consensus::LLMQ_400_60,
        .name = "llmq_400_60",
        .size = 400,
        .minSize = 300,
        .threshold = 240,

        .dkgInterval = 24 * 12, // one DKG every 12 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 28,
        .dkgBadVotesThreshold = 300,

        .signingActiveQuorumCount = 4, // two days worth of LLMQs

        .keepOldConnections = 5,
        .recoveryMembers = 100,
};

// Used for deployment and min-proto-version signalling, so it needs a higher threshold
static Consensus::LLMQParams llmq400_85 = {
        .type = Consensus::LLMQ_400_85,
        .name = "llmq_400_85",
        .size = 400,
        .minSize = 350,
        .threshold = 340,

        .dkgInterval = 24 * 24, // one DKG every 24 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 48, // give it a larger mining window to make sure it is mined
        .dkgBadVotesThreshold = 300,

        .signingActiveQuorumCount = 4, // four days worth of LLMQs

        .keepOldConnections = 5,
        .recoveryMembers = 100,
};

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        consensus.nMasternodePaymentsStartBlock = 1; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
        consensus.nMasternodePaymentsIncreaseBlock = 1; // actual historical value
        consensus.nMasternodePaymentsIncreasePeriod = 576*30; // 17280 - actual historical value
        consensus.nInstantSendConfirmationsRequired = 6;
        consensus.nInstantSendKeepLock = 24;
        consensus.nBudgetPaymentsStartBlock = 1; // actual historical value
        consensus.nBudgetPaymentsCycleBlocks = 10080; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nSuperblockStartBlock = 1; // The block at which 12.1 goes live (end of final 12.0 budget cycle)
        consensus.nSuperblockStartHash = uint256S("");
        consensus.nSuperblockCycle = 10080; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;

        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00");
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x00");
        consensus.BIP65Height = 0; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 0; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPoSNoRetargeting = false;
        consensus.nStakeTimestampMask = 0xf;
        consensus.nLastPOWBlock = 10000;
        consensus.nStakeMinAge = 60 * 60;
        consensus.nStakeMaxAge = 28 * 24 * 60 * 60;
        consensus.nCOIN_YEAR_REWARD = CENT * 10;
        consensus.nPowTargetTimespan = 60;
        consensus.nPowTargetSpacing = 60;
        consensus.nModifierInterval =  60;
        consensus.CSVHeight = 0; // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 100; // nPowTargetTimespan / nPowTargetSpacing


        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        // Custom chains connect coinbase outputs to db by default
        consensus.connect_genesis_outputs = true;
        consensus.subsidy_asset = CAsset();
        consensus.masternode_asset = CAsset();
        consensus.founder_asset = CAsset();
        anyonecanspend_aremine = false;
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xaa;
        pchMessageStart[1] = 0xab;
        pchMessageStart[2] = 0xac;
        pchMessageStart[3] = 0xad;

        nDefaultPort = 2013;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        nMasternodeCollateral = 10000 * COIN;
        consensus.nMinColdStakingAmount = 1 * COIN;
        consensus.genesis_subsidy = 5*COIN;

        // All non-zero coinbase outputs must go to this scriptPubKey
        std::vector<unsigned char> addrdata(ParseHex("76a914913a02f203207febf92a5fb659be964bddfa7c3988ac"));
        CScript genscript(addrdata.begin(), addrdata.end());

        consensus.mandatory_coinbase_destination = genscript; // Blank script allows any coinbase destination

        genesis = CreateGenesisBlock(1613343600, 1714406, 0x1e0ffff0, 1, consensus);
        consensus.hashGenesisBlock = genesis.GetHash();
        //std::cout << "GENESISHASH "<< genesis.GetHash().ToString() << std::endl;
        //std::cout << "GENESISMERKLE "<< genesis.hashMerkleRoot.ToString() << std::endl;
        //MineNewGenesisBlock(consensus,genesis);
        //std::cout << genesis.ToString() << std::endl;

//        CDataStream ssBlock(SER_GETHASH, PROTOCOL_VERSION);
//        ssBlock << genesis;
//        std::string strHex = HexStr(ssBlock.begin(), ssBlock.end());
//        std::cout << strHex << std::endl;

        assert(consensus.hashGenesisBlock == uint256S("0x0000003fd35dbcaa59cf3cc33b4a4d6fe04d1e987abc7ee95e5843c00c5f0f75"));
        assert(genesis.hashMerkleRoot == uint256S("0xa3d33a9d392004f2c50430d8cf78c8ae660095f9d34bf6f5cf3fd9d5a278363e"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("157.245.239.62");
        vSeeds.emplace_back("45.86.162.195");
        vSeeds.emplace_back("167.172.111.247");
        vSeeds.emplace_back("142.93.235.158");

        base58Prefixes[PUBKEY_ADDRESS]  = std::vector<unsigned char>(1,60);     // starting with 'R'
        base58Prefixes[SCRIPT_ADDRESS]  = std::vector<unsigned char>(1,38);     // starting with 'G'
        base58Prefixes[STAKING_ADDRESS] = std::vector<unsigned char>(1,63);     // starting with 'S'
        base58Prefixes[BLINDED_ADDRESS] = std::vector<unsigned char>(1,25);     // starting with 'B'
        base58Prefixes[SECRET_KEY]      = std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "cd";
        blech32_hrp = "xd";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
        consensus.llmqTypeChainLocks = Consensus::LLMQ_400_60;
        consensus.llmqTypeInstantSend = Consensus::LLMQ_50_60;

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        fRequireRoutableExternalIP = true;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;
        nLLMQConnectionRetryTimeout = 60;

        nPoolMinParticipants = 3;
        nPoolMaxParticipants = 5;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        vSporkAddresses = {"RTat8T37opuVR2S2AdSAUTnjnEtmnLMNDr"};
        nMinSporkKeys = 1;

        checkpointData = {
            {
                { 0, uint256S("0x00")},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 23599282d5f953a73fac7d1710b055078f8397f2da3dbf767195a0db74160728
            /* nTime    */ 1575570915,
            /* nTxCount */ 1,
            /* dTxRate  */ 0.03119858631405764
        };

        /* disable fallback fee on mainnet */
        m_fallback_fee_enabled = false;
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00000000dd30457c001f4095d208cc1296b0eed002427aa599874af7a432b105");
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.BIP65Height = 581885; // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP66Height = 330776; // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 60; // two weeks
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000007dbe94253893cbd463");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75"); //1354312

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 20480;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;
        consensus.genesis_subsidy = 5*COIN;


        genesis = CreateGenesisBlock(1591769651, 295473,0x1e0fffff, 1, consensus);

        consensus.hashGenesisBlock = genesis.GetHash();
        //MineNewGenesisBlock(consensus,genesis);
  //      assert(consensus.hashGenesisBlock == uint256S("0x00000d1809d66fb14054356cf881c4a2dd087d0dac4e9fb84fd3f551c579cadc"));
  //      assert(genesis.hashMerkleRoot == uint256S("0xba0035a477013f6a9d76fea7fd78df4e6ac70e41a87749772137693fadfb9482"));
        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.rain.jonasschnelli.ch");
        vSeeds.emplace_back("seed.tbtc.petertodd.org");
        vSeeds.emplace_back("seed.testnet.rain.sprovoost.nl");
        vSeeds.emplace_back("testnet-seed.bluematt.me"); // Just a static list of stable node(s), only supports x9

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;


        checkpointData = {
            {
                {0, uint256S("0x13bc3f42175c6b3b744c28ec7e37ac59d822483b5eb21a5c5259154746378266")},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75
            /* nTime    */ 1531929919,
            /* nTxCount */ 19438708,
            /* dTxRate  */ 0.626
        };

        /* enable fallback fee on testnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateVersionBitsParametersFromArgs(args);
        consensus.genesis_subsidy = 5*COIN;


        genesis = CreateGenesisBlock(1561928393, 221227,0x1e0fffff, 1,consensus);

        consensus.hashGenesisBlock = genesis.GetHash();
    //    assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
    //    assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;

        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";

        /* enable fallback fee on regtest */
        m_fallback_fee_enabled = true;
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateVersionBitsParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateVersionBitsParametersFromArgs(const ArgsManager& args)
{
    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
