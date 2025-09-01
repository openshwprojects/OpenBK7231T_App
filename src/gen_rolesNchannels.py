import re
import sys

def main(file):
    enum_list = []
    rolename_list = []
    INFO2CHANVAL = []
    channels = 0
    ch1funct = ""
    ch2funct = ""
    rolename = ""
    indexcount = 0
    cvalstart = 0		# 0 before start / 1 if inside structure / 2 after 

    
    # Regexes for extracting iodetail
    rx_iodetail = re.compile(r'^([ \t]*)//iodetail:(.+)')
    rx_channels = re.compile(r'"channels used":"(\d+)"')
    rx_ch1 = re.compile(r'"channel 1 usage":"([^"]*)"')
    rx_ch2 = re.compile(r'"channel 2 usage":"([^"]*)"')
    rx_htmlPinRole = re.compile(r'"htmlPinRoleName":("[^"]*")')		# we need this as string, so with quotes 
    
    rx_cvalsstart = re.compile(r'^[ \t]*cval_t cvals\[\] = {')
    rx_cvalsend = re.compile(r'^[ \t]*}[ \t]*;')
    rx_cvalsline = re.compile(r'^[ \t]*{"([^"]*)","([^"]*)","([^"]*)","([^"]*)"(,([0-9]))?}')
    

    # To store last seen iodetail fields
    iodetail_fields = {}

#    for line in sys.stdin:
    for line in file.readlines():
        line = line.rstrip('\n')

        if not line.strip():
            continue


        # search for channel descriptions, comments ...
        if cvalstart < 2 :
            if cvalstart == 0 :
                if rx_cvalsstart.match(line):
                    cvalstart = 1
                    chan_usage = {}
                    cval_t = 'typedef enum {'
                    cval_html = 'de=[ '
                    legend_html = "document.getElementById('legend').innerHTML=\\\"Functions:"
                    i=0

                continue
            # it's 1 now, so we are inside cvals
            if  rx_cvalsend.match(line):
                cvalstart = 2
                cval_t+= '\n} chanval_t ;\n'
                cval_html = cval_html.rstrip(',') + ' ];'
                legend_html = legend_html.rstrip('/') + '\\\"'
                continue
            m = rx_cvalsline.match(line)
            if m:
                use = m.group(1)
                abbr = m.group(2)
                comment = m.group(3)
                html = m.group(4)
                legend = 0
                if m.group(6):
                    legend = int(m.group(6))
                chan_usage[use] = 'CHAN_' + abbr
                cval_t += '\n\tCHAN_' + abbr + ',\t\t// ' + str(i) + " = " + comment
                cval_html += "'" + html + "',"
                if i > 0 and legend == 1:
               	    legend_html += ' \\\" + de[' + str(i) + '] + \\\": ' + use + ' /'  
                i +=1   	
               	
            continue

        
         
        # Parse "//iodetail:" lines 
        # allways first add them (with leading whitespace replaced by tab) to output enum_list 
        # 
        m = rx_iodetail.match(line)
        if m:
            # Replace leading whitespace with a single tab
            iodetail_line = '\t//iodetail:' + m.group(2)
            enum_list.append(iodetail_line)

            detail = m.group(2)
            if rx_channels.search(detail):
                channels = int(rx_channels.search(detail).group(1))
                iodetail_fields['channels'] = channels
            if rx_ch1.search(detail):
                ch1funct = rx_ch1.search(detail).group(1)
                iodetail_fields['ch1funct'] = ch1funct
            if rx_ch2.search(detail):
                ch2funct = rx_ch2.search(detail).group(1)
                iodetail_fields['ch2funct'] = ch2funct
            if rx_htmlPinRole.search(detail):
                rolename = rx_htmlPinRole.search(detail).group(1)
                iodetail_fields['rolename'] = rolename
            continue

        # Skip irrelevant lines (if we use an header-file as input to generate another header-file
        if line.strip().startswith('//'):
            continue
        if line.strip().startswith('typedef'):
            continue
        if line.strip().startswith('#'):
            continue

        # Process enum lines
        m_enum = re.match(r'^\s*(IOR_[^,]+),?\s*$', line)
        if m_enum:
            role = m_enum.group(1)
