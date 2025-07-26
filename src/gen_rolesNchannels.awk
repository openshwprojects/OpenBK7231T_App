BEGIN {
	enum_list = ""
	rolename_list = ""
	channels0 = ""
	channels1 = ""
	channels2 = ""
	ch0rule = ""
	ch2rule = ""
	ch0nums = ""
	ch2nums = ""
	chdesc_list = ""
	nofcdesc = 0
	indexcount = 0
	rulesprinted = 0
}



function finish_rule(start, end){

	ret=""
	# if it's one or two, add rule1 or rule1 || rule2
	# no need for a "range" here
	 if (end <= (start +1)) {
		ret = ret " || ( test == " rn[start] " )"
		if (end == start +1) {
			ret = ret " || ( test == " rn[end] " )"
			rulesprinted++
		}
		} else {
			ret = ret " || ( ( test >= " rn[start] " )&&( test <= " rn[end] " ) )"
			rulesprinted += 2
		}
	return ret

}

function genrolerule(numarr){
    # Initialize variables for range tracking
     split(numarr, nums)
    # nums will use "enums" so we need to convert to numbers
    range_start = nums[1]
    range_end = nums[1]
    rule = ""
    rulesprinted=0
    # Loop through the array to build the rule
    for (i = 2; i <= length(nums); i++) {
    	if (rulesprinted >=4){ 
    		rule = rule "\n\t\t"
    		rulesprinted = 0
    	} 
        if (nums[i] == range_end + 1) {
            # Extend the range if consecutive
            range_end = nums[i]
        } else {
            # Finalize the previous range
            rule = rule finish_rule(range_start,range_end)
            # Start a new range
            range_start = nums[i]
            range_end = nums[i]
        }
    }

    # Finalize the last range
    rule = rule finish_rule(range_start,range_end)
    # Remove the leading ' ||' from the rule
    sub(/^[[:space:]]*\|\|[[:space:]]*/, "", rule)

# return final rule
return rule;

}

# don't handle comment lines or empty lines
/^[[:space:]]*$/ {next}
/^[[:space:]]*\/\// {next}

