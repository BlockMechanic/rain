
#include <pubkey.h>
#include <core_io.h>
#include <key_io.h>
#include <logging.h>

#include <rpc/util.h>
#include <rpc/server.h>
#include <script/miniscript.h>
#include <script/compiler.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/standard.h>

#include <crypto/sha1.h>

#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#include <util/translation.h>
#include <univalue.h>
#include <boost/algorithm/string.hpp>

using miniscript::operator"" _mst;

void replacekeys(std::string& structure, std::string& keysstring){
    boost::replace_all(structure, "\n", " ");
    boost::replace_all(structure, "     ", " ");
    boost::replace_all(structure, "   ", " ");
    boost::replace_all(structure, "  ", " ");
    boost::replace_all(structure, "<", "");
    boost::replace_all(structure, ">", "");

    std::vector<std::string> strsr;
    boost::split(strsr, keysstring, boost::is_any_of(","));

    std::set<CTxDestination> destinations;
    for (auto &p: strsr){
        std::vector<std::string> vv;
        boost::split(vv, p, boost::is_any_of(":"));
        std::string address = vv[1];

        CTxDestination dest = DecodeDestination(address);
        if (!IsValidDestination(dest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Rain address: ") + address);
        }

        if (destinations.count(dest)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated address: ") + address);
        }
        destinations.insert(dest);
        std::string placeholder = vv[0];
        boost::replace_all(structure, placeholder, ScriptToAsmStr(GetScriptForDestination(dest)));
        boost::replace_all(structure, "(", " ");
        boost::replace_all(structure, ")", "");
    }
}

UniValue compileAnalyze(std::string &line, std::string &keysstring){

    UniValue result(UniValue::VOBJ);

    miniscript::NodeRef<std::string> ret;
    double avgcost = 0;
    CScript script;
    std::string structure ="";
    if (Compile(Expand(line), ret, avgcost)) {
        std::string str;
        ret->ToString(COMPILER_CTX, str);
        result.pushKV("Script", std::to_string(ret->ScriptSize())+ " WU");
        result.pushKV("Input", std::to_string(avgcost)+ " WU");
        result.pushKV("Total", std::to_string(ret->ScriptSize() + avgcost)+ " WU");
        structure = Disassemble(ret->ToScript(COMPILER_CTX));
        script = ret->ToScript(COMPILER_CTX);
    } else if ((ret = miniscript::FromString(Expand(line), COMPILER_CTX))) {
        result.pushKV("scriptlen", (int)ret->ScriptSize());
        result.pushKV("safe", ret->GetType() << "s"_mst ? "yes" : "no");
        result.pushKV("non-malleable ", ret->GetType() << "m"_mst ? "yes" : "no");
        structure = Disassemble(ret->ToScript(COMPILER_CTX));
        script = ret->ToScript(COMPILER_CTX);
    } else {
        result.pushKV("Error", strprintf("Failed to parse as policy or miniscript %s",line.c_str()));
        return result;
    }

    replacekeys(structure, keysstring);

    std::vector<std::string> strs;
    boost::split(strs, structure, boost::is_any_of(" "));
    CScript testscript;
    for(auto & stmt : strs){
        if(stmt !=" "){
            opcodetype op = GetOpCode(stmt);
            if(GetOpName(op) == "OP_UNKNOWN")
                testscript << ParseHex(stmt);
            else
                testscript << op;
        }
    }

    UniValue r(UniValue::VOBJ);

    ScriptPubKeyToUniv(testscript, r, true);

    UniValue type;
    type = find_value(r, "type");

    if (type.isStr() && type.get_str() != "scripthash") {
        // P2SH cannot be wrapped in a P2SH. If this script is already a P2SH,
        // don't return the address for a P2SH of the P2SH.
        r.pushKV("p2sh", EncodeDestination(ScriptHash(script)));
        // P2SH and witness programs cannot be wrapped in P2WSH, if this script
        // is a witness program, don't return addresses for a segwit programs.
        if (type.get_str() == "pubkey" || type.get_str() == "pubkeyhash" || type.get_str() == "multisig" || type.get_str() == "nonstandard") {
            std::vector<std::vector<unsigned char>> solutions_data;
            TxoutType which_type = Solver(script, solutions_data);
            // Uncompressed pubkeys cannot be used with segwit checksigs.
            // If the script contains an uncompressed pubkey, skip encoding of a segwit program.
            if ((which_type == TxoutType::PUBKEY) || (which_type == TxoutType::MULTISIG)) {
                for (const auto& solution : solutions_data) {
                    if ((solution.size() != 1) && !CPubKey(solution).IsCompressed()) {
                        return r;
                    }
                }
            }
            UniValue sr(UniValue::VOBJ);
            CScript segwitScr;
            if (which_type == TxoutType::PUBKEY) {
                segwitScr = GetScriptForDestination(WitnessV0KeyHash(Hash160(solutions_data[0])));
            } else if (which_type == TxoutType::PUBKEYHASH) {
                segwitScr = GetScriptForDestination(WitnessV0KeyHash(uint160{solutions_data[0]}));
            } else {
                // Scripts that are not fit for P2WPKH are encoded as P2WSH.
                // Newer segwit program versions should be considered when then become available.
                segwitScr = GetScriptForDestination(WitnessV0ScriptHash(script));
            }
            ScriptPubKeyToUniv(segwitScr, sr, /* fIncludeHex */ true);
            sr.pushKV("p2sh-segwit", EncodeDestination(ScriptHash(segwitScr)));
            r.pushKV("segwit", sr);
        }
    }

    result.pushKV("Structure", structure);
    result.pushKV("script_pub_key", HexStr(testscript));
    result.pushKV("Address Info", r);
    return result;
}

static RPCHelpMan minscript()
{
    return RPCHelpMan{"minscript",
                "\nParse and Analyze a miniscript expression or compile \n"
                "a policy statement into a miniscript expression.\n",
                {
                    {"miniscript/policy", RPCArg::Type::STR, RPCArg::Optional::NO, "The statement to analyse"},
                    {"addresses", RPCArg::Type::STR, RPCArg::Optional::NO, "addresses"},
                },
                RPCResult{
                    RPCResult::Type::STR, "Result", "Structure, Cost"
                },
                RPCExamples{
                    HelpExampleCli("minscript", " expression {key_1:" + EXAMPLE_ADDRESS[0] + ",key_2:" + EXAMPLE_ADDRESS[1] + "}")
            + HelpExampleRpc("minscript", "expression {\" key_1:" + EXAMPLE_ADDRESS[0] + ",\"key_2:" + EXAMPLE_ADDRESS[1] + "\"}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string line ="";
    if (!request.params[0].isNull()) {
        line = request.params[0].get_str();
        if (line.size() && line.back() == '\n') line.pop_back();
        if (line.size() == 0) return false;
    }

    std::string keysstring = request.params[1].get_str();

    return compileAnalyze(line, keysstring);
},
    };
}

static RPCHelpMan analyzescript()
{
    return RPCHelpMan{"analyzescript",
                "\nParse and Analyze a script expression\n"
                "returns a miniscript expression\n",
                {
                    {"script", RPCArg::Type::STR, RPCArg::Optional::NO, "The script to analyse"},
                },
                RPCResult{
                    RPCResult::Type::STR, "Result", "Structure, Cost"
                },
                RPCExamples{
                    HelpExampleCli("minscript", "script expression")
            + HelpExampleRpc("minscript", " script expression ")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    UniValue result(UniValue::VOBJ);

    std::string line ="";
    if (!request.params[0].isNull()) {
        line = request.params[0].get_str();
        if (line.size() && line.back() == '\n') line.pop_back();
        if (line.size() == 0) return false;
    }

    std::vector<std::string> strs;
    boost::split(strs, line, boost::is_any_of(" "));
    CScript testscript;
    for(auto & stmt : strs){
        if(stmt !=" "){
            opcodetype op = GetOpCode(stmt);
            if(GetOpName(op) == "OP_UNKNOWN")
                testscript << ParseHex(stmt);
            else
                testscript << op;
        }
    }

    miniscript::NodeRef<std::string> ret;

    CScript script;
    std::string structure ="";
    if ((ret = miniscript::FromScript(testscript, COMPILER_CTX))) {
        result.pushKV("scriptlen", (int)ret->ScriptSize());
        result.pushKV("safe", ret->GetType() << "s"_mst ? "yes" : "no");
        result.pushKV("non-malleable ", ret->GetType() << "m"_mst ? "yes" : "no");
        structure = Disassemble(ret->ToScript(COMPILER_CTX));
        script = ret->ToScript(COMPILER_CTX);
    } else {
        result.pushKV("Error", strprintf("Failed to parse as policy or miniscript %s",line.c_str()));
        return result;
    }
    result.pushKV("Structure", structure);

//    UniValue r(UniValue::VOBJ);

//    ScriptPubKeyToUniv(testscript, r, true);

    return result;
},
    };
}

static RPCHelpMan createescrowscript()
{
    return RPCHelpMan{"createescrowscript",
               "creates a script for use in escrow (no time lock)\n",
                {
                    {"referee", RPCArg::Type::STR, RPCArg::Optional::NO, "referee' public key"},
                    {"data", RPCArg::Type::ARR, RPCArg::Optional::NO, "The participant data",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "party message"},
                                    {"pubkey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "party pubkey"},
                                },
                            },
                        },
                    },
                },
                RPCResult{
                    RPCResult::Type::STR, "Result", "Script, Details"
                },
                RPCExamples{
            "\nImport the wallet\n"
            + HelpExampleCli("createescrowscript", "\"referee pub_key\" \"data\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("createescrowscript", "\"referee pub_key\" \"data\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!IsHex(request.params[0].get_str()))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey must be a hex string");
    std::vector<unsigned char> data(ParseHex(request.params[0].get_str()));
    CPubKey pubKey(data.begin(), data.end());
    if (!pubKey.IsFullyValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");

    std::string datastring = request.params[1].get_str();

    std::vector<std::pair<std::string,CPubKey> > parties;

    std::vector<std::string> strsr;
    boost::split(strsr, datastring, boost::is_any_of(","));
    for (auto &p: strsr){
        std::vector<std::string> vv;
        boost::split(vv, p, boost::is_any_of(":"));
        std::string message = vv[0];
        std::string pubkeystring = vv[1];

		if (!IsHex(pubkeystring))
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey must be a hex string");
		std::vector<unsigned char> pdata(ParseHex(pubkeystring));
		CPubKey upubKey(pdata.begin(), pdata.end());
		if (!upubKey.IsFullyValid())
			throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");
		std::pair mnp{message,upubKey};
		parties.push_back(mnp);
	}

    CScript outputScript = GetScriptForP2ES(parties, pubKey);
    
    UniValue r(UniValue::VOBJ);

    ScriptPubKeyToUniv(outputScript, r, true);

    UniValue type;
    type = find_value(r, "type");

    if (type.isStr() && type.get_str() != "scripthash") {
        // P2SH cannot be wrapped in a P2SH. If this script is already a P2SH,
        // don't return the address for a P2SH of the P2SH.
        r.pushKV("p2sh", EncodeDestination(ScriptHash(outputScript)));
        // P2SH and witness programs cannot be wrapped in P2WSH, if this script
        // is a witness program, don't return addresses for a segwit programs.
        if (type.get_str() == "pubkey" || type.get_str() == "pubkeyhash" || type.get_str() == "multisig" || type.get_str() == "nonstandard") {
            std::vector<std::vector<unsigned char>> solutions_data;
            TxoutType which_type = Solver(outputScript, solutions_data);
            // Uncompressed pubkeys cannot be used with segwit checksigs.
            // If the script contains an uncompressed pubkey, skip encoding of a segwit program.
            if ((which_type == TxoutType::PUBKEY) || (which_type == TxoutType::MULTISIG)) {
                for (const auto& solution : solutions_data) {
                    if ((solution.size() != 1) && !CPubKey(solution).IsCompressed()) {
                        return r;
                    }
                }
            }
            UniValue sr(UniValue::VOBJ);
            CScript segwitScr;
            if (which_type == TxoutType::PUBKEY) {
                segwitScr = GetScriptForDestination(WitnessV0KeyHash(Hash160(solutions_data[0])));
            } else if (which_type == TxoutType::PUBKEYHASH) {
                segwitScr = GetScriptForDestination(WitnessV0KeyHash(uint160{solutions_data[0]}));
            } else {
                // Scripts that are not fit for P2WPKH are encoded as P2WSH.
                // Newer segwit program versions should be considered when then become available.
                segwitScr = GetScriptForDestination(WitnessV0ScriptHash(outputScript));
            }
            ScriptPubKeyToUniv(segwitScr, sr, /* fIncludeHex */ true);
            sr.pushKV("p2sh-segwit", EncodeDestination(ScriptHash(segwitScr)));
            r.pushKV("segwit", sr);
        }
    }
    
    return r;    

},
    };
}

