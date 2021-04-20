// Copyright (c) 2017-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assetdb.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <tinyformat.h>
#include <util/system.h>
#include <validation.h>
#include <wallet/ismine.h>

static const char ASSET_FLAG = 'A';
static const char ASSET_ADDRESS_QUANTITY_FLAG = 'B';
static const char ADDRESS_ASSET_QUANTITY_FLAG = 'C';

static size_t MAX_DATABASE_RESULTS = 50000;

CDatabasedAssetData::CDatabasedAssetData(const CConfidentialAsset& _asset, const CTransactionRef& assetTx, const int& nOut)
{
    SetNull();
    asset = _asset;
    const CTxOut& txout = assetTx->vout[nOut];
    CAmount inputs =0;
    int inputassets=0;
    
    nTime = assetTx->nTime;

    if(!assetTx->IsCoinBase()){
		for (const CTxIn &txin : assetTx->vin) {
			const CScript& scriptPubKey = assetTx->vout[txin.prevout.n].scriptPubKey;

				const CConfidentialAsset assettmp = assetTx->vout[txin.prevout.n].nAsset;

				if(assettmp.IsExplicit())
				   inputAssetID = assettmp.GetAsset().assetID;

				const CConfidentialValue& amount = assetTx->vout[txin.prevout.n].nValue;
				if(amount.IsExplicit())
				   inputs += amount.GetAmount();

			inputassets+=1;
		}
    }
    issuingAddress = txout.scriptPubKey;
    inputAmount = inputs;

    if(txout.nValue.IsExplicit())
       issuedAmount = txout.nValue.GetAmount();

}

CDatabasedAssetData::CDatabasedAssetData()
{
    this->SetNull();
}

CAssetsDB::CAssetsDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "assets", nCacheSize, fMemory, fWipe) {
}

bool CAssetsDB::WriteAssetData(const CConfidentialAsset &asset, const CTransactionRef& assetTx, const int& nOut)
{
    CDatabasedAssetData data(asset, assetTx, nOut);
    return Write(std::make_pair(ASSET_FLAG, asset.GetAsset().getName()), data);
}

bool CAssetsDB::ReadAssetData(const std::string& strName, CConfidentialAsset& asset, CScript &issuingAddress, CAmount& inputAmount, CAmount& issuedAmount, CAssetID& inputAssetID)
{
    CDatabasedAssetData data;
    bool ret =  Read(std::make_pair(ASSET_FLAG, strName), data);

    if (ret) {
        asset = data.asset;
        issuingAddress = data.issuingAddress;
        inputAmount = data.inputAmount;
        issuedAmount = data.issuedAmount;
        inputAssetID = data.inputAssetID;
    }

    return ret;
}

bool CAssetsDB::EraseAssetData(const std::string& sAssetName)
{
    return Erase(std::make_pair(ASSET_FLAG, sAssetName));
}

void DumpAssets()
{
    for(auto const& x : passetsCache->GetItemsMap()){
        CDatabasedAssetData data = x.second->second;
        passetsdb->Write(std::make_pair(ASSET_FLAG, x.first), data);
    }
}

