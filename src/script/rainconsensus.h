// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_SCRIPT_RAINCONSENSUS_H
#define RAIN_SCRIPT_RAINCONSENSUS_H

#include <stdint.h>

#if defined(BUILD_RAIN_INTERNAL) && defined(HAVE_CONFIG_H)
#include <config/rain-config.h>
  #if defined(_WIN32)
    #if defined(DLL_EXPORT)
      #if defined(HAVE_FUNC_ATTRIBUTE_DLLEXPORT)
        #define EXPORT_SYMBOL __declspec(dllexport)
      #else
        #define EXPORT_SYMBOL
      #endif
    #endif
  #elif defined(HAVE_FUNC_ATTRIBUTE_VISIBILITY)
    #define EXPORT_SYMBOL __attribute__ ((visibility ("default")))
  #endif
#elif defined(MSC_VER) && !defined(STATIC_LIBRAINCONSENSUS)
  #define EXPORT_SYMBOL __declspec(dllimport)
#endif

#ifndef EXPORT_SYMBOL
  #define EXPORT_SYMBOL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define RAINCONSENSUS_API_VER 1

typedef enum rainconsensus_error_t
{
    rainconsensus_ERR_OK = 0,
    rainconsensus_ERR_TX_INDEX,
    rainconsensus_ERR_TX_SIZE_MISMATCH,
    rainconsensus_ERR_TX_DESERIALIZE,
    rainconsensus_ERR_AMOUNT_REQUIRED,
    rainconsensus_ERR_INVALID_FLAGS,
} rainconsensus_error;

/** Script verification flags */
enum
{
    rainconsensus_SCRIPT_FLAGS_VERIFY_NONE                = 0,
    rainconsensus_SCRIPT_FLAGS_VERIFY_P2SH                = (1U << 0), // evaluate P2SH (BIP16) subscripts
    rainconsensus_SCRIPT_FLAGS_VERIFY_DERSIG              = (1U << 2), // enforce strict DER (BIP66) compliance
    rainconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY           = (1U << 4), // enforce NULLDUMMY (BIP147)
    rainconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY = (1U << 9), // enable CHECKLOCKTIMEVERIFY (BIP65)
    rainconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY = (1U << 10), // enable CHECKSEQUENCEVERIFY (BIP112)
    rainconsensus_SCRIPT_FLAGS_VERIFY_WITNESS             = (1U << 11), // enable WITNESS (BIP141)
    rainconsensus_SCRIPT_FLAGS_VERIFY_ALL                 = rainconsensus_SCRIPT_FLAGS_VERIFY_P2SH | rainconsensus_SCRIPT_FLAGS_VERIFY_DERSIG |
                                                               rainconsensus_SCRIPT_FLAGS_VERIFY_NULLDUMMY | rainconsensus_SCRIPT_FLAGS_VERIFY_CHECKLOCKTIMEVERIFY |
                                                               rainconsensus_SCRIPT_FLAGS_VERIFY_CHECKSEQUENCEVERIFY | rainconsensus_SCRIPT_FLAGS_VERIFY_WITNESS
};

/// Returns 1 if the input nIn of the serialized transaction pointed to by
/// txTo correctly spends the scriptPubKey pointed to by scriptPubKey under
/// the additional constraints specified by flags.
/// If not nullptr, err will contain an error/success code for the operation
EXPORT_SYMBOL int rainconsensus_verify_script(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                                 const unsigned char *txTo        , unsigned int txToLen,
                                                 unsigned int nIn, unsigned int flags, rainconsensus_error* err);

EXPORT_SYMBOL int rainconsensus_verify_script_with_amount(const unsigned char *scriptPubKey, unsigned int scriptPubKeyLen,
                                    const unsigned char *amount, unsigned int amountLen,
                                    const unsigned char *txTo        , unsigned int txToLen,
                                    unsigned int nIn, unsigned int flags, rainconsensus_error* err);

EXPORT_SYMBOL unsigned int rainconsensus_version();

#ifdef __cplusplus
} // extern "C"
#endif

#undef EXPORT_SYMBOL

#endif // RAIN_SCRIPT_RAINCONSENSUS_H
