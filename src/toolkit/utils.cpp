#include <iostream>
#include <stdio.h>
#include <string>
#include <ctype.h>
#include <assert.h>

#include <script/miniscript.h>

#include <script/compiler.h>

#include <toolkit/utils.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif
#include <string>

//maybe start with policy templates 
// eg 1 & time/block
// 2/3
 

std::string htlc(std::vector<std::string> & keys){

    fn winner($ceo, $players, $managers, $hash, $expiry, $hash_fn) {
        $redeem = (any($players) && $hash_fn($hash)) && older($expiry);
        $federation = 4 of $managers;
        $refund = ($ceo && older(65535)) || ($ceo && 2 of $managers) || $federation;

        9@$redeem || $refund
    }

    $managersArr = [ pk(A), pk(B), pk(C), pk(D), pk(E) ];
    $playersArr = [ pk(F), pk(G), pk(H) ];
    $timeout = (90 days);
    $ceoAddr = pk(I);

    winner($ceoAddr, $playersArr, $managersArr, J, $timeout, sha256)
}

bool run(std::string& line, int64_t count) {


}
/*

void PublickeyFromString(const std::string &ks, CPubKey &out, CWallet * pwallet)
{
#ifdef ENABLE_WALLET
    // Case 1: Bitcoin address and we have full public key:
    CTxDestination dest = DecodeDestination(ks);
    if (pwallet && IsValidDestination(dest))
    {
        CKeyID keyID = GetKeyForDestination(*pwallet, dest);
        if (keyID.IsNull())
            throw std::runtime_error(strprintf("%s does not refer to a key",ks));
        CPubKey vchPubKey;
        if (pwallet->GetPubKey(keyID, vchPubKey))
            throw std::runtime_error(strprintf("no full public key for address %s",ks));
        if (!vchPubKey.IsFullyValid())
            throw std::runtime_error(" Invalid public key: "+ks);
        out = vchPubKey;
    }

    // Case 2: hex public key
    else
#endif
    if (IsHex(ks))
    {
        CPubKey vchPubKey(ParseHex(ks));
        if (!vchPubKey.IsFullyValid())
            throw std::runtime_error(" Invalid public key: "+ks);
        out = vchPubKey;
    }
    else
    {
        throw std::runtime_error(" Invalid public key: "+ks);
    }
}

/**
 * Used by addmultisigaddress / createmultisig:
 */
CScript Createmultisig_redeemScript(const UniValue& params, const UniValue& options, CWallet * pwallet)
{
    int nRequired = params[0].get_int();
    const UniValue& keys = params[1].get_array();

    int64_t cltv_height = 0, cltv_time = 0;
    if (!options.isNull()) {
        std::vector<std::string> keys = options.getKeys();
        for(const std::string& key: keys) {
            const UniValue& val = options[key];
            if (key == "cltv_height" && val.isNum()) {
                cltv_height = val.get_int64();
            } else
            if (key == "cltv_time" && val.isNum()) {
                cltv_time = val.get_int64();
            } else {
                throw std::runtime_error(strprintf("unknown key/type for option '%s'", key));
            }
        }
    }

    // Gather public keys
    if (nRequired < 1)
        throw std::runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw std::runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    if (keys.size() > 16)
        throw std::runtime_error("Number of addresses involved in the multisignature address creation > 16\nReduce the number");
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        CPubKey vchPubKey;
        _publickey_from_string(keys[i].get_str(), vchPubKey, pwallet);
        pubkeys[i] = vchPubKey;
    }
    CScript result = GetScriptForMultisig(nRequired, pubkeys, cltv_height, cltv_time);

    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE)
        throw std::runtime_error(
                strprintf("redeemScript exceeds size limit: %d > %d", result.size(), MAX_SCRIPT_ELEMENT_SIZE));

    return result;
}

