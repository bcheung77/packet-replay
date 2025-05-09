# packet-replay

This repository contains a library and a set of executables designed to recreate network requests saved in a PCAP (Wireshark, tcpdump) capture file.

## Motivations

There are two primary use cases for creating this tool:

- Automated Testing:
  - From experience, I spent a considerable amount of time crafting REST requests and using Postman or curl to send them to the server, followed by manually verifying correctness.
  - The goal of this tool is to enable running tests manually once, capturing the interaction, and then simply replaying the conversation after any code changes.
  - This can also be applied to generate automated tests.
    
- Production Issue Reproduction:
  - Occasionally, issues occur in the field that are not reproducible because the customer's specific client is inaccessible. This tool can be used to mimic that client via a packet capture.
  - This can be extended to support protocols things like SIP handshake.
 
## Requirements

- libpcap-dev
- python development library

## Build

- mkdir build
- cd build
- cmake ..
- make

## http_replay

Mimics an HTTP client.

Usage: http_replay [-c <client spec>] <cap file>

-c specifes the client to emulate.  Format: \<src IP\>[:\<src port\>[:\<test IP\>[:\<test port\>]]]

- src IP - the IP address of the client in the capture file to recreate
- src port - the port of the client in the capture file to recreate
- test IP - the IP of the target server to send the recreated requests to.  Defaults to the IP in the capture file
- test port - the port of the target server to send the recreated request to.  Defaults to the port in the capture file

If no "client spec" is specified, the TCP stream with the first connection request is recreated.

## udp_replay

Replay captured UDP packets

Usage: ./udp_replay[-c <client spec>] [-k <packet validator spec>] <cap file>

-c specifes the client to emulate.  Format: \<src IP\>[:\<src port\>[:\<test IP\>[:\<test port\>]]]

- src IP - the IP address of the client in the capture file to recreate
- src port - the port of the client in the capture file to recreate
- test IP - the IP of the target server to send the recreated requests to.  Defaults to the IP in the capture file
- test port - the port of the target server to send the recreated request to.  Defaults to the port in the capture file

-k specifies how to validate packets.  Default is exact packet match.  Format: \<type\>:\<type specific spec>

- type - the type of validator.  Currently supports only "python"
- <python spec> - format \<path to python module\>:<\<function name\>.  Example: [python:../src/python/dns.cap](/src/python/dns.py)  The function must return bool and take 2 "bytes" arguments expected-packet and test-packet

## Work in Progress

This tool is still under active development. Current planned features include:

- IPv6 support
- REST-specific enhancements, such as checking for JSON equivalency
- TLS support
- gzip, deflate, and chunked HTTP transport support

