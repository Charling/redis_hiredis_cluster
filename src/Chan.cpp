#include "Chan.h"
#include "MemoryMgr.h"

namespace redis
{

	Chan::Chan(const std::string& chanId)
		: m_chanId(chanId)
	{
		uv_mutex_init(&m_mutexData);
		m_vecData.clear();
		m_ops_handler.clear();
	}

	Chan::~Chan()
	{
		auto it = m_vecData.begin();
		for (; it != m_vecData.end(); ++it)
		{
			it->destroy();
		}

		m_vecData.clear();
	}

	bool Chan::registerOps(int32 ops, ops_handler func)
	{
		if (m_ops_handler.find(ops) != m_ops_handler.end()) {
			LOGERROR("redis channel(%d) has register!", ops);
			return false;
		}

		m_ops_handler.insert(make_pair(ops, func));
		return true;
	}

	void Chan::polling()
	{
		uv_mutex_lock(&m_mutexData);
		auto it = m_vecData.begin();
		for (; it != m_vecData.end(); ++it)
		{
			stData& data = *it;
			static Message msg;
			msg.Clear();

			if (msg.ParseFromArray(data.data, data.len)) {
				auto itor = m_ops_handler.find(msg.ops());
				if (itor == m_ops_handler.end()) {
					LOGERROR("msg channel ops(%d) has not register !", msg.ops());
				}
				else {
					int64 playerId = msg.has_playerid() ? msg.playerid() : 0;
					auto data = msg.data();
					ops_handler func = itor->second;
					auto size = msg.size();
					func(playerId, data.c_str(), size);
				}

			}
			else {
				LOGERROR("chan ParseFromArray failed ...");
			}
			it->destroy();
		}

		m_vecData.clear();
		uv_mutex_unlock(&m_mutexData);
	}

	std::string Chan::getChanId()
	{
		return m_chanId;
	}

	void Chan::addMsg(const stData* pData)
	{
		uv_mutex_lock(&m_mutexData);
		stData temp;
		temp.data = (char*)allocateMemory(pData->len);
		memset(temp.data, 0, pData->len);
		memcpy(temp.data, pData->data, pData->len);
		temp.len = pData->len;
	//	LOGWARN("addMsg: data:%s,len:%d.", pData->data, pData->len);
		m_vecData.push_back(temp);
		uv_mutex_unlock(&m_mutexData);
	}

} //end namespace redis
