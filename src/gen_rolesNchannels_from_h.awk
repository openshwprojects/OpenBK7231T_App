BEGIN {
	enum_list = ""
	rolename_list = ""
	indexcount = 0
	INFO2CHANVAL = ""
	
	# For every role we send a desription string and the number of channels for this role
	# We can only have 0, 1 or 2 channels, so using an integer is "wasted information"
	# Idea: code the "channel use" (like "connector", "temperature" ...) there
	# so let's use an integer (at least 16 bits for three numbers:
	# 	two 6-bit numbers for "use of channel 1 and 2" and 
	#	one 4-bit number (number of channels)	
	#	X % 16		Number of channels (4 bits)
	#	(X >> 4) % 64	Channel 1 usage (6 bits) 
	#	(X >> 10) % 64	Channel 2 usage (6 bits) 

	# define a "function" for a channel as number
	# values for C, T, H, I, X plus some placeholders for A and B

	# step one: Define an array of pairs
	split("C:1,T:2,H:3,I:4,X:5,CO2:6,TVOC:7,A:8,B:9", arr, ",")
	# now build the final associative array
	for (i in arr) {
		split(arr[i], cp, ":")
		chan_pairs[cp[1]] = cp[2]	# chan_pairs["C"] = 1, chan_pairs["T"] = 2 ....
	}
	
	# step two: Define an array of pairs
	split( \
	":CHAN_Empty," \
	"Connector:CHAN_Connector," \
	"Temperature:CHAN_Temperature," \
	"Humidity:CHAN_Humidity," \
	"Input:CHAN_Input," \
	"Toggle:CHAN_Toggle," \
	"CO2:CHAN_CO2," \
	"TVOC:CHAN_TVOC," \
	"addFunctionhere 1:CHAN_Func1," \
	"addFunctionhere 1:CHAN_Func2", arr, ",")
	for (i in arr) {
		split(arr[i], cp, ":")
		chan_usage[cp[1]] = cp[2]	# chan_usage["Connector"] =CHAN_Connector , chan_usage["Temperature"] = CHAN_Temperature ....
	}
}

# this is how the lines look we need to extract information from :
#
#		//iodetail:{"name":"Relay",
#		//iodetail:"title":"TODO",
#		//iodetail:"descr":"an active-high relay. This relay is closed when a logical 1 value is on linked channel",
#		//iodetail:"channels used":"1",
#		//iodetail:"channel 1 usage":"Connector",
#		//iodetail:"channel 2 usage":"",
#		//iodetail:"htmlPinRoleName":"Rel",
#		//iodetail:"enum":"IOR_Relay",
#		//iodetail:"file":"pins_and_roles.h",
#		//iodetail:"driver":""}
#		IOR_Relay,
#
# we need
#	the number of channels in var "channels" from 			//iodetail:"channels used":"1",
#	the function of channel1 in var "ch1funct" from 			//iodetail:"channel 1 usage":"Connector",
#	the function of channel2 in var "ch2funct" from 			//iodetail:"channel 2 usage":"",
#	the string for var "rolename" from 					//iodetail:"htmlPinRoleName":"Rel",


# don't handle empty lines or comments exept "//iodetails: ..." lines
/^[[:space:]]*$/ {next}
/^[[:space:]]*\/\/iodetail:/ {
	sub(/^[[:space:]]*/,"\t")
	enum_list = enum_list $0 "\n"
	
	# build array from input, ":" as seperator
	split($0,arr,":")


	if (/channels used/) {
		# remove qoutes 
		gsub("\"","",arr[3])
		channels=0+arr[3]
	}
	# "\"" still used here, but remove trailing ","
	sub(",","",arr[3])	
	if (/channel 1 usage/) ch1funct=arr[3]
	if (/channel 2 usage/) ch2funct=arr[3]
	if (/htmlPinRoleName/) rolename=arr[3]
	
}

