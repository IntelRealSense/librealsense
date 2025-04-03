# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.

from rspy import log
import time
import platform, re
from rspy import device_hub
import os

if __name__ == '__main__':
    import os, sys, getopt
    def usage():
        ourname = os.path.basename( sys.argv[0] )
        print( 'Syntax: unifi [options]' )
        print( '        Control the Unifi Ubiquiti hub' )
        print( 'Options:' )
        print( '        --enable       Enable all ports' )
        print( '        --disable      Disable all ports' )
        print( '        --recycle      Recycle all ports' )
        print( '        --reset        Reset the Unifi' )
        sys.exit(2)
    try:
        opts,args = getopt.getopt( sys.argv[1:], '',
            longopts = [ 'help', 'recycle', 'enable', 'disable', 'reset' ])
    except getopt.GetoptError as err:
        print( '-F-', err )   # something like "option -a not recognized"
        usage()
    if args or not opts:
        usage()
    # See the end of the file for all the option handling

# we assume that if paramiko is not installed, unifi is not in use
try:
    import paramiko
except ModuleNotFoundError:
    log.d( 'no paramiko library is available' )
    raise


if "UNIFI_SSH_PASSWORD" not in os.environ:
    log.d("no unifi credentials set")
    raise EnvironmentError("Environment variable UNIFI_SSH_PASSWORD not set")
SWITCH_IP = "192.168.11.20"
SWITCH_SSH_USER = "admin"
SWITCH_SSH_PASS = os.environ["UNIFI_SSH_PASSWORD"]


def discover(ip=SWITCH_IP, ssh_username=SWITCH_SSH_USER, ssh_password=SWITCH_SSH_PASS, retries = 0):
    """
    Return a UniFi device and connect to it via SSH
    """
    log.d("Discovering UniFi devices...")
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    for i in range(retries+1):
        try:
           client.connect(hostname=ip, username=ssh_username,
                                password=ssh_password, timeout=10)
           return client
        except Exception as e:
            log.w(f"Failed connecting in SSH to UniFi! {e}{', retrying...' if i != retries else ''}")
            time.sleep(2)

    log.w(f"Failed connecting in SSH to UniFi!")
    return None


class NoneFoundError( RuntimeError ):
    def __init__( self, message = None ):
        super().__init__( message  or  'no UniFi device found' )


class UniFiSwitch(device_hub.device_hub):
    def __init__(self, ip=SWITCH_IP, ssh_username=SWITCH_SSH_USER, ssh_password=SWITCH_SSH_PASS):
        """
        :param ip: IP address of the switch.
        :param ssh_username: SSH username.
        :param ssh_password: SSH password.
        """
        self.ip = ip
        self.username = ssh_username
        self.password = ssh_password

        self.client = discover(self.ip, self.username, self.password)
        if self.client is None:
            raise NoneFoundError()

        # ports 1-4 are PoE, 5-8 are regular ports
        self.POE_PORTS = [1, 2, 3, 4]
        self.REGULAR_PORTS = [5, 6, 7, 8]

        self.mac_port_dict = None

    def _init_mac_port_dict(self):
        """
        Initialize mac_port_dict which maps MAC addresses to port numbers
        """
        cmd_out = self._run_command("swctrl mac show")
        self.mac_port_dict = {}

        for line in cmd_out.splitlines()[2:-1]:
            port, vlan, mac, ip = line.split()[:4] # ip is missing sometimes, best to use MAC address
            self.mac_port_dict[mac] = port


    def _get_port_stats(self):
        """
        Get the status of all ports on the switch
        return: a dictionary of port numbers to whether they are up or down
        """
        cmd_out = self._run_command("swctrl port show")
        port_stats = {}
        for line in cmd_out.splitlines()[3:]:
            port_num = int(line.split()[0])
            is_port_up = line.split()[1][0]=="U"
            port_stats[port_num] = is_port_up

        return port_stats

    def connect(self, reset=False):
        if self.client is None:
            self.client = discover(self.ip, self.username, self.password) # assuming no throw because it's done in the c'tor

        if reset:
            # rebooting the switch takes over a minute, so the reboot code is commented out
            log.w("reset flag passed to unifi switch, ignoring it")
            return
            # self._run_command("reboot")
            # time.sleep(5)
            # # we need to reconnect to the device after it's rebooted, might take several tries
            # self.disconnect()
            # self.client = discover(retries=5)

    def is_connected(self):
        if self.client is None:
            return False
        # if the client exists, make sure the connection is open
        transport = self.client.get_transport()
        return transport is not None and transport.is_active()

    def disconnect(self):
        if self.client:
            self.client.close()
            self.client = None

    def all_ports(self):
        """
        :return: a list of all PoE ports on the switch
        """
        return self.POE_PORTS

    def ports(self):
        """
        :return: a list of all PoE ports that are currently enabled
        """
        return [port for port, is_on in self._get_port_stats().items() if is_on and port in self.all_ports()]

    def is_port_enabled(self, port):
        return port in self.ports()

    def port_state(self, port):
        """
        Get the state of a port
        :param port: port number
        :return: "enabled" if the port is enabled, "disabled" otherwise
        """
        return "enabled" if self.is_port_enabled(port) else "disabled"

    def _run_command(self, command):
        """
        Run a command on the switch
        :param command: the command to run
        :return: the output of the command
        """
        if not self.is_connected():
            raise Exception("Not connected to switch.")
        stdin, stdout, stderr = self.client.exec_command(command)
        out = stdout.read().decode().strip()
        err = stderr.read().decode().strip()
        if err:
            log.f(f"Error running command '{command}': {err}")
        # print(f"executed \"{command}\" successfully")
        return out

    def enable_ports(self, ports=None, disable_other_ports=False, sleep_on_change=0):
        if ports is None:
            ports = self.all_ports()

        if disable_other_ports:
            other_ports = set(self.all_ports()) - set(ports)
            self.disable_ports(list(other_ports), sleep_on_change)

        ports_str = ','.join(map(str, ports))
        cmd = f"swctrl poe set auto id {ports_str}"  # cmd should be able to get multiple ports at once
        self._run_command(cmd)

        if sleep_on_change:
            time.sleep(sleep_on_change)
        return True

    def disable_ports(self, ports=None, sleep_on_change=0):
        if ports is None:
            ports = self.all_ports()

        if any(ports) not in self.POE_PORTS:
            log.w("Attempted to disable a non-poe port! ignoring.")
            ports = [port for port in ports if port in self.POE_PORTS]

        ports_str = ','.join(map(str, ports))
        cmd = f"swctrl poe set off id {ports_str}"
        self._run_command(cmd)

        if sleep_on_change:
            time.sleep(sleep_on_change)
        return True

    def get_port_by_location(self, mac_address):
        """
        Get the port number of a device connected to the switch
        Since this isn't usb, we use MAC address ("network location")
        :param mac_address: MAC address of the device
        :return: port number
        """
        if self.mac_port_dict is None:
            self._init_mac_port_dict()
        return self.mac_port_dict[mac_address]


# Example usage:
if __name__ == '__main__':
    unifi = UniFiSwitch()

    for opt,arg in opts:
        if opt in ('--enable'):
            unifi.connect()
            unifi.enable_ports()   # so ports() will return all
        elif opt in ('--disable'):
            unifi.connect()
            unifi.disable_ports()
        elif opt in ('--recycle'):
            unifi.connect()
            unifi.enable_ports()   # so ports() will return all
            unifi.recycle_ports()
        elif opt in ('--reset'):
            unifi.connect( reset = True )