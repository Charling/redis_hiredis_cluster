#include "RedisClient.h"
#include "LogMgr.h"
#include "TimerHelper.h"
#include "Chan.h"
#include "MemoryMgr.h"
#include "Function.h"

namespace redis
{

	RedisClient::RedisClient()
	{
		m_recvText = nullptr;
		m_sendText = nullptr;
		m_mapChan.clear();

		m_ip = "";
		m_port = 0;
	}

	RedisClient::~RedisClient()
	{

	}

	void RedisClient::init(const char* ip, int port)
	{
		m_ip = ip;
		m_port = port;
	}

	bool RedisClient::start()
	{
		uv_thread_create(&m_thread, &RedisClient::redisclient, (void*)this);

		return true;
	}

	bool RedisClient::startnow()
	{
		do 
		{
			if (m_recvText == nullptr) {
				if (!connect(&m_recvText)) {
					LOGERROR("connect redis failed.");
					BASE::dSleep(200);
					continue;
				}
			}

			if (m_sendText == nullptr) {
				if (!connect(&m_sendText)) {
					LOGERROR("connect redis failed.");
					BASE::dSleep(200);
					continue;
				}
			}

			break;
		} while (1);
		return true;
	}

	void RedisClient::redisclient(void* client)
	{
		RedisClient* obj = (RedisClient*)client;
		if (obj == nullptr) {
			LOGERROR("start thread error obj == nullptr");
			return;
		}

		redisReply* reply = nullptr;
		stRedisData redisData;

		while (1)
		{
			if (obj->m_recvText == nullptr) {
				if (!obj->connect(&obj->m_recvText)) {
					LOGERROR("connect redis failed.");
					BASE::dSleep(200);
				}
				continue;
			}

			if (obj->m_sendText == nullptr) {
				if (!obj->connect(&obj->m_sendText)) {
					LOGERROR("connect redis failed.");
					BASE::dSleep(200);
				}

				continue;
			}

			BASE::dSleep(1000);
		}

		LOGWARN("Subscriber server stop...");
		if (obj->m_recvText != nullptr) {
			redisFree(obj->m_recvText);
		}

		if (obj->m_sendText != nullptr) {
			redisFree(obj->m_sendText);
		}
	}

	void RedisClient::dispatch(redisContext* c, const stRedisData& redisData)
	{

	}

	void RedisClient::polling()
	{

	}