# ignore all other comments, "typdef" or  preprocessor directives like #define or #if lines
/^[[:space:]]*\/\// {next}
/^[[:space:]]*typedef/ {next}
/^[[:space:]]*[#]/ {next}

# process matching lines
#
/^[[:space:]]*IOR_[^,]+[,]*[[:space:]]*$/ {

	#trim whitespaces - leading ones:
	sub(/[[:space:]]*/,"")

	#trim whitespaces around ","
	gsub(/[[:space:]]*,/,",")
	gsub(/,[[:space:]]*/,",")


	# build array from input, "," as seperator
	split($0,arr,",")
	role = arr[1]

	# Build enum
	enum_list = enum_list "\t" role ",\n"
	# Build string array
	# rolename kept " " during split
	rolename_list = rolename_list "\t" rolename ",\n"

		# removing "hard" quotes from channel function string
		gsub(/"/, "" ,ch1funct)
		gsub(/"/, "",ch2funct)


	INFO2CHANVAL = INFO2CHANVAL "\tINFO2CHANVAL(" role ", " channels ", " chan_usage[ch1funct]  ", " chan_usage[ch2funct] "),\n"


	# we are done, process next entry - so we know if there is "unprocessed" data to generate a warning below
	indexcount++
	next
}
/ioRole_t;/ { exit }
/.*/ {print "Opps, please check line " $0 > "/dev/stderr" }

END {
	# Remove trailing comma/newline
	sub(/,\n[ ]*$/, "", enum_list)
	sub(/,\n[ ]*$/, "", rolename_list)

	print "#if (INCLUDED_BY_NEW_PINS_H) || (INCLUDED_BY_NEW_PINS_C) || (INCLUDED_BY_NEW_HTTP_C) || (INCLUDED_BY_HTTP_FNS_C)\n\n\n"
	print "#if (INCLUDED_BY_NEW_PINS_H)\n"
	print "typedef enum ioRole_e {\n" enum_list "\n} ioRole_t;\n"

	print "typedef enum {"
	print "\tCHAN_Empty,\t\t\t// 0 = no Description"
	print "\tCHAN_Connector,\t\t// 1 = Connector (LED, button, relay ...)"
	print "\tCHAN_Temperature,\t// 2 = Temperature"
	print "\tCHAN_Humidity,\t\t// 3 = Humidity"
	print "\tCHAN_Input,\t\t\t// 4 = Input pin value"
	print "\tCHAN_Toggle,\t\t// 5 = Channel for toggle"
	print "\tCHAN_CO2,\t\t\t// 6 = Channel or CO2 value"
	print "\tCHAN_TVOC,\t\t\t// 7 = Channel TVOC value"
	print "\tCHAN_Func1,\t\t\t// 8 = Room for additional functions"
	print "\tCHAN_Func2,\t\t\t// 9 = Room for additional functions"
	print "\tCHAN_Func3\t\t\t// 10 = Room for additional functions"
	print "} chanval_t ;"
	print "\n"

	print "#define INFO2CHANVAL(r, nofc, c1, c2)  ((c2 * 1024) + (c1 * 16) + nofc) \n"
	
	print "short chanvals[] = {\n" INFO2CHANVAL "};\n"
	
	
	print "#endif // (INCLUDED_BY_NEW_PINS_H)\n"
	
	print "#if (INCLUDED_BY_NEW_HTTP_C)\n"
	print "const char* htmlPinRoleNames[] = {\n" rolename_list "\n};\n"
	print "#endif // (INCLUDED_BY_NEW_HTTP_C)\n"

	print "#if (INCLUDED_BY_NEW_PINS_C)\n"

	print "\nint PIN_IOR_NofChan(int test){"
	print "\treturn chanvals[test]%16;"
	print "}"

	print "#endif // (INCLUDED_BY_NEW_PINS_C)\n"
	
	print "#endif // (INCLUDED_BY_NEW_PINS_H) || (INCLUDED_BY_NEW_PINS_C) || (INCLUDED_BY_NEW_HTTP_C) || (INCLUDED_BY_HTTP_FNS_C)\n\n\n"
}

