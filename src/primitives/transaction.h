// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_PRIMITIVES_TRANSACTION_H
#define RAIN_PRIMITIVES_TRANSACTION_H

#include <stdint.h>
#include <amount.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>
#include <primitives/asset.h>
#include <primitives/confidential.h>
#include <primitives/txwitness.h>

static const int SERIALIZE_TRANSACTION_NO_WITNESS = 0x40000000;

/** Transaction types */
enum {
    TRANSACTION_NORMAL = 0,
    TRANSACTION_PROVIDER_REGISTER = 1,
    TRANSACTION_PROVIDER_UPDATE_SERVICE = 2,
    TRANSACTION_PROVIDER_UPDATE_REGISTRAR = 3,
    TRANSACTION_PROVIDER_UPDATE_REVOKE = 4,
    TRANSACTION_COINBASE = 5,
    TRANSACTION_QUORUM_COMMITMENT = 6,
    TRANSACTION_GOVERNANCE_VOTE = 7
};

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    /* If this flag is set, the CTxIn including this COutPoint has a
     * CAssetIssuance object. */
    static const uint32_t OUTPOINT_ISSUANCE_FLAG = (1 << 31);

    /* The inverse of the combination of the preceding flags. Used to
     * extract the original meaning of `n` as the index into the
     * transaction's output array. */
    static const uint32_t OUTPOINT_INDEX_MASK = 0x3fffffff;

    static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

    COutPoint(): n(NULL_INDEX) { }
    COutPoint(const uint256& hashIn, uint32_t nIn): hash(hashIn), n(nIn) { }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = NULL_INDEX; }
    bool IsNull() const { return (hash.IsNull() && n == NULL_INDEX); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.n < b.n);
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
    std::string ToStringShort() const;

    uint256 GetHash();
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;

    CAssetIssuance assetIssuance;

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1U << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    bool IsNull() const { return prevout.IsNull() && scriptSig.empty() && nSequence ==SEQUENCE_FINAL; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {

        bool fHasAssetIssuance;
        COutPoint outpoint;
        if (!ser_action.ForRead()) {
            if (prevout.n == (uint32_t) -1) {
                // Coinbase inputs do not have asset issuances attached
                // to them.
                fHasAssetIssuance = false;
                outpoint = prevout;
            } else {
                // The issuance and pegin bits can't be set as it is used to indicate
                // the presence of the asset issuance or pegin objects. They should
                // never be set anyway as that would require a parent
                // transaction with over one billion outputs.
                assert(!(prevout.n & ~COutPoint::OUTPOINT_INDEX_MASK));
                // The assetIssuance object is used to represent both new
                // asset generation and reissuance of existing asset types.
                fHasAssetIssuance = !assetIssuance.IsNull();
                // The mode is placed in the upper bits of the outpoint's
                // index field. The IssuanceMode enum values are chosen to
                // make this as simple as a bitwise-OR.
                outpoint.hash = prevout.hash;
                outpoint.n = prevout.n & COutPoint::OUTPOINT_INDEX_MASK;
                if (fHasAssetIssuance) {
                    outpoint.n |= COutPoint::OUTPOINT_ISSUANCE_FLAG;
                }
            }
        }

        READWRITE(outpoint);

        if (ser_action.ForRead()) {
            if (outpoint.n == (uint32_t) -1) {
                // No asset issuance for Coinbase inputs.
                fHasAssetIssuance = false;
                prevout = outpoint;
            } else {
                // The presence of the asset issuance object is indicated by
                // a bit set in the outpoint index field.
                fHasAssetIssuance = !!(outpoint.n & COutPoint::OUTPOINT_ISSUANCE_FLAG);
                // The mode, if set, must be masked out of the outpoint so
                // that the in-memory index field retains its traditional
                // meaning of identifying the index into the output array
                // of the previous transaction.
                prevout.hash = outpoint.hash;
                prevout.n = outpoint.n & COutPoint::OUTPOINT_INDEX_MASK;
            }
        }

        READWRITE(scriptSig);
        READWRITE(nSequence);

        // The asset fields are deserialized only if they are present.
        if (fHasAssetIssuance) {
            READWRITE(assetIssuance);
        } else if (ser_action.ForRead()) {
            assetIssuance.SetNull();
        }
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence     == b.nSequence &&
                a.assetIssuance == b.assetIssuance);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CConfidentialAsset nAsset;
    CConfidentialValue nValue;
    CConfidentialNonce nNonce;
    CScript scriptPubKey;
    int nRounds;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CConfidentialAsset& nAssetIn, const CConfidentialValue& nValueIn, CScript scriptPubKeyIn, int nRoundsIn = -10);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
            READWRITE(nAsset);
            READWRITE(nValue);
            READWRITE(nNonce);
            READWRITE(scriptPubKey);
    }

    void SetNull()
    {
        nAsset.SetNull();
        nValue.SetNull();
        nNonce.SetNull();
        scriptPubKey.clear();
        nRounds = -10; // an initial value, should be no way to get this by calculations
    }

    bool IsNull() const
    {
        return nAsset.IsNull() && nValue.IsNull() && nNonce.IsNull() && scriptPubKey.empty();
    }

    void SetEmpty()
    {
        nValue = 0;
        nNonce.SetNull();
        nAsset.SetNull();
        scriptPubKey.clear();
    }

    bool IsFee() const {
        return scriptPubKey == CScript()
            && nValue.IsExplicit() && nAsset.IsExplicit();
    }

    bool IsEmpty() const
    {
        return ((nValue.IsExplicit() && nValue.GetAmount() == 0 ) && scriptPubKey.empty());
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nAsset == b.nAsset &&
                a.nValue == b.nValue &&
                a.nNonce == b.nNonce &&
                a.scriptPubKey == b.scriptPubKey &&
                a.nRounds      == b.nRounds);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    uint256 GetHash() const;

    std::string ToString() const;

    bool IsMasternodeReward() const;
};

