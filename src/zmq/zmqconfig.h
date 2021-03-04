// Copyright (c) 2014-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_ZMQ_ZMQCONFIG_H
#define RAIN_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include <config/rain-config.h>
#endif

#include <stdarg.h>
#include <string>

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include <primitives/block.h>
#include <primitives/transaction.h>

#include <governance/governance-object.h>
#include <governance/governance-vote.h>

#include <llmq/quorums_chainlocks.h>

void zmqError(const char *str);

#endif // RAIN_ZMQ_ZMQCONFIG_H
