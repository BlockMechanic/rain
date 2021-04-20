// Copyright (c) 2017-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_ASSETDB_H
#define RAIN_ASSETDB_H

#include <fs.h>
#include <primitives/confidential.h>
#include <primitives/transaction.h>

#include <serialize.h>
#include <list>
#include <unordered_map>
#include <string>
#include <map>
#include <dbwrapper.h>

const int8_t ASSET_UNDO_INCLUDES_VERIFIER_STRING = -1;

class COutPoint;

class CDatabasedAssetData
{
public:
    CConfidentialAsset asset;
    CScript issuingAddress;
    CAmount inputAmount;
    CAmount issuedAmount;
    CAssetID inputAssetID;
    uint32_t nTime;

    CDatabasedAssetData(const CConfidentialAsset& asset, const CTransactionRef& assetTx, const int& nOut);
    CDatabasedAssetData();

    void SetNull()
    {
        asset.SetNull();
        issuingAddress.clear();
        inputAmount =0;
        issuedAmount =0;
        inputAssetID.SetNull();
        nTime=0;
    }

    SERIALIZE_METHODS(CDatabasedAssetData, obj) {READWRITE(obj.asset, obj.issuingAddress, obj.inputAmount, obj.issuedAmount, obj.inputAssetID, obj.nTime);}
};


/** Access to the block database (blocks/index/) */
class CAssetsDB : public CDBWrapper
{
public:
    explicit CAssetsDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    CAssetsDB(const CAssetsDB&) = delete;
    CAssetsDB& operator=(const CAssetsDB&) = delete;

    // Write to database functions
    bool WriteAssetData(const CConfidentialAsset& asset, const CTransactionRef& assetTx, const int& nOut);

    // Read from database functions
    bool ReadAssetData(const std::string& strName, CConfidentialAsset& asset, CScript &issuingAddress, CAmount& inputAmount, CAmount& issuedAmount, CAssetID& inputAssetID);

    // Erase from database functions
    bool EraseAssetData(const std::string& sAssetName);
    bool EraseMyAssetData(const std::string& sAssetName);

    // Helper functions
    bool LoadAssets();
    bool AssetDir(std::vector<CDatabasedAssetData>& assets, const std::string filter, const size_t count, const long start);
    bool AssetDir(std::vector<CDatabasedAssetData>& assets);

    bool AddressDir(std::vector<std::pair<std::string, CAmount> >& vecAssetAmount, int& totalEntries, const bool& fGetTotal, const std::string& address, const size_t count, const long start);
    bool AssetAddressDir(std::vector<std::pair<std::string, CAmount> >& vecAddressAmount, int& totalEntries, const bool& fGetTotal, const std::string& sAssetName, const size_t count, const long start);
};

// Least Recently Used Cache
template<typename cache_key_t, typename cache_value_t>
class CLRUCache
{
public:
    typedef typename std::pair<cache_key_t, cache_value_t> key_value_pair_t;
    typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

    CLRUCache(size_t max_size) : maxSize(max_size)
    {
    }
    CLRUCache()
    {
        SetNull();
    }

    void Put(const cache_key_t& key, const cache_value_t& value)
    {
        auto it = cacheItemsMap.find(key);
        cacheItemsList.push_front(key_value_pair_t(key, value));
        if (it != cacheItemsMap.end())
        {
            cacheItemsList.erase(it->second);
            cacheItemsMap.erase(it);
        }
        cacheItemsMap[key] = cacheItemsList.begin();

        if (cacheItemsMap.size() > maxSize)
        {
            auto last = cacheItemsList.end();
            last--;
            cacheItemsMap.erase(last->first);
            cacheItemsList.pop_back();
        }
    }

    void Erase(const cache_key_t& key)
    {
        auto it = cacheItemsMap.find(key);
        if (it != cacheItemsMap.end())
        {
            cacheItemsList.erase(it->second);
            cacheItemsMap.erase(it);
        }
    }

    const cache_value_t& Get(const cache_key_t& key)
    {
        auto it = cacheItemsMap.find(key);
        if (it == cacheItemsMap.end())
        {
            throw std::range_error("There is no such key in cache");
        }
        else
        {
            cacheItemsList.splice(cacheItemsList.begin(), cacheItemsList, it->second);
            return it->second->second;
        }
    }

    bool Exists(const cache_key_t& key) const
    {
        return cacheItemsMap.find(key) != cacheItemsMap.end();
    }

    size_t Size() const
    {
        return cacheItemsMap.size();
    }


    void Clear()
    {
        cacheItemsMap.clear();
        cacheItemsList.clear();
    }

    void SetNull()
    {
        maxSize = 0;
        Clear();
    }

    size_t MaxSize() const
    {
        return maxSize;
    }


    void SetSize(const size_t size)
    {
        maxSize = size;
    }

   const std::unordered_map<cache_key_t, list_iterator_t>& GetItemsMap()
    {
        return cacheItemsMap;
    };

    const std::list<key_value_pair_t>& GetItemsList()
    {
        return cacheItemsList;
    };


    CLRUCache(const CLRUCache& cache)
    {
        this->cacheItemsList = cache.cacheItemsList;
        this->cacheItemsMap = cache.cacheItemsMap;
        this->maxSize = cache.maxSize;
    }

private:
    std::list<key_value_pair_t> cacheItemsList;
    std::unordered_map<cache_key_t, list_iterator_t> cacheItemsMap;
    size_t maxSize;
};

void DumpAssets();

#endif //RAIN_ASSETDB_H
