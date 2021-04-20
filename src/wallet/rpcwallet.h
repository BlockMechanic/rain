// Copyright (c) 2016-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_WALLET_RPCWALLET_H
#define RAIN_WALLET_RPCWALLET_H

#include <span.h>

#include <memory>
#include <string>
#include <vector>

class CRPCCommand;
class CWallet;
class JSONRPCRequest;
class LegacyScriptPubKeyMan;
class UniValue;
class CTransaction;
struct PartiallySignedTransaction;
struct WalletContext;
class CCoinControl;
struct CRecipient;
Span<const CRPCCommand> GetWalletRPCCommands();

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request);
typedef std::map<std::string, std::string> mapValue_t;
void EnsureWalletIsUnlocked(const CWallet*);
WalletContext& EnsureWalletContext(const util::Ref& context);
LegacyScriptPubKeyMan& EnsureLegacyScriptPubKeyMan(CWallet& wallet, bool also_create = false);
UniValue SendMoney(CWallet* const pwallet, CTransactionRef& tx, const CCoinControl &coin_control, std::vector<CRecipient> &recipients, mapValue_t map_value, bool verbose);
RPCHelpMan getaddressinfo();
RPCHelpMan signrawtransactionwithwallet();
void ParseCoinControlOptions(const UniValue &obj, CWallet *pwallet, CCoinControl &coin_control);

#endif //RAIN_WALLET_RPCWALLET_H
