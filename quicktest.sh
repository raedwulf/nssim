#!/bin/sh

PT_NEW_CLIENT="\x00"
PT_SEND="\x01"
PT_RECV="\x02"
PT_DISCONNECT="\x03"
CLIENT1="\x01\x00\x00\x00"
CLIENT2="\x02\x00\x00\x00"
BROADCAST="\x00\x00\x00\x00"
echo -ne "${PT_SEND}${CLIENT1}${CLIENT2}\x0C\x00\x00\x00Hello World\x00" | ./wifisim | xxd -c 1 -i | sed 's|  |\\|;s|,||' | tr -d '\n'
echo -ne "${PT_SEND}${CLIENT1}${BROADCAST}\x0C\x00\x00\x00Hello World\x00" | ./wifisim | xxd -c 1 -i | sed 's|  |\\|;s|,||' | tr -d '\n'
