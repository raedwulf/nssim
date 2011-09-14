#!/usr/bin/env python

import subprocess
PT_NEW_CLIENT="\x00"
PT_SEND="\x01"
PT_RECV="\x02"
PT_DISCONNECT="\x03"
CLIENT1="\x01\x00\x00\x00"
CLIENT2="\x02\x00\x00\x00"

p = subprocess.Popen('./wifisim', stdout=subprocess.PIPE, stdin=subprocess.PIPE)
p.stdin.write(PT_SEND+CLIENT1+CLIENT2+"\x0C\x00\x00\x00Hello World\x00")
data = p.stdout.read(1+12+12)
print len(data)