static RPCHelpMan hashmessage()
{
    return RPCHelpMan{"hashmessage",
                "\nSign a message with the private key of an address\n",
                {
                    {"hashtype", RPCArg::Type::STR, RPCArg::Optional::NO, "The type of hash required. sha256, hash256, hash160, ripemd160, sha1."},
                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message to hash."},
                },
                RPCResult{
                    RPCResult::Type::STR, "signature", "The signature of the message encoded in base 64"
                },
                RPCExamples{
            "\nCreate the signature\n"
            + HelpExampleCli("hashmessage", "\"hash type\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("hashmessage", "\"hash type\", \"my message\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string hashtype = request.params[0].get_str();
    std::string svch = request.params[1].get_str();
    valtype vch(svch.begin(), svch.end());

	valtype vchHash((hashtype == "ripemd160" || hashtype == "sha1" || hashtype == "hash160") ? 20 : 32);
	if (hashtype == "ripemd160")
		CRIPEMD160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
	else if (hashtype == "sha1")
		CSHA1().Write(vch.data(), vch.size()).Finalize(vchHash.data());
	else if (hashtype == "sha256")
		CSHA256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
	else if (hashtype == "hash160")
		CHash160().Write(vch).Finalize(vchHash);
	else if (hashtype =="hash256")
		CHash256().Write(vch).Finalize(vchHash);

/*
    if(hashtype == "sha256" || hashtype =="hash256"){
		CHashWriter ss(SER_GETHASH, 0);
		ss << strMessage;
		if(hashtype =="sha256")
			return ss.GetSHA256();
		if(hashtype =="hash256")
			return ss.GetHash();
	}

    if(hashtype =="hash160"){
        CHash160().Write(vch).Finalize(vchHash);
	}
*/
    std::string hash(vchHash.begin(), vchHash.end());

    return hash;
},
    };
}

static RPCHelpMan registerchainid()
{
    return RPCHelpMan{"registerchainid",
                "\n Register a new ChainID on the network\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to register"},
                    {"alias", RPCArg::Type::STR, RPCArg::Optional::NO, "The alias of this ID."},
                    {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "Entity name (Either company name, personal etc )."},
                    {"email", RPCArg::Type::STR, RPCArg::Optional::NO, "contact email address"},
                },
                RPCResult{
                    RPCResult::Type::STR, "signature", "The signature of the message encoded in base 64"
                },
                RPCExamples{
            "\nCreate the signature\n"
            + HelpExampleCli("registerchainid", "\"hash type\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("registerchainid", "\"hash type\", \"my message\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;
    CWallet* const pwallet = wallet.get();

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*wallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    EnsureWalletIsUnlocked(pwallet);

    std::string strAddress = request.params[0].get_str();
    std::string alias = request.params[1].get_str();
    std::string name = request.params[2].get_str();
    std::string email = request.params[3].get_str();

    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Rain address");
    }
    auto keyid = GetKeyForDestination(spk_man, dest);
    if (keyid.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    }

    CPubKey cpkOut;
    if (spk_man.GetPubKey(keyid, cpkOut) && !cpkOut.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "unable to retrieve public key");
    }

    CKey key;
    if (!spk_man.GetKey(keyid, key)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    }

    int nMinDepth = 1;
    int nMaxDepth = 9999999;
    bool include_unsafe = true;
    CAmount nMinimumAmount = 10.0001 * COIN;
    CAmount nMaximumAmount = MAX_MONEY;
    CAmount nMinimumSumAmount = MAX_MONEY;
    uint64_t nMaximumCount = 1;
    CAsset asset = pwallet->chain().getAsset("Rain");

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    UniValue results(UniValue::VARR);
    std::vector<COutput> vecOutputs;
    {
        CCoinControl cctl;
        cctl.m_avoid_address_reuse = false;
        cctl.m_min_depth = nMinDepth;
        cctl.m_max_depth = nMaxDepth;
        LOCK(pwallet->cs_wallet);
        pwallet->AvailableCoins(vecOutputs, !include_unsafe, &cctl, nMinimumAmount, nMaximumAmount, nMinimumSumAmount, nMaximumCount);
    }

    LOCK(pwallet->cs_wallet);
    CCoinControl coin_control;

    for (const COutput& out : vecOutputs) {
        CTxDestination address;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);

        if(std::get<WitnessV0KeyHash>(dest) != std::get<WitnessV0KeyHash>(address))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Rain address");

        if (!fValidAddress)
            continue;

        // Elements
        CAmount amount = out.tx->tx->vout[out.i].nValue.GetAmount();
        CAsset assetid = out.tx->tx->vout[out.i].nAsset.GetAsset();
        // Only list known outputs that match optional filter
        if ((amount < 0 || assetid.IsNull())) {
            wallet->WalletLogPrintf("Unable to unblind output: %s:%d\n", out.tx->tx->GetHash().GetHex(), out.i);
            continue;
        }
        if (asset != assetid) {
            continue;
        }
        uint32_t nSequence = CTxIn::SEQUENCE_FINAL;
        coin_control.Select(COutPoint(out.tx->GetHash(), out.i));
    }

    CAmount nAmount = 10.0001 * COIN;
    mapValue_t mapValue;

    coin_control.m_add_inputs = false;
    std::vector<CRecipient> recipients;

    CRecipient recipient = {GetScriptForDestination(dest), nAmount, asset, CPubKey(), false};
    recipients.push_back(recipient);

    CMutableTransaction txNew;
    txNew.data.chainid.alias = alias;
    txNew.data.chainid.name = name;
    txNew.data.chainid.email = email;
    txNew.data.chainid.pubKey = cpkOut;

    bool ret =  key.Sign(txNew.data.chainid.GetHashWithoutSign(), txNew.data.chainid.vchIDSignature);

    if(!ret)
        throw JSONRPCError(RPC_MISC_ERROR, "Unable to sign ID");

    CTransactionRef tx(MakeTransactionRef(txNew));
    UniValue fundingtx = SendMoney(pwallet, tx, coin_control, recipients, std::move(mapValue), true);

    UniValue r(UniValue::VOBJ);
    r.pushKV("txid", r);

    return r;
},
    };
}


void RegisterContractRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- ------------------------
    { "contracts",           &minscript,                  },
    { "contracts",           &createescrowscript,         },
    { "contracts",           &analyzescript,              },
    { "contracts",           &hashmessage,                },
    { "contracts",           &registerchainid,            },
    
};

// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
