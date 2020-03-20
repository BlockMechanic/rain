// Copyright (c) 2018-2019 The Rain Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_BATCHEDLOGGER_H
#define RAIN_BATCHEDLOGGER_H

#include <tinyformat.h>
#include <logging.h>

class CBatchedLogger
{
private:
    bool accept;
    std::string header;
    std::string msg;
public:
    CBatchedLogger(BCLog::LogFlags category, const std::string& _header);
    virtual ~CBatchedLogger();

    template<typename... Args>
    void Batch(const std::string& fmt, const Args&... args)
    {
        if (!accept) {
            return;
        }
        msg += "    " + strprintf(fmt, args...) + "\n";
    }

    void Flush();
};

#endif //RAIN_BATCHEDLOGGER_H
