// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>
#include <groestl.h>
#include <hash.h>
#include <tinyformat.h>
#include <crypto/common.h>
#include <util/strencodings.h>

uint256 CBlockHeader::GetHashWithoutSign() const
{
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << nVersion << hashPrevBlock << hashMerkleRoot << nTime << nHeight << nBits << nNonce << prevoutStake;
    return ss.GetHash();
}

uint256 CBlockHeader::GetGroestlHash() const
{
    CGroestlHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << *this;
    return ss.GetHash();
}

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

std::string CBlockHeader::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlockHeader(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nHeight=%d, nBits=%08x, nNonce=%u, prevoutStake=%s, vchBlockSig=%s)\n",
        GetHash().ToString(), nVersion, hashPrevBlock.ToString(), hashMerkleRoot.ToString(), nTime, nHeight, nBits, nNonce, prevoutStake.ToString(), HexStr(vchBlockSig));
    return s.str();
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nHeight=%d, nBits=%08x, nNonce=%u)",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nHeight, nBits, nNonce);
        s << strprintf(" vtx=%u\n", vtx.size());
        s << strprintf("(prevoutStake=%s)\n", prevoutStake.ToString());
        std::string type = "Proof-of-work";
        if (IsProofOfStake())
            type="Proof-of-stake";
        s << strprintf("(vchBlockSig=%s, proof=%s)\n", HexStr(vchBlockSig), type);
    for (const auto& tx : vtx) {
        s << tx->ToString() << std::endl;
    }
    return s.str();
}