# Only process matching lines
#
# Commented regexp: first ignore leading whitespace and then match
#
#			enum  , [space]   "name"  [space]    ,  [space]   channels   [space]     followed be "0 to 2" ,"desc" 
#/^[[:space:]]*[^,]+,[[:space:]]*"[^"]+"[[:space:]]*,[[:space:]]*[0-9]+[[:space:]]*(,[[:space:]]*"([^"]*)"[[:space:]]*){0,2}[,]*$/ {
# since docker uses mawk in a version not supporting intervals, allow for possible  additional input (not only 0 to 2 "descriptions" but "one or more")
/^[[:space:]]*[^,]+,[[:space:]]*"[^"]+"[[:space:]]*,[[:space:]]*[0-9]+[[:space:]]*(,[[:space:]]*"([^"]*)"[[:space:]]*)*[,]*$/ {

	#trim whitespaces - leading ones:
	sub(/[[:space:]]*/,"")

	#trim whitespaces around ","
	gsub(/[[:space:]]*,/,",")
	gsub(/,[[:space:]]*/,",")

	# build array from input, "," as seperator
	split($0,arr,",")

	# just for better readability
	role = arr[1]
	rolename = arr[2]
	channels = arr[3]
	ch1funct = arr[4]
	ch2funct = arr[5]

	# Build enum
	enum_list = enum_list role ",\n    "
	# Build string array
	# rolename kept " " during split
	rolename_list = rolename_list rolename " ,\n    "

	# Fill an array to keep track of roles and enum values (to find consecutive ones later)
	rn[indexcount]=role;
	rni[role]=indexcount;

	# Build array of channel descriptions 
	if (ch1funct !~ /^[[:space:]"]*$/ ){
		# ensure we always have a defined "empty quoted string" ("") as second parameter in case its ommited or empty in input
		if ( (! ch2funct ) || ( ch2funct == /^[[:space:]"]*$/ ) ) ch2funct = "\"\""
		chdesc_list = chdesc_list "\n    { " role ", " ch1funct "," ch2funct "},"
		nofcdesc++
	}
	else {
	}

	# Fill channel arrays with the enum value
	if (channels == 0) {
		channels0 = channels0 role ","
		ch0nums = ch0nums " " indexcount
	}
	else if (channels == 1){
		channels1 = channels1 role ","
	}
	else if (channels == 2) {
		channels2 = channels2 role ","
		ch2nums = ch2nums " " indexcount
	}


	# we are done, so process next entry - so we know if there is "unprocessed" data to generate a warning below
	indexcount++
	next
}
/.*/ {print "Opps, please check line " $0 > "/dev/stderr" }

END {
	# Remove trailing comma/newline
	sub(/,\n[ ]*$/, "", enum_list)
	sub(/,\n[ ]*$/, "", rolename_list)
	sub(/,$/, "", channels0)
	sub(/,$/, "", channels1)
	sub(/,$/, "", channels2)
	sub(/,$/, "", chdesc_list)
	sub(/[:space:]*||[:space:]*$/, "", ch0rule)
	sub(/[:space:]*||[:space:]*$/, "", ch2rule)

	# test for consecutive numbers to build 
	#  (test >= 5)&&(test <=10) instead of
	#  (test == 5) || (test == 6) || (test == 7) || (test == 8) || (test == 9) || (test == 10)
	ch0rule = (ch0nums!="") ? genrolerule(ch0nums) : ""
	ch2rule = (ch2nums!="") ? genrolerule(ch2nums) : ""



		
	print "#if (INCLUDED_BY_NEW_PINS_H) || (INCLUDED_BY_NEW_PINS_C) || (INCLUDED_BY_NEW_HTTP_C) || (INCLUDED_BY_HTTP_FNS_C)\n\n\n"
	print "#if (INCLUDED_BY_NEW_PINS_H)\n"
	print "typedef enum ioRole_e {\n    " enum_list "\n} ioRole_t;\n"
	print "#endif // (INCLUDED_BY_NEW_PINS_H)\n"
	
	print "#if (INCLUDED_BY_NEW_HTTP_C)\n"
	print "const char* htmlPinRoleNames[] = {\n    " rolename_list "\n};\n"
	print "#endif // (INCLUDED_BY_NEW_HTTP_C)\n"

	print "#if (INCLUDED_BY_NEW_PINS_C)\n"
	print "/*\n"
	print "ioRole_t channels0[] = { " channels0 " };\n"
	print "ioRole_t channels1[] = { " channels1 " };\n"
	print "ioRole_t channels2[] = { " channels2 " };\n\n"
	print "int PIN_IOR_NofChan(int test){\n"
	print "\tfor (int i = 0; i < sizeof(channels0) / sizeof(channels0[0]); i++) {"
	print "\t\tif (test == channels0[i]) {"
	print "\t\t\treturn 0;"
	print "\t\t}"
	print "\t}"
	print "\tfor (int i = 0; i < sizeof(channels2) / sizeof(channels2[0]); i++) {"
	print "\t\tif (test == channels2[i]) {"
	print "\t\t\treturn 2;"
	print "\t\t}"
	print "\t}"
	print "\treturn 1;"
	print "};\n"
	print "*/\n"


	n=split(ch0nums,tmparr)
	tt="// for control usage\n//   roles without any channels\n//\t"
	c=0;
	for (a in tmparr){c++; tt = tt rn[tmparr[a]] "("tmparr[a]")   "; if (c%5==0) tt = tt "\n//\t"}
	print tt "\n//"
	n=split(ch2nums,tmparr)
	tt="//\n//   roles with 2 channels\n//\t"
	c=0;
	for (a in tmparr){c++; tt = tt rn[tmparr[a]] "("tmparr[a]")   "; if (c%5==0) tt = tt "\n//\t"}
	print tt "\n"


	print "int PIN_IOR_NofChan(int test){\n"
	if (ch0rule !="") { print "\tif( " ch0rule " ){\n\t\treturn 0;\n\t}"}
	if (ch2rule !="") { print "\tif( "  ch2rule " ){\n\t\treturn 2;\n\t}"}
	print "\treturn 1;\n}\n"
	print "#endif // (INCLUDED_BY_NEW_PINS_C)\n"

	print "#if (INCLUDED_BY_HTTP_FNS_C)\n"
	print "\n\n#if (ENABLE_USE_PINROLE_CHAN_DESC)\n"
	print "int Nofchandesk=" nofcdesc ";\n"
	print "typedef struct chan_data {\n  int id;\n  const char *ch1;\n  const char *ch2;\n} chan_data;\n"
	print "chan_data chan_desc[]={" chdesc_list "\n};\n"
	print "#endif // (ENABLE_USE_PINROLE_CHAN_DESC)\n"
	print "#endif // (INCLUDED_BY_HTTP_FNS_C)\n"
	
	print "#endif // (INCLUDED_BY_NEW_PINS_H) || (INCLUDED_BY_NEW_PINS_C) || (INCLUDED_BY_NEW_HTTP_C) || (INCLUDED_BY_HTTP_FNS_C)\n\n\n"
}

