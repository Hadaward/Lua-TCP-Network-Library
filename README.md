### Lua TCP Network Library (LTN/LTNet) *Only for windows
A simple network library that makes it easy to create tcp sockets, allows you to create both client and server.

**OBS:** A new version is coming to bring :settimeout and :setsockopt

### Server Sample
```Lua
local net = require"ltnet";
-- Create a server instance
local server = net.server();
-- Bind a port and if not get error return true
local running = server:bind(6112);
-- Client
local client
-- loop
while(running)do
	if (client) then
		-- when data is received status is true and message is data, bytes means count of bytes received, if get any error or connection is closed status is false and message represents the error in string
		local status, message, bytes = client:recv(512)

		if (status) then
			print("new data received: ", message)
			client:write("Pong!")
		else
			client:close()
			print("connection closed by the following reason: ", message)
			break
		end
	else
		-- try to accept pending connections, if can't return nil
		client = server:accept()
	end
end
-- close server socket if loop was broken
server:close()
```

### Client Sample
```Lua
local net = require"ltnet";
-- Create a client instance
local client = net.client();
-- Try connection with a host
local running = client:connect("127.0.0.1", 6112)
-- if connection is established running is true
if (running) then
	client:write("Ping!")

	while(true)do
		-- when data is received status is true and message is data, bytes means count of bytes received, if get any error or connection is closed status is false and message represents the error in string
		local status, message, bytes = client:recv(512)

		if (status) then
			print("server response: ", message)
		else
			print("error: ", message)
		end

		client:close()
		break
	end
end
```