struct CMutableTransaction;

/**
 * Basic transaction serialization format:
 * - int32_t nVersion
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - uint32_t nLockTime
 *
 * Extended transaction serialization format:
 * - int32_t nVersion
 * - unsigned char dummy = 0x00
 * - unsigned char flags (!= 0)
 * - std::vector<CTxIn> vin
 * - std::vector<CTxOut> vout
 * - if (flags & 1):
 *   - CTxWitness wit;
 * - uint32_t nLockTime
 */
template<typename Stream, typename TxType>
inline void UnserializeTransaction(TxType& tx, Stream& s) {

    int32_t n32bitVersion = 0;
    s >> n32bitVersion;
    tx.nVersion = (int16_t) (n32bitVersion & 0xffff);
    tx.nType = (int16_t) ((n32bitVersion >> 16) & 0xffff);
    s >> tx.nTime;
    unsigned char flags = 0;
    tx.vin.clear();
    tx.vout.clear();
    tx.witness.SetNull();

    // Witness serialization is different between Elements and Core.
    // See code comments in SerializeTransaction for details about the differences.
    s >> flags;
    s >> tx.vin;
    s >> tx.vout;
    s >> tx.nLockTime;
    if (tx.nType != TRANSACTION_NORMAL) {
        s >> tx.vExtraPayload;
    }
//  s >> tx.strTxComment;
    if (flags & 1) {
        /* The witness flag is present. */
        flags ^= 1;
        const_cast<CTxWitness*>(&tx.witness)->vtxinwit.resize(tx.vin.size());
        const_cast<CTxWitness*>(&tx.witness)->vtxoutwit.resize(tx.vout.size());
        s >> tx.witness;
        if (!tx.HasWitness()) {
            /* It's illegal to encode witnesses when all witness stacks are empty. */
            throw std::ios_base::failure("Superfluous witness record");
        }
    }
    if (flags) {
        /* Unknown flag in the serialization */
        throw std::ios_base::failure("Unknown transaction optional data");
    }
}

template<typename Stream, typename TxType>
inline void SerializeTransaction(const TxType& tx, Stream& s) {
    const bool fAllowWitness = !(s.GetVersion() & SERIALIZE_TRANSACTION_NO_WITNESS);

    int32_t n32bitVersion = tx.nVersion | (tx.nType << 16);
    s << n32bitVersion;
    s << tx.nTime;
    // Consistency check
    assert(tx.witness.vtxinwit.size() <= tx.vin.size());
    assert(tx.witness.vtxoutwit.size() <= tx.vout.size());
    // Check whether witnesses need to be serialized.
    unsigned char flags = 0;
    if (fAllowWitness && tx.HasWitness()) {
        flags |= 1;
    }
    // Witness serialization is different between Elements and Core.
    // In Elements-style serialization, all normal data is serialized first and the
    // witnesses all in the end.
    s << flags;
    s << tx.vin;
    s << tx.vout;
    s << tx.nLockTime;
    if (tx.nType != TRANSACTION_NORMAL)
        s << tx.vExtraPayload;
//  s << tx.strTxComment;
    if (flags & 1) {
        const_cast<CTxWitness*>(&tx.witness)->vtxinwit.resize(tx.vin.size());
        const_cast<CTxWitness*>(&tx.witness)->vtxoutwit.resize(tx.vout.size());
        s << tx.witness;
    }
}


