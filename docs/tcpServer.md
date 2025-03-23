# Simple TCP command server for scripting
  You can enable a simple TCP server in device Generic/Flags option, which will listen by default on port 100. Server can accept single connection at time from Putty in RAW mode (raw TCP connection) and accepts text commands for OpenBeken console. In future, we may add support for multiple connections at time. Server will close connection if client does nothing for some time.

  There are some extra short commands for TCP console:
- GetChannel [index] 
- GetReadings - returns voltage, current and power
- ShortName 
the commands above return a single ASCII string as a reply so it's easy to parse.