bool CreateMNEscrow(CPubKey buyerkey, CPubkey sellerkey, CPubKey MNkey, ){
    // 2-of-3 escrow contract
    //thresh(2,pk(buyer),pk(seller),pk(arbiter))
    
    wsh(multi(2,03829e91bb8d4df87fea147f98ef5d3e71c7c26204a5ed5de2d1d966938d017ac2,0215152236dd9f518dd2bba50487857b98bdb4778c3618780a25a0cbc660092185,0203bc5458e2b77b5f5a68a738a57bee0271a27e603100c4110533bf8811c19e2e))

}


std::string mofn(std::map<std::string,std::pair<int, std::string> > & keys, int required, int64_t delay){

    // supports M of N optional htlc
    // Useful for escrow , timed contracts, allows addition of random hash/number for entropy

    std::string description = strprintf("%d of %d", required,keys.size());
    // construct a M of N
    std::string base;

    std::string base = "or(";

    for(auto&a:keys){
        if(a.second.first > 0)
            base+=a.second.first + "@";
        base +="pk("+a.first+"),";
    }
    base.replace(base.find("),)"), sizeof("),)") - 1, "))");

    // add minimum key req
    if (required > 0)
        base ="thresh("+required+","+base+")";
    // optionally add a lock time
    if (delay > 0)
        base = "and(" + base + "," + (duration > 65534 "after" : "older") + "(" + duration + "))"

    return base;
}

bool CreateEntropyTransaction (CMutableTransaction &tx, std::vector<std::string> addresses){

    CTxDestination dest;
    std::string error;
    if (!pwallet->GetNewDestination(output_type, label, dest, error)) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, error);
    }

    CTxDestination dest = DecodeDestination(strAddress); //Get Dest from string address
    auto keyid = GetKeyForDestination(*pwallet, dest); // Get CKeyID from dest
    //CKey key = DecodeSecret(str); //Get Ckey from privkey string
    CKey key;
    if (!pwallet->GetKey(key, vchSecret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    }

    if (!key.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");
    }
    CPubKey pubkey = key.GetPubKey(); //Get CPubKey from CKey
    //pubkey.GetID(); // Get CKeyID from CPubkey
}


fn finalized($ceo, $investors, $managers, $hash, $expiry, $hash_fn) {
$redeem = (any($investors) && $hash_fn($hash)) && older($expiry);
$federation = 3 of $managersArr;
$refund = ($ceo && older(65535)) || $federation;

return 9@$redeem || $refund
}

$managersArr = [ pk(A), pk(B), pk(C), pk(D), pk(E) ];
$investorsArr = [ pk(F), pk(G), pk(J) ];
$timeout = (10000 blocks);
$ceoAddr = pk(K);

finalized($ceoAddr, $investorsArr, $managersArr, H, $timeout, sha256)

//////////////////////////////////////////////////////////////////////

Let’s ignore the data uniquification detail and create a binary option script:
First, note that the spend script will set up the stack like this:
oracle signature
oracle data
winner's public key (as in normal p2pkh transactions)
winner's transaction signature (as in normal p2pkh transactions)


//////////////////////////////////////////////////////////////////////////////
//Decaying multisig 
and(thresh(1,thresh(5,pk(key_1),pk(key_2),pk(key_3),pk(key_4),pk(key_5)),thresh(4,pk(key_1),pk(key_2),pk(key_3),pk(key_4),pk(key_5),older(10)),thresh(3,pk(key_1),pk(key_2),pk(key_3),pk(key_4),pk(key_5),older(100)),thresh(2,pk(key_1),pk(key_2),pk(key_3),pk(key_4),pk(key_5),older(1000)),and(thresh(1,pk(key_1),pk(key_2),pk(key_3),pk(key_4),pk(key_5)),older(10000))), sha256(H))


