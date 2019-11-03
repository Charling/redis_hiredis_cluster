//==========================================================================
/**
* @file	 : RedisMgr.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2018-08-23 20:03
* purpose : redis服务管理器
*/
//==========================================================================
#ifndef __RedisMgr_h__
#define __RedisMgr_h__

#include "Chan.h"
#include "SingletonEx.h"
#include "Subscriber.h"
#include "Service.h"
#include "RedisClusterClient.h"

namespace redis
{
	class RedisMgr
		: public SingletonEx<RedisMgr>
		, public Service
	{
	  public:
		RedisMgr();
		~RedisMgr();

	  public:
		//Service
		virtual void polling();

		static void Register(int ops, ops_handler handler);
		bool start(const std::string& ip, int port);
		bool startCluster(stl_vector<stRedisClusterCfg>& cfgs);
		void registerChan(const std::string& index);

		inline Subscriber* getSubscriber() { return m_subscriber; }
		inline RedisClient* getRedisClient() { return m_redisClient; }
		inline RedisClusterClient* getRedisClusterClient() { return m_redisClusterClient; }

	  private:
		Subscriber* m_subscriber;
		RedisClient* m_redisClient;
		RedisClusterClient* m_redisClusterClient;
	};

} //end namespace redis

#endif // __RedisMgr_h__