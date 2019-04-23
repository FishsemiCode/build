CAUTION: You MUST run nuttx with ROOT privileges.

How to setup the networking for Ubuntu?
Assume your NIC as below:
  eth0  Link encap:Ethernet  HWaddr f0:4d:a2:dd:ab:4c
        inet addr:10.235.242.41  Bcast:10.235.242.255  Mask:255.255.255.0

First, create the network bridge:
  sudo ip link add nuttx0 type bridge
  sudo ip addr flush dev eth0
  sudo ip link set eth0 master nuttx0
  sudo ip link set dev nuttx0 up
  sudo dhclient nuttx0

Second, start nuttx simulator with ROOT privileges:
  sudo out/u1/sim/nuttx/nuttx
  sim> ping www.mi.com
  PING 61.149.9.232 56 bytes of data
  56 bytes from 61.149.9.232: icmp_seq=0 time=20 ms
  56 bytes from 61.149.9.232: icmp_seq=1 time=0 ms
  56 bytes from 61.149.9.232: icmp_seq=2 time=10 ms
  56 bytes from 61.149.9.232: icmp_seq=3 time=10 ms
  56 bytes from 61.149.9.232: icmp_seq=4 time=20 ms
  56 bytes from 61.149.9.232: icmp_seq=5 time=10 ms
  56 bytes from 61.149.9.232: icmp_seq=6 time=10 ms
  56 bytes from 61.149.9.232: icmp_seq=7 time=10 ms
  56 bytes from 61.149.9.232: icmp_seq=8 time=10 ms
  56 bytes from 61.149.9.232: icmp_seq=9 time=10 ms
  10 packets transmitted, 10 received, 0% packet loss, time 10100 ms

Last, you can remove the bridge:
  sudo ip link delete nuttx0
  sudo dhclient eth0

Note, Don't use WIFI which can't work well with bridge.

That's it.
