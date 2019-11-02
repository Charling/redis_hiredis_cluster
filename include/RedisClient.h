//==========================================================================
/**
* @file	 : RedisClient.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2018-08-23 20:03
* purpose : redis基础类
*/
//==========================================================================
#ifndef __redis_client_h__
#define __redis_client_h__

#include "RedisData.h"
#include "Chan.h"
#include "Function.h"

namespace redis
{
#define MaxRedisKey 64

	//The base from which all redis client classes are derived.
	class RedisClient
	{
	public:
		RedisClient();
		virtual ~RedisClient();
		virtual bool start();
		virtual bool startnow();
		virtual void dispatch(redisContext* c, const stRedisData& redisData);
		virtual void polling();

	public:
		void init(const char* ip, int port);
		bool registerChan(Chan* chan);
		void unregisterChan(const std::string& chanId);

		bool connect(redisContext** c);
		bool reconnect(redisContext** c);

		int parse(redisReply* r, stRedisData& redisData, int32 idx);

		void registerOps(int32 ops, ops_handler func);

		static void redisclient(void* client);

		//key-value:
		/*
			说明：key为string,value为json结构

			序列化json:
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

			//添加元素

			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			doc.Accept(writer);

			auto data = buffer.GetString();

			反序列化json:
			rapidjson::Document doc;
			if (!doc.Parse(data.c_str()).HasParseError()
			&& doc.IsObject()) {
			}
		*/
		void setValue(const std::string& key, std::string value);
		void getValue(const std::string& key, std::string& value);

		template<typename T>
		bool setValue(const std::string& key, const T& msg)
		{
			if (m_sendText == nullptr) {
				LOGERROR("setValue m_recvText == nullptr.");
				return false;
			}

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

			redisReply* reply = (redisReply *)redisCommandArgv(m_sendText, cnt, (const char**)argv, argvLen);

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
			if (m_recvText == nullptr) {
				LOGERROR("getValue m_recvText == nullptr.");
				return false;
			}

			auto reply = (redisReply*)redisCommand(m_sendText, "GET %s", key.c_str());
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
		// recv
		redisContext* m_recvText;
		// send
		redisContext* m_sendText;

		//<channelId, channel>
		stl_map<std::string, Chan*> m_mapChan;
	
		std::string m_ip;
		int32 m_port;

		uv_thread_t m_thread;
	};

} //end namespace redis

#endif // __redis_client_h__