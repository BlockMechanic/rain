// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_CHAINPARAMS_H
#define RAIN_CHAINPARAMS_H

#include <chainparamsbase.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <protocol.h>

#include <memory>
#include <vector>

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;
};

/**
 * Holds various statistics on transactions within a chain. Used to estimate
 * verification progress during chain sync.
 *
 * See also: CChainParams::TxData, GuessVerificationProgress.
 */
struct ChainTxData {
    int64_t nTime;    //!< UNIX timestamp of last known number of transactions
    int64_t nTxCount; //!< total number of transactions between genesis and that timestamp
    double dTxRate;   //!< estimated number of transactions per second after that timestamp
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Rain system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,
        STAKING_ADDRESS,
        BLINDED_ADDRESS,
        MAX_BASE58_TYPES
    };

    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    int GetDefaultPort() const { return nDefaultPort; }

    const CBlock& GenesisBlock() const { return genesis; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Policy: Filter transactions that do not match well-defined patterns */
    bool RequireStandard() const { return fRequireStandard; }
    /** Require addresses specified with "-externalip" parameter to be routable */
    bool RequireRoutableExternalIP() const { return fRequireRoutableExternalIP; }

    /** returns the coinstake maturity (min depth required) **/
    int COINSTAKE_MIN_AGE() const { return nStakeMinAge; }
    int COINSTAKE_MAX_AGE() const { return nStakeMaxAge; }

    /** If this chain is exclusively used for testing */
    bool IsTestChain() const { return m_is_test_chain; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    /** Allow multiple addresses to be selected from the same network group (e.g. 192.168.x.x) */
    bool AllowMultipleAddressesFromGroup() const { return fAllowMultipleAddressesFromGroup; }
    /** Minimum free space (in GB) needed for data directory */
    uint64_t AssumedBlockchainSize() const { return m_assumed_blockchain_size; }
    /** Minimum free space (in GB) needed for data directory when pruned; Does not include prune target*/
    uint64_t AssumedChainStateSize() const { return m_assumed_chain_state_size; }
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool MineBlocksOnDemand() const { return consensus.fPowNoRetargeting; }
    /** Allow nodes with the same address and multiple ports */
    bool AllowMultiplePorts() const { return fAllowMultiplePorts; }
    /** How long to wait until we allow retrying of a LLMQ connection  */
    int LLMQConnectionRetryTimeout() const { return nLLMQConnectionRetryTimeout; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string NetworkIDString() const { return strNetworkID; }
    /** Return true if the fallback fee is by default enabled for this network */
    bool IsFallbackFeeEnabled() const { return m_fallback_fee_enabled; }
    /** Return the list of hostnames to look up for DNS seeds */
    const std::vector<std::string>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::string& Bech32HRP() const { return bech32_hrp; }
    const std::string& Blech32HRP() const { return blech32_hrp; }
    const std::vector<SeedSpec6>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }
    const ChainTxData& TxData() const { return chainTxData; }

    int PoolMinParticipants() const { return nPoolMinParticipants; }
    int PoolMaxParticipants() const { return nPoolMaxParticipants; }
    int FulfilledRequestExpireTime() const { return nFulfilledRequestExpireTime; }
    const std::vector<std::string>& SporkAddresses() const { return vSporkAddresses; }
    int MinSporkKeys() const { return nMinSporkKeys; }

    CAmount MasternodeCollateral() const { return nMasternodeCollateral; }
    bool NewSigsActive() const { return true; }
    bool anyonecanspend_aremine;

protected:
    CChainParams() {}

    Consensus::Params consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    int nDefaultPort;
    int nStakeMinAge;
    int nStakeMaxAge;
    uint64_t nPruneAfterHeight;
    uint64_t m_assumed_blockchain_size;
    uint64_t m_assumed_chain_state_size;
    std::vector<std::string> vSeeds;
    CAmount nMasternodeCollateral;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    int nExtCoinType;
    std::string bech32_hrp;
    std::string blech32_hrp;
    std::string strNetworkID;
    CBlock genesis;
    CAmount initialFreeCoins;
    CAmount initial_reissuance_tokens;
    std::vector<SeedSpec6> vFixedSeeds;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool fRequireRoutableExternalIP;
    bool fAllowMultipleAddressesFromGroup;
    bool m_is_test_chain;
    bool fAllowMultiplePorts;
    int nLLMQConnectionRetryTimeout;
    CCheckpointData checkpointData;
    ChainTxData chainTxData;
    bool m_fallback_fee_enabled;
    int nPoolMinParticipants;
    int nPoolMaxParticipants;
    int nFulfilledRequestExpireTime;
    std::vector<std::string> vSporkAddresses;
    int nMinSporkKeys;
};

/**
 * Creates and returns a std::unique_ptr<CChainParams> of the chosen chain.
 * @returns a CChainParams* of the chosen chain.
 * @throws a std::runtime_error if the chain is not supported.
 */
std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain);

/**
 * Return the currently selected parameters. This won't change after app
 * startup, except for unit tests.
 */
const CChainParams &Params();

/**
 * Sets the params returned by Params() to those for the given BIP70 chain name.
 * @throws std::runtime_error when the chain is not supported.
 */
void SelectParams(const std::string& chain);
#endif // RAIN_CHAINPARAMS_H
