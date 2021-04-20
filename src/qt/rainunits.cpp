// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/rainunits.h>

#include <QStringList>

#include <cassert>

static constexpr auto MAX_DIGITS_BTC = 16;

RainUnits::RainUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<RainUnits::Unit> RainUnits::availableUnits()
{
    QList<RainUnits::Unit> unitlist;
    unitlist.append(COIN);
    unitlist.append(CENT);
    unitlist.append(BIT);
    unitlist.append(SAT);
    return unitlist;
}

bool RainUnits::valid(int unit)
{
    switch(unit)
    {
    case COIN:
    case CENT:
    case BIT:
    case SAT:
        return true;
    default:
        return false;
    }
}

QString RainUnits::longName(int unit)
{
    switch(unit)
    {
    case COIN: return QString("COIN");
    case CENT: return QString("CENT");
    case BIT: return QString::fromUtf8("BIT (bits)");
    case SAT: return QString("Satoshi (sat)");
    default: return QString("???");
    }
}

QString RainUnits::shortName(int unit)
{
    switch(unit)
    {
    case BIT: return QString::fromUtf8("bits");
    case SAT: return QString("sat");
    default: return longName(unit);
    }
}

QString RainUnits::description(int unit)
{
    switch(unit)
    {
    case COIN: return QString("Full unit");
    case CENT: return QString("Milli-unit (1 / 1" THIN_SP_UTF8 "000)");
    case BIT: return QString("Micro-unit (bits) (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case SAT: return QString("Satoshi (sat) (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    default: return QString("???");
    }
}

qint64 RainUnits::factor(int unit)
{
    switch(unit)
    {
    case COIN: return 100000000;
    case CENT: return 100000;
    case BIT:  return 100;
    case SAT:  return 1;
    default: return 100000000;
    }
}

int RainUnits::decimals(int unit)
{
    switch(unit)
    {
    case COIN: return 8;
    case CENT: return 5;
    case BIT: return 2;
    case SAT: return 0;
    default: return 0;
    }
}

QString RainUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool justify)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    QString quotient_str = QString::number(quotient);
    if (justify) {
        quotient_str = quotient_str.rightJustified(MAX_DIGITS_BTC - num_decimals, ' ');
    }

    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == SeparatorStyle::ALWAYS || (separators == SeparatorStyle::STANDARD && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    if (num_decimals > 0) {
        qint64 remainder = n_abs % coin;
        QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');
        return quotient_str + QString(".") + remainder_str;
    } else {
        return quotient_str;
    }
}


// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString RainUnits::simplestFormat(int unit, const CAmount& amount, int digits, bool plussign, SeparatorStyle separators)
{
    QString result = format(unit, amount, plussign, separators);
    if(decimals(unit) > digits) {
		int lenght = result.mid(result.indexOf("."), result.length() - 1).length() - 1;
		if (lenght > digits) {
			result.chop(lenght - digits);
		}      
    }

    return result;
}

QString RainUnits::simpleFormat(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators);
}

QString RainUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + shortName(unit);
}

QString RainUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

QString RainUnits::formatWithPrivacy(int unit, const CAmount& amount, SeparatorStyle separators, bool privacy)
{
    assert(amount >= 0);
    QString value;
    if (privacy) {
        value = format(unit, 0, false, separators, true).replace('0', '#');
    } else {
        value = format(unit, amount, false, separators, true);
    }
    return value + QString(" ") + shortName(unit);
}

bool RainUnits::parse(int unit, const QString &value, CAmount *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

QString RainUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (RainUnits::valid(unit))
    {
        amountTitle += " ("+RainUnits::shortName(unit) + ")";
    }
    return amountTitle;
}

int RainUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant RainUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(longName(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}

CAmount RainUnits::maxMoney()
{
    return MAX_MONEY;
}


QString formatAssetAmount(const CAsset& asset, const CAmount& amount, const int rain_unit, RainUnits::SeparatorStyle separators, bool include_asset_name)
{
    qlonglong abs = qAbs(amount);
    qlonglong whole = abs / 100000000;
    qlonglong fraction = abs % 100000000;
    QString str = QString("%1").arg(whole);
    if (amount < 0) {
        str.insert(0, '-');
    }
    if (fraction) {
        str += QString(".%1").arg(fraction, 8, 10, QLatin1Char('0'));
    }
    if (include_asset_name) {
        str += QString(" ") + QString::fromStdString(asset.getShortName());
    }
//    return str;
    return RainUnits::simplestFormat(rain_unit, amount, 4, false, separators) + QString(" ") + QString::fromStdString(asset.getShortName());
}

QString formatMultiAssetAmount(const CAmountMap& amountmap, const int rain_unit, RainUnits::SeparatorStyle separators, QString line_separator)
{
    QStringList ret;

    QString rmp = ret.join(line_separator);
    for (const auto& assetamount : amountmap) {
        ret << formatAssetAmount(assetamount.first, assetamount.second, rain_unit, separators);
    }
    QString tmp = ret.join(line_separator);

    return ret.join(line_separator);
}

bool parseAssetAmount(const CAsset& asset, const QString& text, const int rain_unit, CAmount *val_out)
{
    return RainUnits::parse(rain_unit, text, val_out);
}
