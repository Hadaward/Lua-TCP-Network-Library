/*
* ltnet.c v1.2
* Lua TCP Network Library
* Eduardo Gimenez <eduardo.gimenez07@gmail.com>
* 05 Jul 2020 00:49
*/

#undef UNICODE
#define uint unsigned int
#define WIN32_LEAN_AND_MEAN

#include "lua.h"
#include "lauxlib.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

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
// Extra function [tmrecv]
int tmrecv(int s, char *buf, int len, int timeout)
{
    fd_set fds;
    int n;
    struct timeval tv;

    // set up the file descriptor set
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // set up the struct timeval for the timeout
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // wait until timeout or data received
    n = select(s+1, &fds, NULL, NULL, &tv);
    if (n == 0) return -2; // timeout!
    if (n == -1) return -1; // error

    // data must be here, so do a normal recv()
    return recv(s, buf, len, 0);
}

// *:settimeout(time milis->int);
static int ltnet_settimeout(lua_State *L) {
	// Get expected number miliseconds as 1st argument [after metatable]
	int ms = luaL_checknumber(L, 2);
	// Set a new timeout value [< 0 = disable, >= 0 = enable, default: -1]
	lua_pushinteger(L, ms);
	lua_setfield(L, 1, "__TIMEOUT");
	return 0;
}

// letnet.client:recv(buffer length->int);
static int ltnet_client_recv(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int lsock = luaL_checkinteger(L, -1);
	// Get the timeout stored in the metatable
	lua_getfield(L, 1, "__TIMEOUT");
	int lttimeout = luaL_checknumber(L, -1);
	// Get expected buffer size (integer) as 1st argument
	int buffsize = luaL_checkinteger(L, 2);
	// Initialize data message var
	char message[2048];
	// Call recv sock function
	int iResult;
	if (lttimeout >= 0) {
		iResult = tmrecv(lsock, message, buffsize, lttimeout);
	} else {
		iResult = recv(lsock, message, buffsize, 0);
	}
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
	// iResult == -2 = timeout
	} else if (iResult == -2) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "timeout");
		lua_pushinteger(L, 0);
	// other results = error
	} else {
		lua_pushboolean(L, 0);
		lua_pushinteger(L, WSAGetLastError());
		lua_pushinteger(L, 0);
	}
	return 3;
}
// ltnet.client():write(buffer->string);
static int ltnet_client_write(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int lsock = luaL_checkinteger(L, -1);
	// Get expected string message as 1st argument
	const char *message = luaL_checkstring(L, 2);
	
	// Sends the message and if any error occurs returns nil otherwise it will return true
	if (send(lsock, message, strlen(message), 0) == SOCKET_ERROR)
		return 0;
	
	lua_pushboolean(L, 1);
	return 1;
}
// ltnet.client():close();
static int ltnet_client_close(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int ltsock = luaL_checkinteger(L, -1);
	// Close socket
	closesocket(ltsock);
	return 0;
}
// ltnet.client():connect(host->string, port->int);
static int ltnet_client_connect(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int lsock = luaL_checkinteger(L, -1);
	// Get expected string localhost as 1st argument
	const char *host = luaL_checkstring(L, 2);
	// Get expect int port as 2nd argument
	unsigned short port = (unsigned short) luaL_checkinteger(L, 3);
	
	// Make address structure
	struct sockaddr_in address;
	
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = inet_addr(host);

	// If connection fail return nil
	if (connect(lsock, (struct sockaddr *) &address, sizeof(address)) == SOCKET_ERROR)
		return 0;
	// else return true
	lua_pushboolean(L, 1);
	return 1;
}
// ltnet_client lib
luaL_Reg ltnet_client_lib[] = {
	{ "connect", ltnet_client_connect },
	{ "settimeout", ltnet_settimeout },
	{ "write", ltnet_client_write },
	{ "recv", ltnet_client_recv },
	{ "close", ltnet_client_close },
	{ NULL, NULL }
};
// ltnet.client();
static int ltnet_client(lua_State *L) {
	// Create a new TCP socket
	int lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Checks if there was an error creating the socket
	if (lsock == INVALID_SOCKET) {
		WSACleanup();
	} else {
		// Create a new metatable
		luaL_newmetatable(L, "Client");
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		// Register the __SOCKET index in the metatable
		lua_pushstring(L, "__SOCKET");
		lua_pushinteger(L, lsock);
		lua_settable(L, -3);
		// Register the __TIMEOUT index in the metatable
		lua_pushstring(L, "__TIMEOUT");
		lua_pushinteger(L, -1);
		lua_settable(L, -3);
		// Define metatable functions
		luaL_setfuncs(L, ltnet_client_lib, 0);
		return 1;
	}
	
	return 0;
}

