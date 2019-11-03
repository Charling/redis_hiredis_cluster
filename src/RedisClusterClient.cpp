#include "RedisClusterClient.h"
#include "LogMgr.h"
#include "TimerHelper.h"
#include "Chan.h"
#include "MemoryMgr.h"
#include "Function.h"

namespace redis
{

	RedisClusterClient::RedisClusterClient()
	{
		m_cfgs.clear();
	}

	RedisClusterClient::~RedisClusterClient()
	{

	}

	void RedisClusterClient::init(stl_vector<stRedisClusterCfg>& cfgs)
	{
		m_cfgs = cfgs;
	}

	bool RedisClusterClient::start()
	{
		uv_thread_create(&m_thread, &RedisClusterClient::redisClient, (void*)this);

		return true;
	}

	void RedisClusterClient::redisClient(void* client)
	{
		RedisClusterClient* obj = (RedisClusterClient*)client;
		if (obj == nullptr) {
			LOGERROR("start thread error obj == nullptr");
			return;
		}

		redisReply* reply = nullptr;
		stRedisData redisData;

		obj->connect();

		while (1)
		{
			if (!obj->ping()) {
				obj->connect();
			}

			BASE::dSleep(1000);
		}

		redisClusterConnRelease(&obj->m_sendText);
	}

	void RedisClusterClient::setValue(const std::string& key, int64 value)
	{
		// SET %s %s
		const int32 cnt = 3;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("SET");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "SET", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());
		i++;
		std::string values = BASE::ToString(value);
		argvLen[i] = values.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, values.length());
		memcpy(argv[i], values.c_str(), values.length());

		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClusterClient::getValue(const std::string& key, int64& value)
	{
		// GET %s 
		const int32 cnt = 2;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("GET");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "GET", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());
		
		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr) {
			freeReplyObject(reply);
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

	bool RedisClusterClient::ping()
	{
		{//m_sendText
			// PING 
			const int32 cnt = 1;
			// locate memory
			size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
			memset(argvLen, 0, sizeof(size_t) * cnt);

			char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
			memset(argv, 0, sizeof(char*) * cnt);

			int32 i = 0;
			argvLen[i] = strlen("PING");
			argv[i] = (char*)allocateMemory(argvLen[i]);
			memset(argv[i], 0, argvLen[i]);
			memcpy(argv[i], "PING", argvLen[i]);

			auto reply = redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
			//printf("PING: %s\n", reply->str);
			if (reply != nullptr)
				freeReplyObject(reply);
		}

		return true;
	}

	void RedisClusterClient::expire(const std::string& key, int32 cacheTime)
	{
		// EXPIRE %s %s
		const int32 cnt = 3;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("EXPIRE");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "EXPIRE", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());
		i++;
		std::string values = BASE::ToString(cacheTime);
		argvLen[i] = values.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, values.length());
		memcpy(argv[i], values.c_str(), values.length());

		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClusterClient::persist(const std::string& key)
	{
		// PERSIST %s
		const int32 cnt = 2;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("PERSIST");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "PERSIST", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());
	
		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClusterClient::del(const std::string& key)
	{
		// DEL %s
		const int32 cnt = 2;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("DEL");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "DEL", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());

		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClusterClient::setValue(const std::string& key, std::string value)
	{
		// SET %s %s
		const int32 cnt = 3;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("SET");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "SET", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());
		i++;

		argvLen[i] = value.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, value.length());
		memcpy(argv[i], value.c_str(), value.length());

		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr)
			freeReplyObject(reply);
	}

	void RedisClusterClient::getValue(const std::string& key, std::string& value)
	{
		// GET %s 
		const int32 cnt = 2;
		// locate memory
		size_t* argvLen = (size_t*)allocateMemory(sizeof(size_t) * cnt);
		memset(argvLen, 0, sizeof(size_t) * cnt);

		char** argv = (char**)allocateMemory(sizeof(char*) * cnt);
		memset(argv, 0, sizeof(char*) * cnt);

		int32 i = 0;
		argvLen[i] = strlen("GET");
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], "GET", argvLen[i]);
		i++;

		argvLen[i] = key.length();
		argv[i] = (char*)allocateMemory(argvLen[i]);
		memset(argv[i], 0, argvLen[i]);
		memcpy(argv[i], key.c_str(), key.length());

		auto reply = (redisReply*)redisClusterConnCommand(&m_sendText, cnt, argv, argvLen);
		if (reply != nullptr) {
			freeReplyObject(reply);
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

	bool RedisClusterClient::connect()
	{
		auto size = (int)m_cfgs.size();

		redisClusterConnInit(&m_sendText, size, nullptr, 100, 100);
		for (int i = 0; i < size; i++) {
			auto cfg = m_cfgs.at(i);
			redisClusterConnSetNode(&m_sendText, i, cfg.ip.c_str(), cfg.port);
		}
		return true;
	}

} // end namespace redis