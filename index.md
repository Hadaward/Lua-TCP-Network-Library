[View project on GitHub](https://github.com/Hadaward/Lua-TCP-Network-Library)

A simple network library that makes it easy to create tcp sockets, allows you to create both client and server.

## Changelog:
### v1.2 
- Added :settimeout(time in milliseconds) [< 0 = disable, >= 0 = enable]
- The ltnet.server function now has an optional numeric argument "mode" that defines the socket mode for non-blocking or blocking, with 0 for blocking and 1 for non-blocking. (by default mode is non-blocking)
- Minor code changes.

### v1.1
- The main functions were created.
- The library has been published.

### Coming soon:
- :setsockopt
- Errors using the syntax: "error id: message" [string]

### Server Sample
```lua
local net = require"ltnet";
-- Create server socket
local server = net.server(); -- optional: net.server(0); to disable non-blocking, default: 1
-- Bind port 6112 [return nil if got error]
local listening = server:bind(6112)
-- Clients table
local clients = {}

while(true)do
	-- Try to accept pending clients
	local newclient = server:accept()
	-- If newclient isn't nil
	if(newclient)then
		-- Say in console: "new connection:	ip address"
		print("new connection:", newclient.ipAddress)
		-- Set timeout to 1 milliseconds [works better then 0]
		newclient:settimeout(1)
		-- Insert in clients table
		clients[#clients+1] = newclient
	end
	-- Index table to remove disconnected clients from clients
	local removeClients = {}
	-- Walk through clients
	for index, client in next, clients do
		-- Request message with max length 512
		local status, message, lengthBytes = client:recv(512) --[boolean,string or number[error id],number]
		
		if (not status and (message == "closed" or message == 10054)) then
			removeClients[index] = true
			print("client disconnection:", client.ipAddress)
		elseif(not status and message ~= "timeout")then
			print("error: ", message)
		elseif(status)then
			print(message, lengthBytes)
		end
	end
	
	for index in next, removeClients do
		clients[index] = nil
	end
end
```

### Client Sample
```lua
local net = require"ltnet";
-- Create client instance
local client = net.client();
-- Try to connect to the host
local connected = client:connect("127.0.0.1", 6112) --[boolean]
-- Set timeout to 1 milliseconds [works better then 0]
client:settimeout(1)

if (connected) then
	-- send "ping" to the server
	client:write("ping!")
	-- loop to check if recv any message
	while(connected)do
		-- Request message with max length 512
		local status, message, lengthBytes = client:recv(512) --[boolean,string or number[error id],number]
		
		if (not status and (message == "closed" or message == 10054)) then
			connected = false
			print("client disconnected!")
		elseif(not status and message ~= "timeout")then
			print("error: ", message)
		elseif(status)then
			print(message, lengthBytes)
		end
	end
else
	print("not connected!")
end
```