#            enum_list.append(f"\t{role},")
            enum_list.append("\t" + role + ",")  # Using string concatenation - ESP8266 envoronment seems to use very old python
            rolename_val = iodetail_fields.get('rolename', '')
#            rolename_list.append(f"\t{rolename_val},")
            rolename_list.append("\t" + rolename_val + ",")  # Using string concatenation - ESP8266 envoronment seems to use very old python
            ch1funct_val = iodetail_fields.get('ch1funct', '')
            ch2funct_val = iodetail_fields.get('ch2funct', '')
            channels_val = iodetail_fields.get('channels', 0)
            c1 = chan_usage.get(ch1funct_val, 'CHAN_Empty')
            c2 = chan_usage.get(ch2funct_val, 'CHAN_Empty')
#            INFO2CHANVAL.append(f"\tINFO2CHANVAL({role}, {channels_val}, {c1}, {c2}),")
            INFO2CHANVAL.append("\tINFO2CHANVAL(" + role + ", " + str(channels_val) + ", " + str(c1) + ", " + str(c2) + "),") # Using string concatenation - ESP8266 envoronment seems to use very old python
            indexcount += 1
            # Reset for next role
            iodetail_fields.clear()
            continue

        if 'ioRole_t;' in line:
            break

        # Print error for unhandled lines
#        print(f"Opps, please check line {line}", file=sys.stderr)
        print("Opps, please check line " + str(line), file=sys.stderr)


    # Remove trailing commas
#    if enum_list and enum_list[-1].endswith(','):
#        enum_list[-1] = enum_list[-1].rstrip(',')
#    if rolename_list and rolename_list[-1].endswith(','):
#        rolename_list[-1] = rolename_list[-1].rstrip(',')
#    if INFO2CHANVAL and INFO2CHANVAL[-1].endswith(','):
#        INFO2CHANVAL[-1] = INFO2CHANVAL[-1].rstrip(',')

    # Output
    print("#if (INCLUDED_BY_NEW_PINS_H) || (INCLUDED_BY_NEW_PINS_C) || (INCLUDED_BY_NEW_HTTP_C) || (INCLUDED_BY_HTTP_FNS_C)\n\n\n")
    print("#if (INCLUDED_BY_NEW_PINS_H)\n")
    print("typedef enum ioRole_e {\n" + "\n".join(enum_list) + "\n} ioRole_t;\n")
    print("extern short chanvals[];\n")




    print("#endif // (INCLUDED_BY_NEW_PINS_H)\n")

    print("#if (INCLUDED_BY_NEW_HTTP_C)\n")
    print("const char* htmlPinRoleNames[] = {\n" + "\n".join(rolename_list) + "\n};\n")
    print("#endif // (INCLUDED_BY_NEW_HTTP_C)\n")

    print("#if (INCLUDED_BY_NEW_PINS_C)\n")
    print("\n" + cval_t + "\n")
    print("#define INFO2CHANVAL(r, nofc, c1, c2)  ((c2 * 1024) + (c1 * 16) + nofc) \n")
    print("short chanvals[] = {\n" + "\n".join(INFO2CHANVAL) + "\n};\n")
    print("\nint PIN_IOR_NofChan(int test){")
    print("\treturn chanvals[test]%16;")
    print("}\n")
    print("#endif // (INCLUDED_BY_NEW_PINS_C)\n")

    print("#if (INCLUDED_BY_HTTP_FNS_C)\n")
    print("\n#define LEGENDSCRIPT \"<script>" + cval_html +  legend_html + "</script>\"\n")
    print("#endif // (INCLUDED_BY_HTTP_FNS_C)\n")


    print("#endif // (INCLUDED_BY_NEW_PINS_H) || (INCLUDED_BY_NEW_PINS_C) || (INCLUDED_BY_NEW_HTTP_C) || (INCLUDED_BY_HTTP_FNS_C)\n\n\n")

if __name__ == '__main__':
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            main(f)
    else:
        main(sys.stdin)
