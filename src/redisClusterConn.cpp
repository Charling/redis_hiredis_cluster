
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "redisClusterConn.h"
#include "LogMgr.h"
#include "MemoryMgr.h"

namespace redis
{
	int redisClusterConnInit(redisClusterConn* conn, int maxNodes, const char * password, int connTimeMs, int dataTimeoMs)
	{
		size_t authlen = 0;

		if (password) {
			authlen = strlen(password);
			if (authlen >= sizeof(conn->password)) {
				return -1;
			}
		}

		memset(conn, 0, sizeof(redisClusterConn));

		conn->nodes = (redisClusterNode *)nedalloc::nedcalloc(maxNodes, sizeof(redisClusterNode));
		conn->num = maxNodes;
		do {
			int i;
			for (i = 0; i < maxNodes; ++i) {
				conn->nodes[i].index = i;
			}
		} while (0);

		if (connTimeMs) {
			conn->conn.tv_sec = connTimeMs / 1000;
			conn->conn.tv_usec = connTimeMs * 1000;
		}

		if (dataTimeoMs) {
			conn->data.tv_sec = dataTimeoMs / 1000;
			conn->data.tv_usec = dataTimeoMs * 1000;
		}

		if (authlen) {
			memcpy(conn->password, password, authlen);
		}

		return 0;
	}

	void redisClusterConnRelease(redisClusterConn* conn)
	{
		while (conn->num-- > 0) {
			char* hp = conn->nodes[conn->num].host;
			redisClusterConnCloseNode(conn, conn->num);

			if (hp) {
				conn->nodes[conn->num].host = 0;

				nedalloc::nedfree(hp);
			}
		}

		nedalloc::nedfree(conn->nodes);
		conn->nodes = 0;
	}

	void redisClusterConnSetNode(redisClusterConn* conn, int nodeIndex, const char *host, int port)
	{
		assert(conn->nodes[nodeIndex].index == nodeIndex);

		conn->nodes[nodeIndex].host = (char *)nedalloc::nedcalloc(strlen(host) + 1, sizeof(char));
		memcpy(conn->nodes[nodeIndex].host, host, strlen(host));

		conn->nodes[nodeIndex].port = port;
	}

	void redisClusterConnCloseNode(redisClusterConn* conn, int index)
	{
		redisContext * ctx = conn->nodes[index].redctx;
		conn->nodes[index].redctx = 0;
		if (ctx) {
			if (conn->node == &(conn->nodes[index])) {
				conn->node = 0;
			}
			redisFree(ctx);
		}
	}

	redisContext* redisClusterConnOpenNode(redisClusterConn* conn, int index)
	{
		redisClusterConnCloseNode(conn, index);

		do {
			redisClusterNode* node = &(conn->nodes[index]);
			redisContext* ctx = 0;

			if (conn->conn.tv_sec || conn->conn.tv_usec) {
				ctx = redisConnectWithTimeout(node->host, node->port, conn->conn);
			}
			else {
				ctx = redisConnect(node->host, node->port);
			}

			if (ctx) {
				if (ctx) {
					if (conn->data.tv_sec || conn->data.tv_usec) {
						if (redisSetTimeout(ctx, conn->data) == REDIS_ERR) {
							redisFree(ctx);
							ctx = 0;
						}
					}
				}

				if (ctx) {
					node->redctx = ctx;
					conn->node = node;
				}
			}

			return node->redctx;
		} while (0);

		/* never run to this. */
		return 0;
	}


	redisContext* redisClusterConnGetActiveContext(redisClusterConn* conn, const char* host, int port)
	{
		int index;

		if (!host) {
			if (conn->node && conn->node->redctx) {
				return conn->node->redctx;
			}
			else {
				// 找到第一个活动节点
				conn->node = 0;

				for (index = 0; index < conn->num; index++) {
					if (conn->nodes[index].redctx) {
						conn->node = &(conn->nodes[index]);
						return conn->node->redctx;
					}
				}
			}

			// 全部节点都不是活的, 创建连接. 遇到第一个成功的就返回
			for (index = 0; index < conn->num; index++) {
				if (redisClusterConnOpenNode(conn, index)) {
					return conn->node->redctx;
				}
			}
		}
		else {
			for (index = 0; index < conn->num; index++) {
				redisClusterNode * node = &(conn->nodes[index]);

				if (node->port == port && !strcmp(node->host, host)) {
					if (node->redctx) {
						conn->node = node;
						return conn->node->redctx;
					}
					else {
						return redisClusterConnOpenNode(conn, index);
					}
				}
			}
		}

		// 没有连接可以使用
		return 0;
	}

	redisReply* redisClusterConnCommand(redisClusterConn* conn, int argc, const char** argv, const size_t* argvlen)
	{
		redisReply* reply = 0;

		redisContext* ctx = redisClusterConnGetActiveContext(conn, 0, 0);
		if (ctx == 0) {
			return 0;
		}

		reply = (redisReply *)redisCommandArgv(ctx, argc, argv, argvlen);

		if (!reply) {
			// 执行失败, 连接不能再被使用. 必须建立新连接!!
			redisClusterConnCloseNode(conn, conn->node->index);
			return 0;
		}

		if (reply->type == REDIS_REPLY_ERROR) {
			LOGERROR("REDIS_REPLY_ERROR:%s\n", reply->str);

			if (strstr(reply->str, "MOVED ")) {
				char * start = &(reply->str[6]);
				char * end = strchr(start, 32);

				if (end) {
					start = end;
					++start;
					*end = 0;

					end = strchr(start, ':');

					if (end) {
						*end++ = 0;

						LOGERROR("Redirected to slot [%s] located at [%s:%s]\n", &reply->str[6], start, end);

						ctx = redisClusterConnGetActiveContext(conn, start, atoi(end));

						if (ctx) {
							freeReplyObject(reply);

							return redisClusterConnCommand(conn, argc, argv, argvlen);
						}
					}
				}
			}

			if (strstr(reply->str, "NOAUTH ")) {
				const char * cmds[] = {
					"AUTH",
					conn->password
				};

				freeReplyObject(reply);

				return redisClusterConnCommand(conn, 2, cmds, 0);
			}
		}

		return reply;
	}
}