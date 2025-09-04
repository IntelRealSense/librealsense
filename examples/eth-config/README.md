# Ethernet Configuration Example

This example demonstrates how to use the `rs2::eth_config_device` class to read and modify Ethernet configuration settings on RealSense devices that support Ethernet connectivity.

## Overview

The example shows how to:
1. Find devices that support Ethernet configuration
2. Read current Ethernet configuration parameters including both configured and actual values
3. Modify various Ethernet settings safely
4. Restore original configuration values

## Features Demonstrated

### Configuration Parameters
- **Link Speed**: Current Ethernet link speed in Mbps (read-only, 0 if no link established)
- **Link Priority**: Controls whether the device prefers USB or Ethernet connection
- **Link Timeout**: Controls when to fallback from prefered link type to the other (milliseconds)
- **IP Address**: Shows both configured and actual IP address values
- **Network Mask**: Shows both configured and actual subnet mask values  
- **Gateway**: Shows both configured and actual gateway values
- **DHCP Settings**: Dynamic IP configuration status and timeout
- **MTU**: Maximum Transmission Unit size in bytes
- **Transmission Delay**: Inter-packet delay in microseconds

### IP Address Handling
- Reading both configured and actual IP configuration values
- Displaying IP address differences when DHCP is enabled
- Safe handling of invalid IP addresses

## Usage

```bash
./rs-eth-config
```

## Sample Output

```
RealSense Ethernet Configuration Example
=========================================

Found Ethernet-capable device:
  Name: Intel RealSense D555
  Serial: 333422302656
  FW Version: 7.56.19918.835

=== Current Ethernet Configuration ===
Link Speed:    1000 Mbps
Link Priority: Dynamic Eth First
Link Timeout:  4000 milliseconds
IP Address:    192.168.11.55 (actual: 192.168.11.55)
Network Mask:  255.255.255.0 (actual: 255.255.255.0)
Gateway:       192.168.11.1 (actual: 192.168.11.1)
DHCP Enabled:  No
DHCP Timeout:  30 seconds
MTU:           9000 bytes
TX Delay:      0 microseconds

=== Demonstrating Configuration Changes ===

Demonstrating link priority change...
Setting link priority to Eth First
Link priority changed from Dynamic Eth First to Eth First

Demonstrating DHCP timeout change...
Setting DHCP timeout to 60 seconds
DHCP timeout changed from 30 to 60 seconds

Restoring original configuration...
Configuration restored to original values
```

## Important Notes

- For production IP configuration changes, consider using `rs-dds-config` tool.

### Network Configuration Safety
- **IP Address Changes**: This example intentionally avoids changing IP configuration to prevent device disconnection

### Device Requirements
- Device must support Ethernet configuration (check with `supports_eth_config()`)

### Link Speed Information
- Link speed is read-only and reflects the current Ethernet connection speed
- Returns 0 if no Ethernet link is established

### Configured vs Actual Values
- **Configured**: The static IP settings stored in device configuration
- **Actual**: The current IP values in use (may differ when DHCP is enabled)
- When DHCP is enabled, actual values reflect the DHCP-assigned addresses

## Code Structure

### Main Functions
- `print_ethernet_config()`: Displays current configuration including link speed and configured vs actual IP values
- `demonstrate_config_changes()`: Shows how to modify selected settings safely

### Helper Functions
- `link_priority_to_string()`: Converts enum to readable string
- `format_ip_address()`: Formats IP addresses for display, handles invalid addresses
- `print_link_speed()`: Reads and displays current link speed
