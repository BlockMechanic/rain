// Copyright (c) 2012-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_VERSION_H
#define RAIN_VERSION_H

/**
 * network protocol versioning
 */

static const int PROTOCOL_VERSION = 80000;

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

//! In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 80000;

//! disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 80000;

//! minimum proto version of masternode to accept in DKGs
static const int MIN_MASTERNODE_PROTO_VERSION = 80000;

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 70000;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70005;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 70921;

//! "feefilter" tells peers to filter invs to you by fee starts with this version
static const int FEEFILTER_VERSION = 70013;

//! short-id-based block download starts with this version
static const int SHORT_IDS_BLOCKS_VERSION = 70014;

//! not banning for invalid compact blocks starts with this version
static const int INVALID_CB_NO_BAN_VERSION = 70015;

//! introduction of DIP3/deterministic masternodes
static const int DMN_PROTO_VERSION = 80000;

//! introduction of LLMQs
static const int LLMQS_PROTO_VERSION = 80000;

//! introduction of SENDDSQUEUE
//! TODO we can remove this in 0.15.0.0
static const int SENDDSQUEUE_PROTO_VERSION = 80000;

//! protocol version is included in MNAUTH starting with this version
static const int MNAUTH_NODE_VER_VERSION = 80000;

//! assetdata network request is allowed for this version
static const int ASSETDATA_VERSION = 80000;

static const int MIN_GOVERNANCE_PEER_PROTO_VERSION = 80000;
static const int GOVERNANCE_FILTER_PROTO_VERSION = 80000;
static const int GOVERNANCE_POSE_BANNED_VOTES_VERSION = 80000;

#endif // RAIN_VERSION_H
