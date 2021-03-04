// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_OPTIONSMODEL_H
#define RAIN_QT_OPTIONSMODEL_H

#include <amount.h>

#include <QAbstractListModel>
#include <QSettings>

namespace interfaces {
class Node;
}

extern const char *DEFAULT_GUI_PROXY_HOST;
static constexpr unsigned short DEFAULT_GUI_PROXY_PORT = 9050;

/** Interface from Qt to configuration data structure for Rain client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(interfaces::Node& node, QObject *parent = nullptr, bool resetSettings = false);

    enum OptionID {
        StartAtStartup,         // bool
        HideTrayIcon,           // bool
        MinimizeToTray,         // bool
        MapPortUPnP,            // bool
        MinimizeOnClose,        // bool
        ProxyUse,               // bool
        ProxyIP,                // QString
        ProxyPort,              // int
        ProxyUseTor,            // bool
        ProxyIPTor,             // QString
        ProxyPortTor,           // int
        DisplayUnit,            // RainUnits::Unit
        ThirdPartyTxUrls,       // QString
        Digits,              // QString
        Language,               // QString
        CoinControlFeatures,    // bool
        ThreadsScriptVerif,     // int
        Prune,                  // bool
        PruneSize,              // int
        DatabaseCache,          // int
        SpendZeroConfChange,    // bool
        Listen,                 // bool
	EnableMessageSendConf, // bool
        NotUseChangeAddress,    // bool
        ReserveBalance,         // CAmount
        HideCharts,          // bool
        HideZeroBalances,    // bool
        ShowMasternodesTab,  // bool
        StakeSplitThreshold,    // CAmount (LongLong)
        ShowColdStakingScreen,  // bool
        OptionIDRowCount,
    };

    void Init(bool resetSettings = false);
    void Reset();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    void refreshDataView();
    /** Updates current unit in memory, settings and emits displayUnitChanged(newUnit) signal */
    void setDisplayUnit(const QVariant &value);

    /* Update StakeSplitThreshold's value in wallet */
    void setStakeSplitThreshold(const CAmount value);
    double getSSTMinimum() const;
    bool isSSTValid();

    /* Explicit getters */
    bool isHideCharts() { return fHideCharts; }
    bool getHideTrayIcon() const { return fHideTrayIcon; }
    bool getMinimizeToTray() const { return fMinimizeToTray; }
    bool getMinimizeOnClose() const { return fMinimizeOnClose; }
    int getDisplayUnit() const { return nDisplayUnit; }
    QString getThirdPartyTxUrls() const { return strThirdPartyTxUrls; }
   // bool getProxySettings(QNetworkProxy& proxy) const;
    bool getCoinControlFeatures() const { return fCoinControlFeatures; }
    const QString& getOverriddenByCommandLine() { return strOverriddenByCommandLine; }

    /* Explicit setters */
    void SetPrune(bool prune, bool force = false);

	bool getEnableMessageSendConf();

    /* Restart flag helper */
    void setRestartRequired(bool fRequired);
    bool isRestartRequired() const;
    void setSSTChanged(bool fChanged);
    bool isSSTChanged();
    interfaces::Node& node() const { return m_node; }

    bool isColdStakingScreenEnabled() { return showColdStakingScreen; }
    bool invertColdStakingScreenStatus() {
        setData(
                createIndex(ShowColdStakingScreen, 0),
                !isColdStakingScreenEnabled(),
                Qt::EditRole
        );
        return showColdStakingScreen;
    }

    // Reset
    void setMainDefaultOptions(QSettings& settings, bool reset = false);
    void setWalletDefaultOptions(QSettings& settings, bool reset = false);
    void setNetworkDefaultOptions(QSettings& settings, bool reset = false);
    void setWindowDefaultOptions(QSettings& settings, bool reset = false);
    void setDisplayDefaultOptions(QSettings& settings, bool reset = false);
private:
    interfaces::Node& m_node;
    /* Qt-only settings */
    bool fHideTrayIcon;
    bool fMinimizeToTray;
    bool fMinimizeOnClose;
    QString language;
    int nDisplayUnit;
    QString strThirdPartyTxUrls;
    bool fCoinControlFeatures;
    bool showColdStakingScreen;
    bool fHideCharts;
    bool fHideZeroBalances;
    /* settings that were overridden by command-line */
    QString strOverriddenByCommandLine;
	bool fEnableMessageSendConf;

    // Add option to list of GUI options overridden through command line/config file
    void addOverriddenOption(const std::string &option);

    // Check settings version and upgrade default values if required
    void checkAndMigrate();
Q_SIGNALS:
    void displayUnitChanged(int unit);
    void coinControlFeaturesChanged(bool);
    void showHideColdStakingScreen(bool);
    void hideChartsChanged(bool);
    void hideZeroBalancesChanged(bool);
    void hideTrayIconChanged(bool);
    void enableMessageSendConfChanged(bool);
};

#endif // RAIN_QT_OPTIONSMODEL_H
