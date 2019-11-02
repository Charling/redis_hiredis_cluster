//==========================================================================
/**
* @file	 : Chann.h
* @author : Charling(����)/56430114@qq.com
* created : 2018-08-23 20:03
* purpose : ��redis������Ϣ
*/
//==========================================================================
#ifndef __redis_chan_h__
#define __redis_chan_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <async.h>

#include "LogMgr.h"
#include "RedisData.h"

#ifndef WIN32
#include <functional>
#endif // !WIN32

namespace redis
{
	typedef std::function<void(int32 playerId, const char* data, int size)> ops_handler;

	class Chan
	{
	public:
		Chan(const std::string& chanId);
		virtual ~Chan();

		bool registerOps(int32 ops, ops_handler func);
	
		void polling(); 

		std::string getChanId();

		void addMsg(const stData* pData);

		inline void setGameType(int32 gameType) { m_nGameType = gameType; }
	protected:
		stl_vector<stData> m_vecData;
		stl_map<int, ops_handler> m_ops_handler;
	private:
		std::string m_chanId;
		int32 m_nGameType;
		uv_mutex_t m_mutexData;
	};

} //end namespace redis
#endif // __redis_chan_h__