./src/rain-cli minscript "and_v(v:sha256(H),or_i(multi(5,key_1,key_2,key_3,key_4,key_5),or_i(thresh(4,pkh(key_1),a:pkh(key_2),a:pkh(key_3),a:pkh(key_4),a:pkh(key_5),sdv:older(10)),or_i(thresh(3,pkh(key_1),a:pkh(key_2),a:pkh(key_3),a:pkh(key_4),a:pkh(key_5),sdv:older(100)),or_i(thresh(2,pkh(key_1),a:pkh(key_2),a:pkh(key_3),a:pkh(key_4),a:pkh(key_5),sdv:older(1000)),and_v(vc:or_i(pk_h(key_1),or_i(pk_h(key_2),or_i(pk_h(key_3),or_i(pk_h(key_4),pk_h(key_5))))),older(10000)))))))" "key_1:hms1qapcpg4d0m022sj5sf00lanzaqrggvlry6kfag8,key_2:hms1qx75x8rvwjs2wdz6adp2gjrz8m7upfp3pmj0m4s,key_3:hms1qxh5tecsvuv957x6xvvvmmzg2z028dgaafxl523,key_4:hms1qgvn0al9lku8wpud27trcw9t2elnqaspasuua7r,key_5:hms1qm7642jds438zfcvsljrs2qnpa643uzy3vns06u"



thresh(1,and(pk(029ffbe722b147f3035c87cb1c60b9a5947dd49c774cc31e94773478711a929ac0),sha256(01ba4719c80b6fe911b091a7c05124b64eeece964e09c058ef8f9805daca546b)),and(pk(025f05815e3a1a8a83bfbb03ce016c9a2ee31066b98f567f6227df1d76ec4bd143),sha256(01ba4719c80b6fe911b091a7c05124b64eeece964e09c058ef8f9805daca546b)),and(pk(025625f41e4a065efc06d5019cbbd56fe8c07595af1231e7cbc03fafb87ebb71ec),older(10)))



//////////////////////////////// ANY HEDGE CONTRACT ///////////////////////////////////

; Compiled on 2020-10-30
; Opcode count: 189 of 201
; Bytesize: (336~357) of 361 (520 - 159)
<maturityHeight> <earliestLiquidationHeight>            ; 2 * (3~4) + 2 * 1 bytes
<highLiquidationPrice> <lowLiquidationPrice>            ; 2 * (1~4) + 2 * 1 bytes
<lowTruncatedZeroes> <highLowDeltaTruncatedZeroes>      ; (0~4) + (0~3) + 2 * 1 bytes
<hedgeUnitsXSatsPerBch_HighTrunc> <payoutSats_LowTrunc> ; 2 * (1~4) + 2 * 1 bytes
<payoutDataHash> <mutualRedemptionDataHash>             ; 2 * 20 + 2 * 1 bytes
; Mutual Redeem
OP_10 OP_PICK OP_0 OP_NUMEQUAL OP_IF
    ; Verify data hash
    OP_11 OP_PICK OP_13 OP_PICK OP_CAT OP_HASH160 OP_EQUALVERIFY
    ; Verify hedge sig
    OP_12 OP_ROLL OP_11 OP_ROLL OP_CHECKSIGVERIFY
    ; Verify long sig
    OP_11 OP_ROLL OP_11 OP_ROLL OP_CHECKSIGVERIFY
    ; Cleanup
    OP_2DROP OP_2DROP OP_2DROP OP_2DROP OP_2DROP OP_1
