//==========================================================================
/**
* @file	 : RedisClusterClient.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2019-11-03 20:03
* purpose : redis集群基础类
*/
//==========================================================================
#ifndef __RedisClusterClient_h__
#define __RedisClusterClient_h__

#include "RedisData.h"
#include "Chan.h"
#include "Function.h"
#include "redisClusterConn.h"

namespace redis
{
	struct stRedisClusterCfg {
		std::string ip;
		int port;

		stRedisClusterCfg() {
			clear();
		}

		inline void clear() {
			ip = "";
			port = 0;
		}
	};

	class RedisClusterClient
	{
	public:
		RedisClusterClient();
		virtual ~RedisClusterClient();
		virtual bool start();

	public:
		void init(stl_vector<stRedisClusterCfg>& cfgs);

		bool connect();

		static void redisClient(void* client);

		void setValue(const std::string& key, std::string value);
		void getValue(const std::string& key, std::string& value);

		template<typename T>
		bool setValue(const std::string& key, const T& msg)
		{
			auto size = msg.ByteSize();
			auto data = (char*)allocateMemory(size);
			if (!msg.SerializeToArray(data, size)) {
				deallocateMemory(data);
				LOGERROR("setValue SerializePartialToArray failed.");
				return false;
			}

			// set %s %s
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
			argvLen[i] = size;
			argv[i] = (char*)allocateMemory(argvLen[i]);
			memset(argv[i], 0, argvLen[i]);
			memcpy(argv[i], data, size);

			redisReply* reply = (redisReply *)RedisClusterConn::redisClusterConnCommand(&m_sendText, cnt, (const char**)argv, argvLen);

			// free memory
			deallocateMemory(argvLen);
			for (int k = 0; k < cnt; k++) {
				deallocateMemory(argv[k]);
			}
			deallocateMemory(argv);

			if (reply != nullptr)
				freeReplyObject(reply);
			deallocateMemory(data);
			return true;
		}


		template<typename T>
		bool getValue(const std::string& key, T& msg)
		{
			auto reply = (redisReply*)RedisClusterConn::redisClusterConnCommand(&m_sendText, "GET %s", key.c_str());
			if (reply == nullptr) {
				LOGERROR("getValue reply == nullptr...");
				return false;
			}

			switch (reply->type)
			{
			case REDIS_REPLY_STRING:
				msg.Clear();
				if (!msg.ParseFromArray(reply->str, reply->len)) {
					LOGERROR("getValue ParseFromArray failed...");
				}
				else {
					if (reply != nullptr)
						freeReplyObject(reply);
					return true;
				}
				break;
			case REDIS_REPLY_NIL:
				LOGINFO("GET %s is nil", key.c_str());
				break;
			default:
				LOGERROR("reply type is wrong key=%s，type=%d...", key.c_str(), reply->type);
				break;
			}
			if (reply != nullptr)
				freeReplyObject(reply);
			return false;
		}

		void setValue(const std::string& key, int64 value);
		void getValue(const std::string& key, int64& value);

		void expire(const std::string& key, int32 cacheTime);
		void persist(const std::string& key);
		void del(const std::string& key);

		bool ping();

	protected:
		// send
		redisClusterConn m_sendText;

		stl_vector<stRedisClusterCfg> m_cfgs;
		uv_thread_t m_thread;
	};

} //end namespace redis

#endif // __RedisClusterClient_h__