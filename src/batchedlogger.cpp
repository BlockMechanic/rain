// Copyright (c) 2018-2019 The Rain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchedlogger.h>
#include <util/system.h>

CBatchedLogger::CBatchedLogger(BCLog::LogFlags _category, const std::string& _header) :
    accept(LogAcceptCategory(_category)), header(_header)
{
}

CBatchedLogger::~CBatchedLogger()
{
    Flush();
}

void CBatchedLogger::Flush()
{
    if (!accept || msg.empty()) {
        return;
    }
    LogPrintf("%s:\n%s", header, msg);
    msg.clear();
}
