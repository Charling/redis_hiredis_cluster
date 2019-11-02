//==========================================================================
/**
* @file	 : Subscriber.h
* @author : Charling(����)/56430114@qq.com
* created : 2018-08-23 20:03
* purpose : 
1.redis pub/sub�ӿ�(�����ġ����ա��ַ�redis��Ϣ);
2.���￪��һ���̣߳�������Ϣ����ʱҪ����;
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