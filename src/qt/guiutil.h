// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_GUIUTIL_H
#define RAIN_QT_GUIUTIL_H

#include <amount.h>
#include <fs.h>
#include <qt/rainunits.h>
#include <primitives/asset.h>

#include <QEvent>
#include <QHeaderView>
#include <QItemDelegate>
#include <QMessageBox>
#include <QObject>
#include <QProgressBar>
#include <QString>
#include <QTableView>
#include <QLabel>

class QValidatedLineEdit;
class SendCoinsRecipient;
class SendAssetsRecipient;
class WalletModel;

namespace interfaces
{
    class Node;
}

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QDateTime;
class QFont;
class QLineEdit;
class QProgressDialog;
class QUrl;
class QWidget;
class QGraphicsDropShadowEffect;
class AskPassphraseDialog;
QT_END_NAMESPACE

/** Utility functions used by the Rain Qt UI.
 */
namespace GUIUtil
{
    // Get the font for the sub labels
    QFont getSubLabelFont();
    QFont getSubLabelFontBolded();

    // Get the font for the main labels
    QFont getTopLabelFont();
    QFont getTopLabelFontBolded();
    QFont getTopLabelFont(int weight, int pxsize);

    // Create human-readable string from date
    QString dateTimeStr(const QDateTime &datetime);
    QString dateTimeStr(qint64 nTime);

    QString defaultTheme();

// Parse string into a CAmount value
CAmount parseValue(const QString& text, int displayUnit, bool* valid_out = 0);

// Format an amount
QString formatBalance(CAmount amount, int nDisplayUnit = 0);

// Request wallet unlock
bool requestUnlock(WalletModel* walletModel, bool relock);

// Set up widgets for address and amounts
void setupAddressWidget(QValidatedLineEdit* widget, QWidget* parent);
void setupAmountWidget(QLineEdit* widget, QWidget* parent);

// Update the cursor of the widget after a text change
void updateWidgetTextAndCursorPosition(QLineEdit* widget, const QString& str);

    // Return a monospace font
    QFont fixedPitchFont();

    // Parse "rain:" URI into recipient object, return true on successful parsing
    bool parseRainURI(const QUrl &uri, SendCoinsRecipient *out);
    bool parseRainURI(QString uri, SendCoinsRecipient *out);
    bool parseRainURI(const QUrl &uri, SendAssetsRecipient *out);
    bool parseRainURI(QString uri, SendAssetsRecipient *out);
    QString formatRainURI(const SendCoinsRecipient &info);
    QString formatRainURI(const SendAssetsRecipient &info);

    // Returns true if given address+amount meets "dust" definition
    bool isDust(interfaces::Node& node, const QString& address, const CAmount& amount);

    // HTML escaping for rich text controls
    QString HtmlEscape(const QString& str, bool fMultiLine=false);
    QString HtmlEscape(const std::string& str, bool fMultiLine=false);

    /** Copy a field of the currently selected entry of a view to the clipboard. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @param[in] role    Data role to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    void copyEntryData(QAbstractItemView *view, int column, int role=Qt::EditRole);

    /** Return a field of the currently selected entry as a QString. Does nothing if nothing
        is selected.
       @param[in] column  Data column to extract from the model
       @see  TransactionView::copyLabel, TransactionView::copyAmount, TransactionView::copyAddress
     */
    QList<QModelIndex> getEntryData(QAbstractItemView *view, int column);

    void setClipboard(const QString& str);

    /**
     * Determine default data directory for operating system.
     */
    QString getDefaultDataDirectory();

    /** Get save filename, mimics QFileDialog::getSaveFileName, except that it appends a default suffix
        when no suffix is provided by the user.

      @param[in] parent  Parent window (or 0)
      @param[in] caption Window caption (or empty, for default)
      @param[in] dir     Starting directory (or empty, to default to documents directory)
      @param[in] filter  Filter specification such as "Comma Separated Files (*.csv)"
      @param[out] selectedSuffixOut  Pointer to return the suffix (file type) that was selected (or 0).
                  Can be useful when choosing the save file format based on suffix.
     */
    QString getSaveFileName(QWidget *parent, const QString &caption, const QString &dir,
        const QString &filter,
        QString *selectedSuffixOut);

    /** Get open filename, convenience wrapper for QFileDialog::getOpenFileName.

      @param[in] parent  Parent window (or 0)
      @param[in] caption Window caption (or empty, for default)
      @param[in] dir     Starting directory (or empty, to default to documents directory)
      @param[in] filter  Filter specification such as "Comma Separated Files (*.csv)"
      @param[out] selectedSuffixOut  Pointer to return the suffix (file type) that was selected (or 0).
                  Can be useful when choosing the save file format based on suffix.
     */
    QString getOpenFileName(QWidget *parent, const QString &caption, const QString &dir,
        const QString &filter,
        QString *selectedSuffixOut);

