local net = require"ltnet";
-- Create server socket
local server = net.server();
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
		-- Set timeout to 0 milliseconds
		newclient:settimeout(0)
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
			
			if(message == "ping!") then
				client:write("pong!")
			end
		end
	end
	
	for index in next, removeClients do
		clients[index] = nil
	end
end