// ltnet.server():bind(port->int)
static int ltnet_server_bind(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int lsock = luaL_checkinteger(L, -1);
	
	// Get expected number port as 1st argument
	unsigned short port = (unsigned short) luaL_checkinteger(L, 2);
	
	// Make address structure
	struct sockaddr_in address;
	
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	// Set address port
	address.sin_port = htons(port);
	// Define address name > *
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Checks if an error has occurred and returns nil.
	if (bind(lsock, (struct sockaddr *) &address, sizeof(address)) == SOCKET_ERROR) {
		WSACleanup();
		return 0;
	} else if (listen(lsock, 5) == SOCKET_ERROR) {
		WSACleanup();
		return 0;
	}
	
	// Return true
	lua_pushboolean(L, 1);
	return 1;
}
// ltnet.server():accept()
static int ltnet_server_accept(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int lsock = luaL_checkinteger(L, -1);
	
	// Make address structure
	struct sockaddr_in address;
	int address_length = sizeof(address);
	
	// Attempts to accept a pending connection.
	int lsock_client = accept(lsock, (struct sockaddr *) &address, &address_length);
	
	// If the connection was accepted without errors
	if(lsock_client != INVALID_SOCKET)
	{
		// Create a new metatable
		luaL_newmetatable(L, "Client");
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		// Register the __SOCKET index in the metatable
		lua_pushstring(L, "__SOCKET");
		lua_pushinteger(L, lsock_client);
		lua_settable(L, -3);
		// Register the __TIMEOUT index in the metatable
		lua_pushstring(L, "__TIMEOUT");
		lua_pushinteger(L, -1);
		lua_settable(L, -3);
		// Register the ipAddress index in the metatable
		lua_pushstring(L, "ipAddress");
		lua_pushstring(L, inet_ntoa(address.sin_addr));
		lua_settable(L, -3);
		// Define metatable functions
		luaL_setfuncs(L, ltnet_client_lib, 0);
		return 1;
	}
	
	// Return nil
	return 0;
}
// ltnet.server():close()
static int ltnet_server_close(lua_State *L) {
	// Get the socket stored in the metatable
	lua_getfield(L, 1, "__SOCKET");
	int lsock = luaL_checkinteger(L, -1);
	// Clean up WSA
	WSACleanup();
	// Close socket
	closesocket(lsock);
	return 0;
}
// ltnet_server lib
luaL_Reg ltnet_server_lib[] = {
	{ "bind", ltnet_server_bind },
	{ "accept", ltnet_server_accept },
	{ "close", ltnet_server_close },
	{ NULL, NULL }
};
// ltnet.server(optional mode->number);
static int ltnet_server(lua_State *L) {
	// Get optional mode number
	u_long iMode = (u_long)luaL_optnumber(L, 2, 1); //[default: 1]
	// Create a new TCP socket
	int lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Checks if there was an error creating the socket
	if (lsock == INVALID_SOCKET) {
		WSACleanup();
	} else {
		int iResult = ioctlsocket(lsock, FIONBIO, &iMode);
		if (iResult != NO_ERROR) {
			printf("ioctlsocket failed with error: %ld\n", iResult);
		}
		// Create a new metatable
		luaL_newmetatable(L, "Server");
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		// Register the __SOCKET index in the metatable
		lua_pushstring(L, "__SOCKET");
		lua_pushinteger(L, lsock);
		lua_settable(L, -3);
		// Define metatable functions
		luaL_setfuncs(L, ltnet_server_lib, 0);
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
	lua_pushliteral(L, "1.2");
	lua_setfield(L, -2, "_VERSION");
	return 1;
}
