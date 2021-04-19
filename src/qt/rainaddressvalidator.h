// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_RAINADDRESSVALIDATOR_H
#define RAIN_QT_RAINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class RainAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RainAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Rain address widget validator, checks for a valid rain address.
 */
class RainAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RainAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // RAIN_QT_RAINADDRESSVALIDATOR_H
