// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_COINRECORD_H
#define RAIN_QT_COINRECORD_H

#include <math.h>
#include <amount.h>
#include <tinyformat.h>
/** UI model for Assets.
 */
class CoinControlRecord
{
public:

    CoinControlRecord(): address("")
    {
    }

    CoinControlRecord(const QString _address): address(_address)
    {
    }

    /** @name Immutable attributes
      @{*/

    QString address;
    QString asset;
    QString label;
    QString txhash;
    int index;
    QString amount;
    QString date;   
    int64_t confirmations;
    bool locked;

    /**@}*/
};
#endif // RAIN_QT_COINRECORD_H
