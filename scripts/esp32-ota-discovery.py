import time
import socket
from zeroconf import ServiceBrowser, ServiceListener, Zeroconf


class MyListener(ServiceListener):

    def update_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        pass

    def remove_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        pass

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        info = zc.get_service_info(type_, name)
        print(f"{info.server:<30} {socket.inet_ntoa(info.addresses[0]):<16} {info.port:<16}")


zeroconf = Zeroconf()
listener = MyListener()
browser = ServiceBrowser(zeroconf, "_arduino._tcp.local.", listener)

timeout = 3
poll    = 1
while(timeout > 0):
    time.sleep(poll)
    timeout -= poll

zeroconf.close()
