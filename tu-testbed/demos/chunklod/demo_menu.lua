-- launcher for presentation demos, requires only lua.exe

options = " -f -w 1024 -h 768 -2"

while 1 do
	write("\n")
	write("1. maui\n")
	write("2. riverblue\n")
	write("3. puget\n")
	write("4. crater\n")
	write("5. hawaii\n")
	write("6. kauai\n")
	write("Q. quit\n")
	write("\n");

	local choice = read("*l")
	write("choice = " .. choice .. "\n")

	if (choice == "q" or choice == "Q") then
		exit(0)
	elseif (choice == "1") then
		execute("./chunkdemo.exe maui_2k.chu maui_4k_d5.tqt " .. options)
	elseif (choice == "2") then
		execute("./chunkdemo.exe riverblue_e1_d8.chu riverblue_16k_d7.tqt " .. options)
	elseif (choice == "3") then
		execute("./chunkdemo.exe puget_e4_d9.chu puget_32k_d9.tqt " .. options)
	elseif (choice == "4") then
		execute("./chunkdemo.exe " .. options)
	elseif (choice == "5") then
		execute("./chunkdemo.exe island_e1_d9.chu island_16k_d7.tqt " .. options)
	elseif (choice == "6") then
		execute("./chunkdemo.exe kauai_e1_d9.chu kauai_8k_d6.tqt " .. options)
	end
end
