# HideProc
This code is part of "Absoute Essence of Rootkits" series @https://epicshellcode.blogspot.com

Brief : 
This is a  driver which hides a calling userland process (DKOM).This is very interesting feature most malware/rootkit uses for hiding itself to evade and confuse the malware researchers. Here, malicious driver manipulate the EPROCESS structure's Flink and Blink pointers.


To test this driver, load it with the help of OSR loader and call its IOCTL 0x0022200B. Python file HideProc.py will help you test this feature very fast.

This POC will work on 32bit system only due to PatchGuard.
