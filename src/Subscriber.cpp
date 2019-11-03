#include "Subscriber.h"
#include <protocol.h>
#include "TimerHelper.h"

extern "C" {
#include "uv.h"
#include "adapters/libuv.h"
}

namespace redis
{
	Subscriber::Subscriber()
	{		
		uv_mutex_init(&m_mutexMsgs);
	//	m_thread = nullptr;
	}

	Subscriber::~Subscriber()
	{
	}

	void Subscriber::polling()
	{
		uv_mutex_lock(&m_mutexMsgs);
		
		auto it = m_mapChan.begin();
		for (; it != m_mapChan.end(); ++it)
		{		
			it->second->polling();	
		}

		uv_mutex_unlock(&m_mutexMsgs);
	}

	bool Subscriber::start()
	{	
		uv_thread_create(&m_thread, &Subscriber::redisThread, (void*)this);

		return true;
	}

	void Subscriber::redisThread(void* pSub)
	{
		Subscriber* pObj = (Subscriber*)pSub;
		if (pObj == nullptr) {
			LOGERROR("start thread error pObj == nullptr");
			return;
		}
		
		redisReply* reply = nullptr;
		stRedisData redisData;
		
		while (1)
		{	
			if (pObj->m_recvText == nullptr) { 
				if (pObj->connect(&pObj->m_recvText)) {
					pObj->subscribe();
				}
				else {
					LOGERROR("connect redis failed.");
					BASE::dSleep(200);
				}

				continue;
			}

			if (pObj->m_sendText == nullptr) {
				if (!pObj->connect(&pObj->m_sendText)) {
					LOGERROR("connect redis failed.");
					BASE::dSleep(200);
				}

				continue;
			}
	
			if (redisGetReply(pObj->m_recvText, (void **)&reply) != REDIS_OK) {
				LOGERROR("redis get reply error:%s\n", pObj->m_recvText->errstr);
				BASE::dSleep(200);
				if (pObj->reconnect(&pObj->m_recvText)) {
					pObj->subscribe();
				}
				else {
					LOGERROR("reconnect redis failed.");
				}

				continue;
			}

			redisData.clear();
			if (reply != nullptr) {
				pObj->parse(reply, redisData, 0);
				pObj->dispatch(pObj->m_recvText, redisData);
				freeReplyObject(reply);
			}
			else {
				LOGERROR("reply == nullptr, can not dispatch redis data.");
			}
		}

		LOGWARN("Subscriber server stop...");
		if (pObj->m_recvText != nullptr) {
			redisFree(pObj->m_recvText);
		}	

		if (pObj->m_sendText != nullptr) {
			redisFree(pObj->m_sendText);
		}
	}

	bool Subscriber::subscribe()
	{
		if (m_sendText == nullptr) {
			return false;
		}

		// mutex
		auto it = m_mapChan.begin();
		for (; it != m_mapChan.end(); ++it)
		{
			redisReply* reply = (redisReply *)redisCommand(m_sendText, "SUBSCRIBE %s", it->first.c_str());
			if (reply != nullptr && REDIS_REPLY_ERROR == reply->type) {
				LOGERROR("redis subscribe channel:%s error:%s!", it->first.c_str(), reply->str);
				return false;
			}

			if (reply != nullptr) {
				freeReplyObject(reply);
			}	
		}
		
		return true;
	}

	bool Subscriber::publish(const std::string& chan, const char* data, int size)
	{
		if (m_sendText == nullptr) {
			return false;
		}

		IF_NOT_RETURN_VALUE(data, false);
	
		// publish %s %s
		const int32 cnt = 3;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("PUBLISH");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "PUBLISH", argvLen[i]);
		i++;

		argvLen[i] = chan.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], chan.c_str(), chan.length());
		i++;

		argvLen[i] = size;
		argv[i] = (char*)allocateMemory(size);
		memset(argv[i], 0, size);
		memcpy(argv[i], data, size);

		redisReply* reply = (redisReply *)redisCommandArgv(m_sendText, cnt, (const char**)argv, argvLen);
	
		// free memory
		deallocateMemory(argvLen);
		for (int k = 0; k < cnt; k++){
			deallocateMemory(argv[k]);
		}
		deallocateMemory(argv);

		if (reply != nullptr)
			freeReplyObject(reply);
		return true;
	}

	void Subscriber::dispatch(redisContext* c, const stRedisData& redisData)
	{
		std::string channel;
		const stData* pData = redisData.getData(enSubPubReplyChannel);
		if (nullptr == pData) {
			LOGERROR("dispatch cann't find channel");
			return;
		}

		channel = pData->data;
	//	LOGWARN("dispatch channel=%s.", channel);
		pData = redisData.getData(enSubPubReplyData);
		if (nullptr == pData) {
			LOGWARN("dispatch cann't find data");
			return;
		}

		uv_mutex_lock(&m_mutexMsgs);
		auto it = m_mapChan.find(channel);
		if (it == m_mapChan.end()) {
			LOGERROR("dispatch chan(%s) no register", channel.c_str());
			return;
		}

		it->second->addMsg(pData);
		uv_mutex_unlock(&m_mutexMsgs);
	}
}