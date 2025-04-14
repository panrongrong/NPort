#include <vxWorks.h>
#include <stdio.h>
#include <sockLib.h>
#include <inetLib.h>
#include <taskLib.h>
#include <socket.h>
#include <stdlib.h>
#include <sysLib.h>
#include <logLib.h>
#include <errno.h>
#include <string.h>
#include "wdLib.h"
#include "in.h"
#include "ioLib.h"
#include <unistd.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>


// 定义状态
typedef enum {
	STATE_INIT,
	STATE_LISTENING,
	STATE_HANDLING_CLIENT,
	STATE_ERROR
} ServerState;

// 定义事件
typedef enum {
	EVENT_INIT_SUCCESS,
	EVENT_CLIENT_CONNECTED,
	EVENT_CLIENT_DISCONNECTED,
	EVENT_ERROR_OCCURRED
} ServerEvent;

// 状态机结构体
typedef struct {
	ServerState currentState;
} ServerStateMachine;

// 状态处理函数
void TCPhandleInit(ServerStateMachine *stateMachine, ServerEvent event) {
	switch (event) {
	case EVENT_INIT_SUCCESS:
		stateMachine->currentState = STATE_LISTENING;
		//printf("Server is now listening on port %d\n", SERVER_PORT);
		break;
	case EVENT_ERROR_OCCURRED:
		stateMachine->currentState = STATE_ERROR;
		//printf("Error occurred during initialization\n");
		break;
	default:
		break;
	}
}

void handleListening(ServerStateMachine *stateMachine, ServerEvent event) {
	switch (event) {
	case EVENT_CLIENT_CONNECTED:
		stateMachine->currentState = STATE_HANDLING_CLIENT;
		//printf("Client connected, handling client...\n");
		break;
	case EVENT_ERROR_OCCURRED:
		stateMachine->currentState = STATE_ERROR;
		//printf("Error occurred while listening\n");
		break;
	default:
		break;
	}
}

void handleHandlingClient(ServerStateMachine *stateMachine, ServerEvent event) {
	switch (event) {
	case EVENT_CLIENT_DISCONNECTED:
		stateMachine->currentState = STATE_LISTENING;
		//printf("Client disconnected, back to listening state\n");
		break;
	case EVENT_ERROR_OCCURRED:
		stateMachine->currentState = STATE_ERROR;
		//printf("Error occurred while handling client\n");
		break;
	default:
		break;
	}
}

void handleError(ServerStateMachine *stateMachine, ServerEvent event) {
	// 可以添加错误恢复逻辑
	stateMachine->currentState = STATE_INIT;
	//printf("Trying to re-initialize the server...\n");
}

// 状态机处理函数
void stateMachineHandleEvent(ServerStateMachine *stateMachine, ServerEvent event) {
	switch (stateMachine->currentState) {
	case STATE_INIT:
		TCPhandleInit(stateMachine, event);
		break;
	case STATE_LISTENING:
		handleListening(stateMachine, event);
		break;
	case STATE_HANDLING_CLIENT:
		handleHandlingClient(stateMachine, event);
		break;
	case STATE_ERROR:
		handleError(stateMachine, event);
		break;
	default:
		break;
	}
}