	void RedisClient::setValue(const std::string& key, int64 value)
	{
		if (m_sendText == nullptr) {
			LOGERROR("m_sendText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_sendText, "SET %s %d", key.c_str(), value);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClient::getValue(const std::string& key, int64& value)
	{
		if (m_recvText == nullptr) {
			LOGERROR("m_recvText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_recvText, "GET %s", key.c_str());
		if (reply == nullptr) {
			LOGERROR("reply == nullptr...");
			return;
		}
		switch (reply->type)
		{
		case REDIS_REPLY_STRING:
			value = BASE::StringToNumber<int64>(reply->str);
			break;
		case REDIS_REPLY_NIL:
			LOGINFO("GET %s is nil", key.c_str());
			break;
		default:
			LOGERROR("reply type is wrong key=%s£¬type=%d...", key.c_str(), reply->type);
			break;
		}
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	bool RedisClient::ping()
	{
		{//m_sendText
			if (m_sendText == nullptr) {
				LOGERROR("m_sendText == nullptr.");
				return false;
			}
			auto reply = redisCommand(m_sendText, "PING");
			//printf("PING: %s\n", reply->str);
			if (reply != nullptr)
				freeReplyObject(reply);
		}
		{//m_recvText
			if (m_recvText == nullptr) {
				LOGERROR("m_sendText == nullptr.");
				return false;
			}
			auto reply = redisCommand(m_recvText, "PING");
			//printf("PING: %s\n", reply->str);
			if (reply != nullptr)
				freeReplyObject(reply);
		}
		return true;
	}

	void RedisClient::expire(const std::string& key, int32 cacheTime)
	{
		if (m_sendText == nullptr) {
			LOGERROR("m_sendText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_sendText, "EXPIRE %s %d", key.c_str(), cacheTime);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClient::persist(const std::string& key)
	{
		if (m_sendText == nullptr) {
			LOGERROR("m_sendText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_sendText, "PERSIST %s", key.c_str());
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClient::del(const std::string& key)
	{
		if (m_sendText == nullptr) {
			LOGERROR("m_sendText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_sendText, "DEL %s", key.c_str());
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClient::setValue(const std::string& key, std::string value)
	{
		if (m_sendText == nullptr) {
			LOGERROR("m_sendText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_sendText, "SET %s %s", key.c_str(), value.c_str());
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClient::getValue(const std::string& key, std::string& value)
	{
		if (m_recvText == nullptr) {
			LOGERROR("m_recvText == nullptr.");
			return;
		}
		auto reply = (redisReply*)redisCommand(m_recvText, "GET %s", key.c_str());
		if (reply == nullptr) {
			LOGERROR("reply == nullptr...");
			return;
		}
		switch (reply->type)
		{
		case REDIS_REPLY_STRING:
			value = reply->str;
			break;
		case REDIS_REPLY_NIL:
			LOGINFO("GET %s is nil", key.c_str());
			break;
		default:
			LOGERROR("reply type is wrong key=%s£¬type=%d...", key.c_str(), reply->type);
			break;
		}
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	bool RedisClient::registerChan(Chan* chan)
	{
		IF_NOT_RETURN_VALUE(chan, false);

		if (m_mapChan.find(chan->getChanId()) != m_mapChan.end()) {
			LOGERROR("redis channel(%s) has register!", chan->getChanId().c_str());
			return false;
		}

		m_mapChan.insert(make_pair(chan->getChanId(), chan));
		return true;
	}

	void RedisClient::unregisterChan(const std::string& chanId)
	{
		auto it = m_mapChan.find(chanId);
		if (it != m_mapChan.end()) {
			safeDeleteObject(it->second, Chan);
			m_mapChan.erase(it);
		}
		else {
			LOGERROR("redis channel(%s) has register!", chanId.c_str());
		}
	}

	bool RedisClient::connect(redisContext** c)
	{
		timeval timeoutcfg = { 1, 500000 };
		*c = redisConnectWithTimeout(m_ip.c_str(), m_port, timeoutcfg);
		if (nullptr == *c || (*c)->err) {
			if (nullptr != *c) {
				LOGERROR("Connection error: %s\n", (*c)->errstr);
				redisFree(*c);
				*c = nullptr;
			}else {
				LOGERROR("Connection error: can't allocate redis context\n");
			}

			return false;
		}

		return true;
	}

	bool RedisClient::reconnect(redisContext** c)
	{
		if (nullptr != *c) {
			redisFree(*c);
		}

		struct timeval timeout = { 1, 500000 };
		*c = redisConnectWithTimeout(m_ip.c_str(), m_port, timeout);
		if (nullptr == *c || (*c)->err) {
			if (nullptr != *c) {
				LOGERROR("reconnection error: %s\n", (*c)->errstr);
			}else {
				LOGERROR("reconnection error: can't allocate redis context\n");
			}

			return false;
		}

		return true;
	}

	void RedisClient::registerOps(int32 ops, ops_handler func)
	{
		for (auto it = m_mapChan.begin(); it != m_mapChan.end(); ++it)
			(*it).second->registerOps(ops, func);
	}

	int32 RedisClient::parse(redisReply* r, stRedisData& redisData, int32 idx)
	{
		switch (r->type)
		{
		case REDIS_REPLY_ERROR:
			LOGERROR("reply error type:%d, %s\n", r->type, r->str);
			break;
		case REDIS_REPLY_STATUS:
			break;
		case REDIS_REPLY_INTEGER:
			break;
		case REDIS_REPLY_STRING:
		{
			redisData.insert(idx, r->str, r->len);
		} break;
		case REDIS_REPLY_NIL:
			break;
		case REDIS_REPLY_ARRAY:
		{
			for (int i = 0; i < r->elements; i++) {
				parse(r->element[i], redisData, i + 1);
			}
		}break;
		default:
			fprintf(stderr, "Unknown reply type: %d\n", r->type);
		}

		return r->type;
	}

} // end namespace redis