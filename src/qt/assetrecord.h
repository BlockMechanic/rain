// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_ASSETRECORD_H
#define RAIN_QT_ASSETRECORD_H

#include <math.h>
#include <amount.h>
#include <tinyformat.h>
/** UI model for Assets.
 */
class AssetRecord
{
public:

    AssetRecord(): sAssetName("")
    {
    }

    AssetRecord(const QString _sAssetName): sAssetName(_sAssetName)
    {
    }

    /** @name Immutable attributes
      @{*/

    QString sAssetName;
    QString sAssetShortName;
    CAmount balance;
    CAmount pending;
    CAmount immature;
    CAmount stake;
    QString issuer; // issuingaddress   

    CAmount inputAmount;
    CAmount issuedAmount;
    CAssetID inputAssetID;
    uint32_t nTime; //creation time
    
    bool fIsAdministrator;
    int32_t nVersion;
    int32_t flags;
    int32_t floor;

    /**@}*/
};
#endif // RAIN_QT_ASSETRECORD_H
