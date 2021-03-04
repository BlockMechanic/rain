// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/client.h>
#include <rpc/protocol.h>
#include <util/system.h>

#include <set>
#include <stdint.h>

class CRPCConvertParam
{
public:
    std::string methodName; //!< method whose params want conversion
    int paramIdx;           //!< 0-based idx of param to convert
    std::string paramName;  //!< parameter name
};

// clang-format off
/**
 * Specify a (method, idx, name) here if the argument is a non-string RPC
 * argument and needs to be converted from JSON.
 *
 * @note Parameter indexes start from 0.
 */
static const CRPCConvertParam vRPCConvertParams[] =
{
    { "issue", 1, "qty" },
    { "issue", 4, "units" },
    { "issue", 5, "reissuable" },
    { "issue", 6, "has_ipfs" },
    { "issuerestrictedasset", 1, "qty" },
    { "issuerestrictedasset", 5, "units" },
    { "issuerestrictedasset", 6, "reissuable" },
    { "issuerestrictedasset", 7, "has_ipfs" },
    { "issuequalifierasset", 1,  "qty"},
    { "issuequalifierasset", 4, "has_ipfs" },
    { "reissuerestrictedasset", 1, "qty" },
    { "reissuerestrictedasset", 3, "change_verifier" },
    { "reissuerestrictedasset", 6, "new_unit" },
    { "reissuerestrictedasset", 7, "reissuable" },
    { "issueunique", 1, "asset_tags"},
    { "issueunique", 2, "ipfs_hashes"},
    { "transfer", 1, "qty"},
    { "transfer", 4, "expire_time"},
    { "transferfromaddress", 2, "qty"},
    { "transferfromaddress", 5, "expire_time"},
    { "transferfromaddresses", 1, "from_addresses"},
    { "transferfromaddresses", 2, "qty"},
    { "transferfromaddresses", 5, "expire_time"},
    { "transferqualifier", 2, "qty"},
    { "transferqualifier", 5, "expire_time"},
    { "reissue", 1, "qty"},
    { "reissue", 4, "reissuable"},
    { "reissue", 5, "new_unit"},
    { "listmyassets", 1, "verbose" },
    { "listmyassets", 2, "count" },
    { "listmyassets", 3, "start"},
    { "listmyassets", 4, "confs"},
    { "listassets", 1, "verbose" },
    { "listassets", 2, "count" },
    { "listassets", 3, "start" },
    { "setmocktime", 0, "timestamp" },
    { "utxoupdatepsbt", 1, "descriptors" },
    { "setgenerate", 0, "genproclimit" },
    { "generate", 0, "nblocks" },
    { "generate", 1, "maxtries" },
    { "generatetoaddress", 0, "nblocks" },
    { "generatetoaddress", 2, "maxtries" },
    { "getnetworkhashps", 0, "nblocks" },
    { "getnetworkhashps", 1, "height" },
    { "delegatestake", 1, "amount"},
    { "delegatestake", 3, "fExternalOwner"},
    { "delegatestake", 4, "fUseDelegated"},
    { "delegatestake", 5, "fForceNotEnabled"},
    { "rawdelegatestake", 1, "amount"},
    { "rawdelegatestake", 3, "fExternalOwner"},
    { "rawdelegatestake", 4, "fUseDelegated"},
    { "sendtoaddress", 1, "amount" },
    { "sendtoaddress", 4, "subtractfeefromamount" },
    { "sendtoaddress", 5 , "replaceable" },
    { "sendtoaddress", 6 , "conf_target" },
    { "sendtoaddress", 8, "avoid_reuse" },
    { "settxfee", 0, "amount" },
    { "sethdseed", 0, "newkeypool" },
    { "getsubsidy", 0, "height" },
    { "getreceivedbyaddress", 1, "minconf" },
    { "getreceivedbylabel", 1, "minconf" },
    { "listcoldutxos", 0, "true"},
    { "listdelegators", 0, "addr"},
    { "listreceivedbyaddress", 0, "minconf" },
    { "listreceivedbyaddress", 1, "include_empty" },
    { "listreceivedbyaddress", 2, "include_watchonly" },
    { "listreceivedbylabel", 0, "minconf" },
    { "listreceivedbylabel", 1, "include_empty" },
    { "listreceivedbylabel", 2, "include_watchonly" },
    { "getbalance", 1, "minconf" },
    { "getbalance", 2, "include_watchonly" },
    { "getbalance", 3, "avoid_reuse" },
    { "getblockhash", 0, "height" },
    { "getsuperblockbudget", 0, "index" },
    { "waitforblockheight", 0, "height" },
    { "waitforblockheight", 1, "timeout" },
    { "waitforblock", 1, "timeout" },
    { "waitfornewblock", 0, "timeout" },
    { "listtransactions", 1, "count" },
    { "listtransactions", 2, "skip" },
    { "listtransactions", 3, "include_watchonly" },
    { "walletpassphrase", 1, "timeout" },
    { "getblocktemplate", 0, "template_request" },
    { "listsinceblock", 1, "target_confirmations" },
    { "listsinceblock", 2, "include_watchonly" },
    { "listsinceblock", 3, "include_removed" },
    { "sendmany", 1, "amounts" },
    { "sendmany", 2, "minconf" },
    { "sendmany", 4, "subtractfeefrom" },
    { "sendmany", 5 , "replaceable" },
    { "sendmany", 6 , "conf_target" },
    { "deriveaddresses", 1, "range" },
    { "scantxoutset", 1, "scanobjects" },
    { "addmultisigaddress", 0, "nrequired" },
    { "addmultisigaddress", 1, "keys" },
    { "createmultisig", 0, "nrequired" },
    { "createmultisig", 1, "keys" },
    { "listunspent", 0, "minconf" },
    { "listunspent", 1, "maxconf" },
    { "listunspent", 2, "addresses" },
    { "listunspent", 3, "include_unsafe" },
    { "listunspent", 4, "query_options" },
    { "getblock", 1, "verbosity" },
    { "getblock", 1, "verbose" },
    { "getblockheader", 1, "verbose" },
    { "getchaintxstats", 0, "nblocks" },
    { "gettransaction", 1, "include_watchonly" },
    { "getrawtransaction", 1, "verbose" },
    { "createrawtransaction", 0, "inputs" },
    { "createrawtransaction", 1, "outputs" },
    { "createrawtransaction", 2, "locktime" },
    { "createrawtransaction", 3, "replaceable" },
    { "decoderawtransaction", 1, "iswitness" },
    { "signrawtransactionwithkey", 1, "privkeys" },
    { "signrawtransactionwithkey", 2, "prevtxs" },
    { "signrawtransactionwithwallet", 1, "prevtxs" },
    { "sendrawtransaction", 1, "allowhighfees" },
    { "sendrawtransaction", 1, "maxfeerate" },
    { "testmempoolaccept", 0, "rawtxs" },
    { "testmempoolaccept", 1, "allowhighfees" },
    { "testmempoolaccept", 1, "maxfeerate" },
    { "combinerawtransaction", 0, "txs" },
    { "fundrawtransaction", 1, "options" },
    { "fundrawtransaction", 2, "iswitness" },
    { "walletcreatefundedpsbt", 0, "inputs" },
    { "walletcreatefundedpsbt", 1, "outputs" },
    { "walletcreatefundedpsbt", 2, "locktime" },
    { "walletcreatefundedpsbt", 3, "options" },
    { "walletcreatefundedpsbt", 4, "bip32derivs" },
    { "walletprocesspsbt", 1, "sign" },
    { "walletprocesspsbt", 3, "bip32derivs" },
    { "createpsbt", 0, "inputs" },
    { "createpsbt", 1, "outputs" },
    { "createpsbt", 2, "locktime" },
    { "createpsbt", 3, "replaceable" },
    { "combinepsbt", 0, "txs"},
    { "joinpsbts", 0, "txs"},
    { "finalizepsbt", 1, "extract"},
    { "converttopsbt", 1, "permitsigdata"},
    { "converttopsbt", 2, "iswitness"},
    { "gettxout", 1, "n" },
    { "gettxout", 2, "include_mempool" },
    { "gettxoutproof", 0, "txids" },
    { "lockunspent", 0, "unlock" },
    { "lockunspent", 1, "transactions" },
    { "importprivkey", 2, "rescan" },
    { "importaddress", 2, "rescan" },
    { "importaddress", 3, "p2sh" },
    { "importpubkey", 2, "rescan" },
    { "importmulti", 0, "requests" },
    { "importmulti", 1, "options" },
    { "verifychain", 0, "checklevel" },
    { "verifychain", 1, "nblocks" },
    { "getblockstats", 0, "hash_or_height" },
    { "getblockstats", 1, "stats" },
    { "pruneblockchain", 0, "height" },
    { "keypoolrefill", 0, "newsize" },
    { "getrawmempool", 0, "verbose" },
    { "estimatesmartfee", 0, "conf_target" },
    { "estimaterawfee", 0, "conf_target" },
    { "estimaterawfee", 1, "threshold" },
    { "prioritisetransaction", 1, "dummy" },
    { "prioritisetransaction", 2, "fee_delta" },
    { "setban", 2, "bantime" },
    { "setban", 3, "absolute" },
    { "setstakesplitthreshold", 0, "amount"},
    { "autocombinerewards", 0, "true"},
    { "autocombinerewards", 1, "amount"},
    { "setnetworkactive", 0, "state" },
    { "setprivatesendrounds", 0, "rounds" },
    { "setprivatesendamount", 0, "amount" },
    { "setwalletflag", 1, "value" },
    { "getmempoolancestors", 1, "verbose" },
    { "getmempooldescendants", 1, "verbose" },
    { "bumpfee", 1, "options" },
    { "logging", 0, "include" },
    { "logging", 1, "exclude" },
    { "spork", 1, "value" },
    { "voteraw", 1, "tx_index" },
    { "voteraw", 5, "time" },
    { "getblockhashes", 0, "high"},
    { "getblockhashes", 1, "low" },
    { "getspentinfo", 0, "json" },
    { "getaddresstxids", 0, "addresses" },
    { "getaddressbalance", 0, "addresses" },
    { "getaddressdeltas", 0, "addresses" },
    { "getaddressutxos", 0, "addresses" },
    { "getaddressmempool", 0, "addresses" },
    { "getspecialtxes", 1, "type" },
    { "getspecialtxes", 2, "count" },
    { "getspecialtxes", 3, "skip" },
    { "getspecialtxes", 4, "verbosity" },
    { "disconnectnode", 1, "nodeid" },
    // Echo with conversion (For testing only)
    { "echojson", 0, "arg0" },
    { "echojson", 1, "arg1" },
    { "echojson", 2, "arg2" },
    { "echojson", 3, "arg3" },
    { "echojson", 4, "arg4" },
    { "echojson", 5, "arg5" },
    { "echojson", 6, "arg6" },
    { "echojson", 7, "arg7" },
    { "echojson", 8, "arg8" },
    { "echojson", 9, "arg9" },
    { "rescanblockchain", 0, "start_height"},
    { "rescanblockchain", 1, "stop_height"},
    { "listaddressesbyasset", 1, "totalonly"},
    { "listaddressesbyasset", 2, "count"},
    { "listaddressesbyasset", 3, "start"},
    { "listassetbalancesbyaddress", 1, "totalonly"},
    { "listassetbalancesbyaddress", 2, "count"},
    { "listassetbalancesbyaddress", 3, "start"},
    { "sendmessage", 2, "expire_time"},
    { "requestsnapshot", 1, "block_height"},
    { "getsnapshotrequest", 1, "block_height"},
    { "listsnapshotrequests", 1, "block_height"},
    { "cancelsnapshotrequest", 1, "block_height"},
    { "distributereward", 1, "snapshot_height"},
    { "distributereward", 3, "gross_distribution_amount"},
    { "getdistributestatus", 1, "snapshot_height"},
    { "getdistributestatus", 3, "gross_distribution_amount"},
    { "getsnapshot", 1, "block_height"},
    { "purgesnapshot", 1, "block_height"},
    { "createwallet", 1, "disable_private_keys"},
    { "createwallet", 2, "blank"},
    { "createwallet", 4, "avoid_reuse"},
    { "getnodeaddresses", 0, "count"},
    { "stop", 0, "wait" },
    { "dumpissuanceblindingkey", 1, "vin" },
    { "importissuanceblindingkey", 1, "vin" },
    { "rawissueasset", 1, "issuances" },
    { "rawreissueasset", 1, "reissuances" },
    { "issueasset", 0, "assetamount" },
    { "issueasset", 1, "tokenamount" },
    { "issueasset", 2, "blind" },
    { "reissueasset", 1, "assetamount" },
    { "rawblindrawtransaction", 1, "inputblinder" },
    { "rawblindrawtransaction", 2, "inputamount" },
    { "rawblindrawtransaction", 3, "inputasset" },
    { "rawblindrawtransaction", 4, "inputassetblinder" },
    { "rawblindrawtransaction", 6, "ignoreblindfail" },
    { "blindrawtransaction", 1, "ignoreblindfail" },
    { "blindrawtransaction", 2, "asset_commitments" },
    { "blindrawtransaction", 3, "blind_issuances" },
    { "destroyamount", 1, "amount" },
    { "spork", 1, "" },
    { "preparebudget", 2,"" },
    { "preparebudget", 3,"" },
    { "preparebudget", 5,"" },
    { "submitbudget", 2,"" },
    { "submitbudget", 3,"" },
    { "submitbudget", 5,"" },
    { "submitbudget", 7,"" },
    // TODO: remove this and switch over to proper arg parsing in rpc/masternode.cpp for the second argument
    //{"startmasternode", 1},
    { "startmasternode", 3,"" },
    { "mnvoteraw", 1,"" },
    { "mnvoteraw", 4,"" },
    { "setstakesplitthreshold", 0,"" },
    { "autocombinerewards", 0,"" },
    { "autocombinerewards", 1,"" },
    { "getsupplyinfo", 0,"" },
};
// clang-format on

