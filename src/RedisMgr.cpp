#include "RedisMgr.h"
#include "TimerHelper.h"
#include "Function.h"

namespace redis
{
	RedisMgr::RedisMgr()
		: m_subscriber(nullptr)
	{
		m_subscriber = safeCreateObject(Subscriber);
		m_redisClusterClient = safeCreateObject(RedisClusterClient);
		m_redisClient = safeCreateObject(RedisClient);

		setPollingEnable(true);
	}

	RedisMgr::~RedisMgr()
	{
		safeDeleteObject(m_subscriber, Subscriber);
		safeDeleteObject(m_redisClusterClient, RedisClusterClient);
		safeDeleteObject(m_redisClient, RedisClient);
	}

	void RedisMgr::Register(int ops, ops_handler handler)
	{
		auto sub = RedisMgr::getInstance().getSubscriber();
		if (sub == nullptr)
			return;

		sub->registerOps(ops, handler);
	}

	void RedisMgr::registerChan(const std::string& index)
	{
		{//push
			redis::Chan *pChan = safeCreateObject1(redis::Chan, index.c_str());
			m_subscriber->registerChan(pChan);
		}
	}

	bool RedisMgr::start(const std::string& ip, int port)
	{
		m_subscriber->init(ip.c_str(), port);
		if (!m_subscriber->start())
			return false;

		m_redisClient->init(ip.c_str(), port);
		if (!m_redisClient->start())
			return false;

		return true;
	}

	bool RedisMgr::startCluster(stl_vector<stRedisClusterCfg>& cfgs)
	{
		m_redisClusterClient->init(cfgs);
		if (!m_subscriber->start())
			return false;
		return true;
	}

	void RedisMgr::polling()
	{
		if (m_subscriber == nullptr || m_redisClient == nullptr)
			return;

 		m_subscriber->polling();
		m_redisClient->polling();
	}
} // namespace redis