    /** Get connection type to call object slot in GUI thread with invokeMethod. The call will be blocking.

       @returns If called from the GUI thread, return a Qt::DirectConnection.
                If called from another thread, return a Qt::BlockingQueuedConnection.
    */
    Qt::ConnectionType blockingGUIThreadConnection();

    // Determine whether a widget is hidden behind other windows
    bool isObscured(QWidget *w);

    // Activate, show and raise the widget
    void bringToFront(QWidget* w);

    // Open debug.log
    void openDebugLogfile();

    // Open the config file
    bool openRainConf();

// Open rain.conf
bool openConfigfile();

// Open masternode.conf
bool openMNConfigfile();

// Browse backup folder
bool showBackups();

    // Concatenate a string given the painter, static text width, left side of rect, and right side of rect
    // and which side the concatenated string is on (default left)
    void concatenate(QPainter* painter, QString& strToCon, int static_width, int left_side, int right_size);

    /** Qt event filter that intercepts ToolTipChange events, and replaces the tooltip with a rich text
      representation if needed. This assures that Qt can word-wrap long tooltip messages.
      Tooltips longer than the provided size threshold (in characters) are wrapped.
     */
    class ToolTipToRichTextFilter : public QObject
    {
        Q_OBJECT

    public:
        explicit ToolTipToRichTextFilter(int size_threshold, QObject *parent = nullptr);

    protected:
        bool eventFilter(QObject *obj, QEvent *evt);

    private:
        int size_threshold;
    };

    bool GetStartOnSystemStartup();
    bool SetStartOnSystemStartup(bool fAutoStart);

    /* Convert QString to OS specific boost path through UTF-8 */
    fs::path qstringToBoostPath(const QString &path);

    /* Convert OS specific boost path to QString through UTF-8 */
    QString boostPathToQString(const fs::path &path);

    /* Format an amount of assets in a user-friendly style */
    QString formatAssetAmount(const CAsset&, const CAmount&, int rain_unit, RainUnits::SeparatorStyle, bool include_asset_name = true);

    /* Format one or more asset+amounts in a user-friendly style */
    QString formatMultiAssetAmount(const CAmountMap&, int rain_unit, RainUnits::SeparatorStyle, QString line_separator);

    /* Parse an amount of a given asset from text */
    bool parseAssetAmount(const CAsset&, const QString& text, int rain_unit, CAmount *val_out);

    /* Convert seconds into a QString with days, hours, mins, secs */
    QString formatDurationStr(int secs);

    /* Format CNodeStats.nServices bitmask into a user-readable string */
    QString formatServicesStr(quint64 mask);

    /* Format a CNodeCombinedStats.dPingTime into a user-readable string or display N/A, if 0*/
    QString formatPingTime(double dPingTime);

    /* Format a CNodeCombinedStats.nTimeOffset into a user-readable string. */
    QString formatTimeOffset(int64_t nTimeOffset);

    QString formatNiceTimeOffset(qint64 secs);

    QString formatBytes(uint64_t bytes);

    qreal calculateIdealFontSize(int width, const QString& text, QFont font, qreal minPointSize = 4, qreal startPointSize = 14);

    class ClickableLabel : public QLabel
    {
        Q_OBJECT

    Q_SIGNALS:
        /** Emitted when the label is clicked. The relative mouse coordinates of the click are
         * passed to the signal.
         */
        void clicked(const QPoint& point);
    protected:
        void mouseReleaseEvent(QMouseEvent *event);
    };

    class ClickableProgressBar : public QProgressBar
    {
        Q_OBJECT

    Q_SIGNALS:
        /** Emitted when the progressbar is clicked. The relative mouse coordinates of the click are
         * passed to the signal.
         */
        void clicked(const QPoint& point);
    protected:
        void mouseReleaseEvent(QMouseEvent *event);
    };

    typedef ClickableProgressBar ProgressBar;

    class ItemDelegate : public QItemDelegate
    {
        Q_OBJECT
    public:
        ItemDelegate(QObject* parent) : QItemDelegate(parent) {}

    Q_SIGNALS:
        void keyEscapePressed();

    private:
        bool eventFilter(QObject *object, QEvent *event);
    };

    // Fix known bugs in QProgressDialog class.
    void PolishProgressDialog(QProgressDialog* dialog);

    /**
     * Returns the distance in pixels appropriate for drawing a subsequent character after text.
     *
     * In Qt 5.12 and before the QFontMetrics::width() is used and it is deprecated since Qt 13.0.
     * In Qt 5.11 the QFontMetrics::horizontalAdvance() was introduced.
     */
    int TextWidth(const QFontMetrics& fm, const QString& text);
} // namespace GUIUtil

#endif // RAIN_QT_GUIUTIL_H
