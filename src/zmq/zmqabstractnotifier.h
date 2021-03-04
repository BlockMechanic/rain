// Copyright (c) 2015-2020 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAIN_ZMQ_ZMQABSTRACTNOTIFIER_H
#define RAIN_ZMQ_ZMQABSTRACTNOTIFIER_H

#include <zmq/zmqconfig.h>

class CBlockIndex;
class CGovernanceObject;
class CGovernanceVote;
class CZMQAbstractNotifier;

namespace llmq {
    class CChainLockSig;
    class CInstantSendLock;
} // namespace llmq

typedef CZMQAbstractNotifier* (*CZMQNotifierFactory)();

class CZMQAbstractNotifier
{
public:
    static const int DEFAULT_ZMQ_SNDHWM {1000};

    CZMQAbstractNotifier() : psocket(nullptr), outbound_message_high_water_mark(DEFAULT_ZMQ_SNDHWM) { }
    virtual ~CZMQAbstractNotifier();

    template <typename T>
    static CZMQAbstractNotifier* Create()
    {
        return new T();
    }

    std::string GetType() const { return type; }
    void SetType(const std::string &t) { type = t; }
    std::string GetAddress() const { return address; }
    void SetAddress(const std::string &a) { address = a; }
    int GetOutboundMessageHighWaterMark() const { return outbound_message_high_water_mark; }
    void SetOutboundMessageHighWaterMark(const int sndhwm) {
        if (sndhwm >= 0) {
            outbound_message_high_water_mark = sndhwm;
        }
    }

    virtual bool Initialize(void *pcontext) = 0;
    virtual void Shutdown() = 0;

    virtual bool NotifyBlock(const CBlockIndex *pindex);
    virtual bool NotifyChainLock(const CBlockIndex *pindex, const llmq::CChainLockSig& clsig);
    virtual bool NotifyTransaction(const CTransaction &transaction);
    virtual bool NotifyGovernanceVote(const CGovernanceVote &vote);
    virtual bool NotifyGovernanceObject(const CGovernanceObject &object);

protected:
    void *psocket;
    std::string type;
    std::string address;
    int outbound_message_high_water_mark; // aka SNDHWM
};

#endif // RAIN_ZMQ_ZMQABSTRACTNOTIFIER_H
