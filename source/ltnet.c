/*
* ltnet.c v1.1
* Lua TCP Network Library
* Eduardo Gimenez <eduardo.gimenez07@gmail.com>
* 05 Jun 2020 23:52
*/

// Defs
#undef UNICODE
#define uint unsigned int
#define WIN32_LEAN_AND_MEAN

// Includes
#include "lua.h"
#include "lauxlib.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Pragmas
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

// Lua 5.1 compatibility
#if LUA_VERSION_NUM < 502
// From Lua 5.3 lauxlib.c
LUALIB_API void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
  luaL_checkstack(L, nup, "too many upvalues");
  for (; l->name != NULL; l++) {  /* fill the table with given functions */
    int i;
    for (i = 0; i < nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
    lua_setfield(L, -(nup + 2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}
LUALIB_API void *luaL_testudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      luaL_getmetatable(L, tname);  /* get correct metatable */
      if (!lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}
#endif

// LTNet Functions

//* Server:Client
// :write(buffer->string)
static int ltnet_sclient_write(lua_State *L) {
	// get client socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	// expect string message as 1st argument
	const char *message = luaL_checkstring(L, 2);
	
	// check if sending a message not got any error (if got return nil, else return true)
	if (send(ltsocket, message, strlen(message), 0) == SOCKET_ERROR)
		return 0;
	
	lua_pushboolean(L, 1);
	return 1;
}
// :recv(buffer length->int);
static int ltnet_sclient_recv(lua_State *L) {
	// get client socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	// expect buffer size (integer) as 1st argument
	int buffsize = luaL_checkinteger(L, 2);
	// initialize data message var
	char message[2048];
	// call recv sock function
	int iResult = recv(ltsocket, message, 2048, 0);
	//* check result
	// iResult > 0 = data received
	if (iResult > 0) {
		lua_pushboolean(L, 1);
		lua_pushstring(L, message);
		lua_pushinteger(L, iResult);
	// iResult == 0 = connection closed
	} else if (iResult == 0) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "closed");
		lua_pushinteger(L, 0);
	// other results = error
	} else {
		char *error = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
					   NULL, WSAGetLastError(),
					   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					   (LPWSTR)&error, 0, NULL);
		lua_pushboolean(L, 0);
		lua_pushstring(L, error);
		lua_pushinteger(L, 0);
	}
	return 3;
}
// :close()
static int ltnet_sclient_close(lua_State *L) {
	// get client socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	// Close ltsocket
	closesocket(ltsocket);
	return 0;
}

// server:client lib
luaL_Reg ltnet_sclient_lib[] = {
	{ "write", ltnet_sclient_write },
	{ "recv", ltnet_sclient_recv },
	{ "close", ltnet_sclient_close },
	{ NULL, NULL }
};

//* Server
// :bind(port->int)
static int ltnet_server_bind(lua_State *L) {
	// get socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	
	// check if the first argument is an integer and returns it
	unsigned short port = (unsigned short) luaL_checkinteger(L, 2);
	
	// make address structure
	struct sockaddr_in address;
	
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	// Set address port
	address.sin_port = htons(port);
	// Set address name > *
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Check if binding address got any error
	if (bind(ltsocket, (struct sockaddr *) &address, sizeof(address)) == SOCKET_ERROR) {
		WSACleanup();
		return 0;
	// Else not just check if listening got any error
	} else if (listen(ltsocket, 5) == SOCKET_ERROR) {
		WSACleanup();
		return 0;
	}
	
	// If doesn't get any error just return true
	lua_pushboolean(L, 1);
	return 1;
}
// :accept()
static int ltnet_server_accept(lua_State *L) {
	// get socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	
	// make address structure
	struct sockaddr_in address;
	int address_length = sizeof(address);
	
	// try to accept any pending connection
	
	int ltsocket_client = accept(ltsocket, (struct sockaddr *) &address, &address_length);
	
	// Check if acception not got any error
	if(ltsocket_client != INVALID_SOCKET)
	{
		// New metatable LTN: Client
		luaL_newmetatable(L, "LTN: Client");
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		// Register Client Socket in LTN: Client
		lua_pushstring(L, "__SOCKET");
		lua_pushinteger(L, ltsocket_client);
		lua_settable(L, -3);
		// Register Client ipAddress in LTN: Client
		lua_pushstring(L, "ipAddress");
		lua_pushstring(L, inet_ntoa(address.sin_addr));
		lua_settable(L, -3);
		// Register LTN: Client functions
		luaL_setfuncs(L, ltnet_sclient_lib, 0);
		return 1;
	}
	
	return 0;
}
// :close()