/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION = 1;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION=2;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int16_t nVersion;
    const uint32_t nTime;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const uint32_t nLockTime;
    const int16_t nType;
//    const std::vector<unsigned char> strTxComment;
    const std::vector<uint8_t> vExtraPayload; // only available for special transaction types
   // For elements we need to keep track of some extra state for script witness outside of vin
    const CTxWitness witness;

private:
    /** Memory only. */
    const uint256 hash;
    const uint256 m_witness_hash;

    uint256 ComputeHash() const;
    uint256 ComputeWitnessHash() const;

public:
    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    explicit CTransaction(const CMutableTransaction &tx);
    CTransaction(CMutableTransaction &&tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }

    /** This deserializing constructor is provided instead of an Unserialize method.
     *  Unserialize is not possible, since it would require overwriting const fields. */
    template <typename Stream>
    CTransaction(deserialize_type, Stream& s) : CTransaction(CMutableTransaction(deserialize, s)) {}

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const { return hash; }
    const uint256& GetWitnessHash() const { return m_witness_hash; };
    // ELEMENTS: the witness only hash used in elements witness roots
    uint256 GetWitnessOnlyHash() const;

    // Return sum of txouts.
    CAmountMap GetValueOutMap() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    /**
     * Get the total transaction size in bytes, including witness data.
     * "Total Size" defined in BIP141 and BIP144.
     * @return Total transaction size in bytes
     */
    unsigned int GetTotalSize() const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    bool IsCoinStake() const;
    bool CheckColdStake(const CScript& script) const;
    bool HasP2CSOutputs() const;

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        return !witness.IsNull();
    }
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int16_t nVersion;
    uint32_t nTime;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    uint32_t nLockTime;
    int16_t nType;
    std::vector<uint8_t> vExtraPayload; // only available for special transaction types
//    std::vector<unsigned char> strTxComment;
    // For elements we need to keep track of some extra state for script witness outside of vin
    CTxWitness witness;


    CMutableTransaction();
    explicit CMutableTransaction(const CTransaction& tx);

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        SerializeTransaction(*this, s);
    }


    template <typename Stream>
    inline void Unserialize(Stream& s) {
        UnserializeTransaction(*this, s);
    }

    template <typename Stream>
    CMutableTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;

    friend bool operator==(const CMutableTransaction& a, const CMutableTransaction& b)
    {
        return a.GetHash() == b.GetHash();
    }

    std::string ToString() const;

    bool HasWitness() const
    {
        return !witness.IsNull();
    }
    bool CheckColdStake(const CScript& script) const;
    bool IsCoinStake() const;
};

typedef std::shared_ptr<const CTransaction> CTransactionRef;
static inline CTransactionRef MakeTransactionRef() { return std::make_shared<const CTransaction>(); }
template <typename Tx> static inline CTransactionRef MakeTransactionRef(Tx&& txIn) { return std::make_shared<const CTransaction>(std::forward<Tx>(txIn)); }

/** Implementation of BIP69
 * https://github.com/bitcoin/bips/blob/master/bip-0069.mediawiki
 */
struct CompareInputBIP69
{
    inline bool operator()(const CTxIn& a, const CTxIn& b) const
    {
        if (a.prevout.hash == b.prevout.hash) return a.prevout.n < b.prevout.n;

        uint256 hasha = a.prevout.hash;
        uint256 hashb = b.prevout.hash;

        typedef std::reverse_iterator<const unsigned char*> rev_it;
        rev_it rita = rev_it(hasha.end());
        rev_it ritb = rev_it(hashb.end());

        return std::lexicographical_compare(rita, rita + hasha.size(), ritb, ritb + hashb.size());
    }
};

struct CompareOutputBIP69
{
    inline bool operator()(const CTxOut& a, const CTxOut& b) const
    {
        return a.nValue.GetAmount() < b.nValue.GetAmount() || (a.nValue.GetAmount() == b.nValue.GetAmount() && a.scriptPubKey < b.scriptPubKey);
    }
};

#endif // RAIN_PRIMITIVES_TRANSACTION_H
