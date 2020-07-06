local net = require"ltnet";
-- Create client instance
local client = net.client();
-- Try to connect to the host
local connected = client:connect("127.0.0.1", 6112) --[boolean]
-- Set timeout to 0 milliseconds
client:settimeout(0)

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