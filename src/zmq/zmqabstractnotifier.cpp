// Copyright (c) 2015-2018 The Rain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqabstractnotifier.h>

const int CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM;

CZMQAbstractNotifier::~CZMQAbstractNotifier()
{
    assert(!psocket);
}

bool CZMQAbstractNotifier::NotifyBlock(const CBlockIndex * /*CBlockIndex*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyChainLock(const CBlockIndex * /*CBlockIndex*/, const llmq::CChainLockSig& /*clsig*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransaction(const CTransaction &/*transaction*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyTransactionLock(const CTransaction &/*transaction*/, const llmq::CInstantSendLock& /*islock*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyGovernanceVote(const CGovernanceVote& /*vote*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyGovernanceObject(const CGovernanceObject& /*object*/)
{
    return true;
}

bool CZMQAbstractNotifier::NotifyInstantSendDoubleSpendAttempt(const CTransaction& /*currentTx*/, const CTransaction& /*previousTx*/)
{
    return true;
}
