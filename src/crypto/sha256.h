// Copyright (c) 2014-2018 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_CRYPTO_SHA256_H
#define RAIN_CRYPTO_SHA256_H

#include <stdint.h>
#include <stdlib.h>
#include <string>

/** A hasher class for SHA-256. */
class CSHA256
{
private:
    uint32_t s[8];
    unsigned char buf[64];
    uint64_t bytes;

public:
    static const size_t OUTPUT_SIZE = 32;

    CSHA256();
    CSHA256& Write(const unsigned char* data, size_t len);
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    //TODO: Midstate is a hack'ish speedup that probably should make way for something
    //akin to the SHA256D64 speedups
    void Midstate(unsigned char hash[OUTPUT_SIZE], uint64_t* len, unsigned char *buffer);
    CSHA256& Reset();
};

/** Autodetect the best available SHA256 implementation.
 *  Returns the name of the implementation.
 */
std::string SHA256AutoDetect();

/** Compute multiple double-SHA256's of 64-byte blobs.
 *  output:  pointer to a blocks*32 byte output buffer
 *  input:   pointer to a blocks*64 byte input buffer
 *  blocks:  the number of hashes to compute.
 */
void SHA256D64(unsigned char* output, const unsigned char* input, size_t blocks);

#endif // RAIN_CRYPTO_SHA256_H