; Payout
OP_ELSE OP_10 OP_ROLL OP_1 OP_NUMEQUALVERIFY
    ; Decode preimage
    ; tx.hashPrevouts
    OP_10 OP_PICK OP_4 OP_SPLIT OP_NIP 20 OP_SPLIT
    ; tx.outpoint
    20 OP_SPLIT OP_NIP 24 OP_SPLIT
    ; tx.hashOutputs
    OP_SIZE 28 OP_SUB OP_SPLIT OP_NIP 20 OP_SPLIT OP_DROP
    ; Verify there's only a single input
    OP_ROT OP_ROT OP_HASH256 OP_EQUALVERIFY
    ; Verify data hash
    OP_ROT OP_12 OP_PICK OP_14 OP_PICK OP_CAT OP_15 OP_PICK OP_CAT OP_HASH160 OP_EQUALVERIFY

    ; Verify oracle sig
    11 OP_ROLL 11 OP_PICK OP_15 OP_ROLL OP_CHECKDATASIGVERIFY
    ; Verify preimage
    OP_13 OP_ROLL OP_14 OP_ROLL OP_2DUP OP_SWAP OP_SIZE OP_1SUB OP_SPLIT OP_DROP
    OP_14 OP_ROLL OP_SHA256 OP_ROT OP_CHECKDATASIGVERIFY OP_CHECKSIGVERIFY
    ; Decode oracle message
    OP_12 OP_PICK OP_4 OP_SPLIT OP_NIP
    OP_DUP 24 OP_SPLIT OP_NIP
    OP_2 OP_SPLIT OP_DROP OP_BIN2NUM
    OP_14 OP_ROLL OP_4 OP_SPLIT OP_DROP OP_BIN2NUM
    OP_ROT OP_4 OP_SPLIT OP_DROP OP_BIN2NUM
    ; Check oracle price
    OP_OVER OP_0 OP_GREATERTHAN OP_VERIFY
    ; Clamp price
    OP_SWAP OP_10 OP_PICK OP_MIN OP_9 OP_PICK OP_MAX
    ; Check oracle height
    OP_OVER ff64cd1d OP_LESSTHAN OP_VERIFY
    ; Check locktime
    OP_OVER OP_CHECKLOCKTIMEVERIFY OP_DROP
    ; Check oracleHeight in bounds
    OP_OVER OP_12 OP_ROLL OP_13 OP_PICK OP_1ADD OP_WITHIN OP_VERIFY
    ; If: liquidation
    OP_SWAP OP_11 OP_ROLL OP_LESSTHAN OP_IF
        ; Check oracle price out of bounds
        OP_DUP OP_9 OP_PICK OP_1ADD OP_11 OP_PICK OP_WITHIN OP_NOT OP_VERIFY
    ; Else: maturation
    OP_ELSE
        ; Check exactly at maturity height
        OP_OVER OP_1 OP_NUMEQUALVERIFY
    OP_ENDIF
    ; TRUNCATION MATH
    ; DIV value high -> low truncation
    OP_4 OP_PICK OP_OVER OP_DIV
    OP_7 OP_PICK OP_SWAP OP_4 OP_NUM2BIN OP_CAT OP_BIN2NUM
    ; MOD value high -> truncation
    OP_5 OP_ROLL OP_2 OP_PICK OP_MOD
    ; Calculate and apply mod extension
    OP_7 OP_ROLL OP_SIZE OP_NIP
    OP_4 OP_2 OP_PICK OP_SIZE OP_NIP OP_SUB OP_OVER OP_MIN
    OP_0 OP_OVER OP_NUM2BIN
    OP_3 OP_ROLL OP_4 OP_NUM2BIN OP_CAT OP_BIN2NUM
    ; Calculate and apply price truncation
    OP_ROT OP_ROT OP_SUB
    OP_3 OP_ROLL OP_SWAP OP_SPLIT OP_NIP OP_BIN2NUM
    ; Calculate hedge sats: Divide extended MOD value by truncated price and clamp
    OP_DIV OP_ADD OP_4 OP_PICK OP_MIN
    ; Calculate long sats
    OP_4 OP_ROLL OP_OVER OP_SUB
    ; Verify hash outputs
    2202 OP_8 OP_NUM2BIN
    OP_8 OP_7 OP_PICK OP_SIZE OP_NIP OP_SUB
    OP_7 OP_PICK OP_4 OP_ROLL OP_2 OP_PICK OP_NUM2BIN OP_CAT OP_2 OP_PICK OP_OR
    OP_10 OP_ROLL OP_CAT
    OP_7 OP_ROLL OP_4 OP_ROLL OP_3 OP_ROLL OP_NUM2BIN OP_CAT OP_ROT OP_OR
    OP_7 OP_ROLL OP_CAT
    OP_3 OP_ROLL OP_ROT OP_ROT OP_CAT OP_HASH256 OP_EQUALVERIFY
    ; Cleanup
    OP_2DROP OP_2DROP OP_1
OP_ENDIF
