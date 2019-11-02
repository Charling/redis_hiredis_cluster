//==========================================================================
/**
* @file	 : RedisData.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2018-08-23 20:03
* purpose : redis使用到的结构
*/
//==========================================================================
#ifndef __redis_data_h__
#define __redis_data_h__

#include "hiredis.h"
#include "CommonDef.h"
#include "allocator.h"
#include "nedmalloc.h"
#include "RedisDefData.h"

namespace redis
{
	//redis单条sub/pub推送的消息对应数组下标
	enum enSubPubReplyIdx
	{
		enSubPubReplyNull,
		enSubPubReplyMessage,
		enSubPubReplyChannel,
		enSubPubReplyData,
	};

	//redis单条list推送的消息对应数组下标
	enum enListReplyIdx
	{
		enListReplyNull,
		enListReplyChannel,
		enListReplyData,
	};

#pragma pack(push)
#pragma pack(1)

	struct stData
	{
		char* data;
		int len;
		stData()
		{
			data = nullptr;
			len = 0;
		}

		inline void destroy()
		{
			if (nullptr != data) {
				nedalloc::nedfree(data);
				data = nullptr;
				len = 0;
			}
		}
	};

#pragma pack(pop)

	struct stRedisData
	{
		//<REDIS_REPLY_ARRAY idx, data>
		std::map<int32, stData> mapData;
		stRedisData()
		{
			clear();
		}
		void insert(const int32 idx, const char* data, const int32 len)
		{
			auto it = mapData.find(idx);
			if (it == mapData.end()) {
				stData tmp;
				tmp.data = (char*)nedalloc::nedmalloc(len + 1);
				memset(tmp.data, 0, len + 1);
				memcpy(tmp.data, data, len);
				tmp.len = len;
				mapData.insert(std::make_pair(idx, tmp));
			}
		}

		const stData* getData(const int32 idx) const
		{
			auto it = mapData.find(idx);
			if (it != mapData.end()) {
				return &it->second;
			}
			else {
				return nullptr;
			}
		}
		void clear()
		{
			auto it = mapData.begin();
			for (; it != mapData.end(); ++it)
			{
				it->second.destroy();
			}

			mapData.clear();
		}
	};

} //end namespace redis

#endif // __redis_data_h__
