//==========================================================================
/**
* @file	 : Subscriber.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2018-08-23 20:03
* purpose : 
1.redis pub/sub接口(负责订阅、接收、分发redis消息);
2.这里开了一个线程，操作消息队列时要加锁;
*/
//==========================================================================
#ifndef __Subscriber_h__
#define __Subscriber_h__

#include "RedisClient.h"

namespace redis
{
	class Subscriber 
		: public RedisClient
	{
	public:
		Subscriber();
		~Subscriber();
		virtual void dispatch(redisContext* c, const stRedisData& redisData);
		virtual bool start();
		virtual void polling();

	public:
		static void redisThread(void* pSub);
		bool publish(const std::string& chan, const char* data, int size);

	private:
		bool subscribe();

	public:
		uv_mutex_t m_mutexMsgs;
	};

}//end namespace redis
#endif // __Subscriber_h__