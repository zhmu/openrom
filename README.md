# Introduction

This is a collection of tools to analyse the Runes of Magic network protocol; however, the tools are quite generic which means you should be able to use them to analyse different data with minor adjustments.

## protocol.xml

COntains the reverse-engineered protocol of Runes of Magic 6.0.5.2579.en - this is by no means complete, but it does provide enough to be able to understand how the game works.

## mkdef

Using `protocol.xml`, capable of generating packet parsing/construction code and Python bindings per packet type.

## romdump

Reads a tcpflow-written text output stream or a romproxy log file and decodes the stream using definitions from `protocol.xml` and optionally a `sysname.csv` file (see below)

## romproxy

A proxy server which 'sits' between the game client and the actual game servers, with the purpose to log all traffic in a custom format which is far easier to process than packet dumps.

# License

Everything is licensed using the GNU Affero Generic Public License version 3 - make sure you understand it before using this work. Furthermore, ensure you read and understand the terms and conditions of Runes of Magic before applying any of these tools to the actual game itself.

# Building

You'll need libxml2's development package (`libxml2-dev` on Debian-based distributions) to build this. It's only been tested on Linux-based hosts.

# Creating sysname.csv for use with romdump

You will need https://github.com/zhmu/romdb; clone it to ~/romdb and:

```
$ cd ~/romdb/tools/fdbtool
$ make
$ cd /tmp
$ (path to)fdbtool (path to)data.fdb
$ cd ~/romdb/scripts
$ cp lib/config.sample.py lib/config.py
$ cd /tmp/data
$ $EDITOR ~/romdb/scripts/stringdb2csv.py
```

And put the following in there:

````
#!/usr/bin/python3

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), 'lib'))
import romdb

if len(sys.argv) != 2:
	print("usage: %s string_....db" % sys.argv[0])
	sys.exit(0)

fname = sys.argv[1]
strings = romdb.read_stringdb(fname)
for k, v in strings.items():
	if k.startswith('Sys'):
		print("%s,%s" % (k, v))
````

Then generate `sysname.csv` by:

```
$ chmod +x ~/romdb/scripts/stringdb2csv.py
$ ~/romdb/scripts/stringdb2csv.py string_eneu.db > sysname.csv
````

# Configuring the game client

The official Runes of Magic client, appropriately named `Client.exe`, has several configuration options which are useful when performing analysis.

## Using a different server

You can alter the IP address of the server to use by editing 'runedev.ini' in the game folder and setting the following options:

```
[Server]
IP=(your IP address here)
Port=21002
RunewakerInsideTestServer=1
DisplayMaintainMode=1

[Debug]
SkipVersionCheck=1
```
## Skipping the version check

Create a shortcut to Client.exe with `NoCheckVersion` as argument
