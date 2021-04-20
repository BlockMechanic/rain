
#include <issuance.h>

#include <primitives/transaction.h>
#include <amount.h>

void GenerateAssetEntropy(uint256& entropy, const COutPoint& prevout, const uint256& contracthash)
{
    // E : entropy
    // I : prevout
    // C : contract
    // E = H( H(I) || H(C) )
    std::vector<uint256> leaves;
    leaves.reserve(2);
    leaves.push_back(SerializeHash(prevout, SER_GETHASH, 0));
    leaves.push_back(contracthash);
    entropy = ComputeFastMerkleRoot(leaves);
}

void CalculateAsset(CAsset& asset, const uint256& entropy)
{
    static const uint256 kZero = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
    // H_a : asset tag
    // E   : entropy
    // H_a = H( E || 0 )
    std::vector<uint256> leaves;
    leaves.reserve(2);
    leaves.push_back(entropy);
    leaves.push_back(kZero);
    asset = CAsset(ComputeFastMerkleRoot(leaves));
}

void CalculateReissuanceToken(CAsset& reissuanceToken, const uint256& entropy, bool fConfidential)
{
    static const uint256 kOne = uint256S("0x0000000000000000000000000000000000000000000000000000000000000001");
    static const uint256 kTwo = uint256S("0x0000000000000000000000000000000000000000000000000000000000000002");
    // H_a : asset reissuance tag
    // E   : entropy
    // if not fConfidential:
    //     H_a = H( E || 1 )
    // else
    //     H_a = H( E || 2 )
    std::vector<uint256> leaves;
    leaves.reserve(2);
    leaves.push_back(entropy);
    leaves.push_back(fConfidential ? kTwo : kOne);
    reissuanceToken = CAsset(ComputeFastMerkleRoot(leaves));
}

/** Add an issuance transaction to the genesis block. Typically used to pre-issue
 * the policyAsset of a blockchain. The genesis block is not actually validated,
 * so this transaction simply has to match issuance structure. */
void AppendInitialIssuance(CBlock& genesis_block, const COutPoint& prevout, const uint256& contract, const int64_t asset_outputs, const int64_t asset_values, const int64_t reissuance_outputs, const int64_t reissuance_values, const CScript& issuance_destination, const CScript& genesisScriptSig) {
    uint256 entropy;
    GenerateAssetEntropy(entropy, prevout, contract);
    //uint32_t nTime = genesis_block.nTime;

    CAsset asset;
    CalculateAsset(asset, entropy);

    // Re-issuance of policyAsset is always unblinded
    CAsset reissuance;
    CalculateReissuanceToken(reissuance, entropy, false);

    // eventually use a structure as follows
    // std::map<CTxDestination, CAmountMap> genesis_values;
    // get list of addresses (maybe dump relevant blockchains) and convert , then extract destinations
    // based on blockchain share calculate asset amount

    // Note: Genesis block isn't actually validated, outputs are entered into utxo db only
    //genesis_block.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    //for (unsigned int i = 0; i < reissuance_outputs; i++) {
    //    txNew.vout.push_back(CTxOut(reissuance, CConfidentialValue(reissuance_values), issuance_destination));
    //}
    //genesis_block.hashMerkleRoot = BlockMerkleRoot(genesis_block);
}