bool CAssetsDB::LoadAssets()
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(ASSET_FLAG, std::string()));

    // Load assets
    while (pcursor->Valid()) {
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
            CDatabasedAssetData data;
            if (pcursor->GetValue(data)) {
				if(data.asset.IsExplicit())
                    passetsCache->Put(data.asset.GetAsset().getName(), data);
                else
                    passetsCache->Put("UNDEFINED", data);
                pcursor->Next();
            } else {
                return error("%s: failed to read asset", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

bool CAssetsDB::AssetDir(std::vector<CDatabasedAssetData>& assets, const std::string filter, const size_t count, const long start)
{
    //FlushStateToDisk();
    BlockValidationState state_dummy;
    const CChainParams& chainparams = Params();
    ::ChainstateActive().FlushStateToDisk(chainparams, state_dummy, FlushStateMode::PERIODIC);

    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ASSET_FLAG, std::string()));

    auto prefix = filter;
    bool wildcard = prefix.back() == '*';
    if (wildcard)
        prefix.pop_back();

    size_t skip = 0;
    if (start >= 0) {
        skip = start;
    }
    else {
        // compute table size for backwards offset
        long table_size = 0;
        while (pcursor->Valid()) {
            std::pair<char, std::string> key;
            if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
                if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                    table_size += 1;
                }
            }
            pcursor->Next();
        }
        skip = table_size + start;
        pcursor->SeekToFirst();
    }


    size_t loaded = 0;
    size_t offset = 0;

    // Load assets
    while (pcursor->Valid() && loaded < count) {
        std::pair<char, std::string> key;
        if (pcursor->GetKey(key) && key.first == ASSET_FLAG) {
            if (prefix == "" ||
                    (wildcard && key.second.find(prefix) == 0) ||
                    (!wildcard && key.second == prefix)) {
                if (offset < skip) {
                    offset += 1;
                }
                else {
                    CDatabasedAssetData data;
                    if (pcursor->GetValue(data)) {
                        assets.push_back(data);
                        loaded += 1;
                    } else {
                        return error("%s: failed to read asset", __func__);
                    }
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}

bool CAssetsDB::AddressDir(std::vector<std::pair<std::string, CAmount> >& vecAssetAmount, int& totalEntries, const bool& fGetTotal, const std::string& address, const size_t count, const long start)
{
    //FlushStateToDisk();
    BlockValidationState state_dummy;
    ::ChainstateActive().FlushStateToDisk(Params(), state_dummy, FlushStateMode::PERIODIC);
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ADDRESS_ASSET_QUANTITY_FLAG, std::make_pair(address, std::string())));

    if (fGetTotal) {
        totalEntries = 0;
        while (pcursor->Valid()) {
            std::pair<char, std::pair<std::string, std::string> > key;
            if (pcursor->GetKey(key) && key.first == ADDRESS_ASSET_QUANTITY_FLAG && key.second.first == address) {
                totalEntries++;
            }
            pcursor->Next();
        }
        return true;
    }

    size_t skip = 0;
    if (start >= 0) {
        skip = start;
    }
    else {
        // compute table size for backwards offset
        long table_size = 0;
        while (pcursor->Valid()) {
            std::pair<char, std::pair<std::string, std::string> > key;
            if (pcursor->GetKey(key) && key.first == ADDRESS_ASSET_QUANTITY_FLAG && key.second.first == address) {
                table_size += 1;
            }
            pcursor->Next();
        }
        skip = table_size + start;
        pcursor->SeekToFirst();
    }


    size_t loaded = 0;
    size_t offset = 0;

    // Load assets
    while (pcursor->Valid() && loaded < count && loaded < MAX_DATABASE_RESULTS) {
        std::pair<char, std::pair<std::string, std::string> > key;
        if (pcursor->GetKey(key) && key.first == ADDRESS_ASSET_QUANTITY_FLAG && key.second.first == address) {
                if (offset < skip) {
                    offset += 1;
                }
                else {
                    CAmount amount;
                    if (pcursor->GetValue(amount)) {
                        vecAssetAmount.emplace_back(std::make_pair(key.second.second, amount));
                        loaded += 1;
                    } else {
                        return error("%s: failed to Address Asset Quanity", __func__);
                    }
                }
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}

// Can get to total count of addresses that belong to a certain asset_name, or get you the list of all address that belong to a certain asset_name
bool CAssetsDB::AssetAddressDir(std::vector<std::pair<std::string, CAmount> >& vecAddressAmount, int& totalEntries, const bool& fGetTotal, const std::string& sAssetName, const size_t count, const long start)
{
    //FlushStateToDisk();
    BlockValidationState state_dummy;
    ::ChainstateActive().FlushStateToDisk(Params(), state_dummy, FlushStateMode::PERIODIC);
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    pcursor->Seek(std::make_pair(ASSET_ADDRESS_QUANTITY_FLAG, std::make_pair(sAssetName, std::string())));

    if (fGetTotal) {
        totalEntries = 0;
        while (pcursor->Valid()) {
            std::pair<char, std::pair<std::string, std::string> > key;
            if (pcursor->GetKey(key) && key.first == ASSET_ADDRESS_QUANTITY_FLAG && key.second.first == sAssetName) {
                totalEntries += 1;
            }
            pcursor->Next();
        }
        return true;
    }

    size_t skip = 0;
    if (start >= 0) {
        skip = start;
    }
    else {
        // compute table size for backwards offset
        long table_size = 0;
        while (pcursor->Valid()) {
            std::pair<char, std::pair<std::string, std::string> > key;
            if (pcursor->GetKey(key) && key.first == ASSET_ADDRESS_QUANTITY_FLAG && key.second.first == sAssetName) {
                table_size += 1;
            }
            pcursor->Next();
        }
        skip = table_size + start;
        pcursor->SeekToFirst();
    }

    size_t loaded = 0;
    size_t offset = 0;

    // Load assets
    while (pcursor->Valid() && loaded < count && loaded < MAX_DATABASE_RESULTS) {
        std::pair<char, std::pair<std::string, std::string> > key;
        if (pcursor->GetKey(key) && key.first == ASSET_ADDRESS_QUANTITY_FLAG && key.second.first == sAssetName) {
            if (offset < skip) {
                offset += 1;
            }
            else {
                CAmount amount;
                if (pcursor->GetValue(amount)) {
                    vecAddressAmount.emplace_back(std::make_pair(key.second.second, amount));
                    loaded += 1;
                } else {
                    return error("%s: failed to Asset Address Quanity", __func__);
                }
            }
            pcursor->Next();
        } else {
            break;
        }
    }

    return true;
}

bool CAssetsDB::AssetDir(std::vector<CDatabasedAssetData>& assets)
{
    return CAssetsDB::AssetDir(assets, "*", MAX_SIZE, 0);
}
