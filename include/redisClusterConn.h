//==========================================================================
/**
* @file	 : redisClusterConn.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2019-11-02 20:03
* purpose : redis cluster 集群连接 connection
*/
//==========================================================================
#ifndef __redisClusterConn_h__
#define __redisClusterConn_h__

#ifdef WIN32 
#include <time.h>
#include "winsock2.h"
#else
#include <sys/time.h> 
#endif

namespace redis
{
#include "hiredis.h"
#ifdef __cplusplus
	extern "C" {
#endif
#define MaxPasswordLen 33

		typedef struct redisClusterNode
		{
			int index;
			int slot;
			int port;
			char* host;

			redisContext* redctx;
		} redisClusterNode;


		typedef struct redisClusterConn
		{
			redisClusterNode* node;

			struct timeval conn;
			struct timeval data;

			char password[MaxPasswordLen];
			int num;

			redisClusterNode* nodes;
		} redisClusterConn;


		extern int redisClusterConnInit(redisClusterConn* conn, int nodesNum, const char* password, int connTimeMs, int dataTimeoMs);
		extern void redisClusterConnRelease(redisClusterConn* conn);
		extern void redisClusterConnSetNode(redisClusterConn* conn, int nodeIndex, const char* host, int port);
		extern void redisClusterConnCloseNode(redisClusterConn* conn, int nodeIndex);
		extern redisContext* redisClusterConnOpenNode(redisClusterConn* conn, int nodeIndex);
		extern redisContext* redisClusterConnGetActiveContext(redisClusterConn* conn, const char* host, int port);
		extern redisReply* redisClusterConnCommand(redisClusterConn* conn, int argc, char** argv, size_t* argvlen);


#ifdef __cplusplus
	}
#endif
}

#endif // __redisClusterConn_h__