class CRPCConvertTable
{
private:
    std::set<std::pair<std::string, int>> members;
    std::set<std::pair<std::string, std::string>> membersByName;

public:
    CRPCConvertTable();

    bool convert(const std::string& method, int idx) {
        return (members.count(std::make_pair(method, idx)) > 0);
    }
    bool convert(const std::string& method, const std::string& name) {
        return (membersByName.count(std::make_pair(method, name)) > 0);
    }
};

CRPCConvertTable::CRPCConvertTable()
{
    const unsigned int n_elem =
        (sizeof(vRPCConvertParams) / sizeof(vRPCConvertParams[0]));

    for (unsigned int i = 0; i < n_elem; i++) {
        members.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                      vRPCConvertParams[i].paramIdx));
        membersByName.insert(std::make_pair(vRPCConvertParams[i].methodName,
                                            vRPCConvertParams[i].paramName));
    }
}

static CRPCConvertTable rpcCvtTable;

/** Non-RFC4627 JSON parser, accepts internal values (such as numbers, true, false, null)
 * as well as objects and arrays.
 */
UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
    UniValue jVal;
    if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
        !jVal.isArray() || jVal.size()!=1)
        throw std::runtime_error(std::string("Error parsing JSON:")+strVal);
    return jVal[0];
}

UniValue RPCConvertValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VARR);

    for (unsigned int idx = 0; idx < strParams.size(); idx++) {
        const std::string& strVal = strParams[idx];

        if (!rpcCvtTable.convert(strMethod, idx)) {
            // insert string value directly
            params.push_back(strVal);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.push_back(ParseNonRFCJSONValue(strVal));
        }
    }

    return params;
}

UniValue RPCConvertNamedValues(const std::string &strMethod, const std::vector<std::string> &strParams)
{
    UniValue params(UniValue::VOBJ);

    for (const std::string &s: strParams) {
        size_t pos = s.find('=');
        if (pos == std::string::npos) {
            throw(std::runtime_error("No '=' in named argument '"+s+"', this needs to be present for every argument (even if it is empty)"));
        }

        std::string name = s.substr(0, pos);
        std::string value = s.substr(pos+1);

        if (!rpcCvtTable.convert(strMethod, name)) {
            // insert string value directly
            params.pushKV(name, value);
        } else {
            // parse string as JSON, insert bool/number/object/etc. value
            params.pushKV(name, ParseNonRFCJSONValue(value));
        }
    }

    return params;
}
