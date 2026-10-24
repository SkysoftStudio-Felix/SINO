# Network loopback stage

Completed: device abstraction and packet queue; IPv4, ICMP, UDP; bounded stop-and-wait TCP; process-local socket handles and default-deny token; nonblocking/readiness/timed user wrappers; DNS A and HTTP/1.0 user clients with threaded local responders; statistics and error paths.

Validation: normal/test builds, native GUI selftest, user-mode isolation test, actual guest network test; kernel packet loss/checksum/duplicate/window/buffer/resource checks. A close-after-peer-FIN resource leak was discovered by the new selftest and fixed by recording peer FIN separately from local FIN acknowledgement. No retries or failures are hidden as successful placeholder operations.

Limits and pending work are listed in network.md. No external NIC, Ethernet/ARP, DHCP or public access. Package permission delegation and full TCP hardening are not complete.