static int ltnet_server_close(lua_State *L) {
	// get socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	// Clean up WSA
	WSACleanup();
	// Close ltsocket
	closesocket(ltsocket);
	return 0;
}

// server lib
luaL_Reg ltnet_server_lib[] = {
	{ "bind", ltnet_server_bind },
	{ "accept", ltnet_server_accept },
	{ "close", ltnet_server_close },
	{ NULL, NULL }
};

// .server();
static int ltnet_server(lua_State *L) {
	// New TCP Socket
	int ltsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Check if socket is invalid or got any error
	if (ltsocket == INVALID_SOCKET) {
		WSACleanup();
	} else {
		// New metatable LTN: Server
		luaL_newmetatable(L, "LTN: Server");
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		// Register the socket in LTN: Server
		lua_pushstring(L, "__SOCKET");
		lua_pushinteger(L, ltsocket);
		lua_settable(L, -3);
		// Register LTN: Server functions
		luaL_setfuncs(L, ltnet_server_lib, 0);
		return 1;
	}
	
	return 0;
}

//* Client
// :connect(host->string, port->int);
static int ltnet_client_connect(lua_State *L) {
	// get client socket from instance
	lua_getfield(L, 1, "__SOCKET");
	int ltsocket = luaL_checkinteger(L, -1);
	// expect string localhost as 1st argument
	const char *host = luaL_checkstring(L, 2);
	// expect int port as 2st argument
	unsigned short port = (unsigned short) luaL_checkinteger(L, 3);
	
	// make address structure
	struct sockaddr_in address;
	
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(host);

	// if connect fail return nil
	if (connect(ltsocket, (struct sockaddr *) &address, sizeof(address)) == SOCKET_ERROR)
		return 0;
	// else return true
	lua_pushboolean(L, 1);
	return 1;
}

// client lib
luaL_Reg ltnet_client_lib[] = {
	{ "connect", ltnet_client_connect },
	{ "write", ltnet_sclient_write },
	{ "recv", ltnet_sclient_recv },
	{ "close", ltnet_sclient_close },
	{ NULL, NULL }
};

// .client();
static int ltnet_client(lua_State *L) {
	// New TCP Socket
	int ltsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Check if socket is invalid or got any error
	if (ltsocket == INVALID_SOCKET) {
		WSACleanup();
	} else {
		// New metatable LTN: Server
		luaL_newmetatable(L, "LTN: Client");
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		// Register the socket in LTN: Server
		lua_pushstring(L, "__SOCKET");
		lua_pushinteger(L, ltsocket);
		lua_settable(L, -3);
		// Register LTN: Server functions
		luaL_setfuncs(L, ltnet_client_lib, 0);
		return 1;
	}
	
	return 0;
}

//* Garbage collector
static int ltnet_gc(lua_State *L) {
	return 0;
}

// ltnet lib	
luaL_Reg ltnet_lib[] = {
	{ "server", ltnet_server },
	{ "client", ltnet_client },
	{ NULL, NULL }
};

__declspec(dllexport)
LUALIB_API int luaopen_ltnet(lua_State *L)
{
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 0), &wsa_data) != 0)
		printf("WSAStartup() failed\n");

	luaL_newmetatable(L, "ltnet");
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, ltnet_gc);
	lua_settable(L, -3);

	lua_newtable(L);
	luaL_setfuncs(L, ltnet_lib, 0);
	lua_pushliteral(L, "Lua TCP Network Library");
	lua_setfield(L, -2, "_NAME");
	lua_pushliteral(L, "1.1");
	lua_setfield(L, -2, "_VERSION");
	return 1;
}
