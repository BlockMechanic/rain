// Copyright (c) 2011-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_QT_GUICONSTANTS_H
#define RAIN_QT_GUICONSTANTS_H

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 250;

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* RainGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

#if defined(Q_OS_ANDROID)
    static const bool DEFAULT_SPLASHSCREEN = false;
#else
    static const bool DEFAULT_SPLASHSCREEN = true;
#endif

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080"
#define STYLE_VALID "border: 1px solid lightgray; padding: 0px;"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 0, 0)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)
/* Transaction list -- TX status decoration - open until date */
#define COLOR_TX_STATUS_OPENUNTILDATE QColor(64, 64, 255)
/* Transaction list -- TX status decoration - offline */
#define COLOR_TX_STATUS_OFFLINE QColor(192, 192, 192)
/* Transaction list -- TX status decoration - danger, tx needs attention */
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
/* Transaction list -- TX status decoration - default color */
#define COLOR_TX_STATUS_SPV_BACKGROUND QColor(240,240,240)
/* Transaction list -- TX status decoration - default color */
#define COLOR_BLACK QColor(0, 0, 0)
/* Widget Background color - default color */
#define COLOR_WHITE QColor(255, 255, 255)

#define COLOR_WALLETFRAME_SHADOW QColor(0,0,0,71)

/* Color of labels */
#define COLOR_LABELS QColor("#4960ad")

/** LIGHT MODE */
/* Background color, very light gray */
#define COLOR_BACKGROUND_LIGHT QColor("#fbfbfe")
/* Raincoin dark orange */
#define COLOR_DARK_ORANGE QColor("#f05339")
/* Raincoin light orange */
#define COLOR_LIGHT_ORANGE QColor("#f79433")
/* Raincoin dark blue */
#define COLOR_DARK_BLUE QColor("#475eaa")
/* Raincoin light blue */
#define COLOR_LIGHT_BLUE QColor("#5874cf")
/* Raincoin asset text */
#define COLOR_ASSET_TEXT QColor(255, 255, 255)
/* Raincoin shadow color - light mode */
#define COLOR_SHADOW_LIGHT QColor("#e1e6f3")
/* Toolbar not selected text color */
#define COLOR_TOOLBAR_NOT_SELECTED_TEXT QColor("#a5b7f3")
/* Toolbar selected text color */
#define COLOR_TOOLBAR_SELECTED_TEXT COLOR_WHITE

/** DARK MODE */
/* Widget background color, dark mode */
#define COLOR_WIDGET_BACKGROUND_DARK QColor("#1c2535")
/* Raincoin shadow color - dark mode */
#define COLOR_SHADOW_DARK QColor("#0c1b3d")
/* Raincoin Light blue - dark mode - dark mode */
#define COLOR_LIGHT_BLUE_DARK QColor("#1e2636")
/* Raincoin Dark blue - dark mode - dark mode */
#define COLOR_DARK_BLUE_DARK QColor("#0b1018")
/* Pricing widget background color */
#define COLOR_PRICING_WIDGET QColor("#161e2d")
/* Raincoin dark mode administrator background color */
#define COLOR_ADMIN_CARD_DARK COLOR_BLACK
/* Raincoin dark mode regular asset background color */
#define COLOR_REGULAR_CARD_DARK_BLUE_DARK_MODE QColor("#06132a")
/* Raincoin dark mode regular asset background color */
#define COLOR_REGULAR_CARD_LIGHT_BLUE_DARK_MODE QColor("#0e1b3b")
/* Toolbar not selected text color */
#define COLOR_TOOLBAR_NOT_SELECTED_TEXT_DARK_MODE QColor("#6c80c5")
/* Toolbar selected text color */
#define COLOR_TOOLBAR_SELECTED_TEXT_DARK_MODE QColor("#c5ccdf")


/* Raincoin label color as a string */
#define STRING_LABEL_COLOR "color: #4960ad"
#define STRING_LABEL_COLOR_WARNING "color: #FF8080"

/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;


/* Number of frames in spinner animation */
#define SPINNER_FRAMES 36

#define QAPP_ORG_NAME "Rain"
#define QAPP_ORG_DOMAIN "rain.org"
#define QAPP_APP_NAME_DEFAULT "Rain-Qt"
#define QAPP_APP_NAME_TESTNET "Rain-Qt-testnet"
#define QAPP_APP_NAME_REGTEST "Rain-Qt-regtest"

/* One gigabyte (GB) in bytes */
static constexpr uint64_t GB_BYTES{1000000000};

#endif // RAIN_QT_GUICONSTANTS_H
