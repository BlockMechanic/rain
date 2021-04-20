// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_SENDCOINSRECIPIENT_H
#define RAIN_QT_SENDCOINSRECIPIENT_H

#if defined(HAVE_CONFIG_H)
#include <config/rain-config.h>
#endif

#include <amount.h>
#include <serialize.h>
#include <primitives/asset.h>
#include <string>

#include <QString>

class SendAssetsRecipient
{
public:
    explicit SendAssetsRecipient() : amount(0), fSubtractFeeFromAmount(false), nVersion(SendAssetsRecipient::CURRENT_VERSION) { }
    explicit SendAssetsRecipient(const QString &addr, const QString &_label, const CAmount& _amount, const QString &_message):
        address(addr), label(_label), amount(_amount), message(_message), fSubtractFeeFromAmount(false), nVersion(SendAssetsRecipient::CURRENT_VERSION) {}

    // If from an unauthenticated payment request, this is used for storing
    // the addresses, e.g. address-A<br />address-B<br />address-C.
    // Info: As we don't need to process addresses in here when using
    // payment requests, we can abuse it for displaying an address list.
    // Todo: This is a hack, should be replaced with a cleaner solution!
    QString address;
    QString label;
    CAmount amount;
    // If from a payment request, this is used for storing the memo
    QString message;
    // Keep the payment request around as a serialized string to ensure
    // load/store is lossless.
    std::string sPaymentRequest;
    // Empty if no authentication or invalid signature/cert/etc.
    QString authenticatedMerchant;

    bool fSubtractFeeFromAmount; // memory only

    static const int CURRENT_VERSION = 1;
    int nVersion;

    QString ownerAddress;
    CAsset asset;

    SERIALIZE_METHODS(SendAssetsRecipient, obj)
    {
        std::string address_str, label_str, message_str, auth_merchant_str;

        SER_WRITE(obj, address_str = obj.address.toStdString());
        SER_WRITE(obj, label_str = obj.label.toStdString());
        SER_WRITE(obj, message_str = obj.message.toStdString());
        SER_WRITE(obj, auth_merchant_str = obj.authenticatedMerchant.toStdString());

        READWRITE(obj.nVersion, address_str, label_str, obj.amount, obj.asset, message_str, obj.sPaymentRequest, auth_merchant_str);

        SER_READ(obj, obj.address = QString::fromStdString(address_str));
        SER_READ(obj, obj.label = QString::fromStdString(label_str));
        SER_READ(obj, obj.message = QString::fromStdString(message_str));
        SER_READ(obj, obj.authenticatedMerchant = QString::fromStdString(auth_merchant_str));
    }
};

#endif // RAIN_QT_SENDCOINSRECIPIENT_H
