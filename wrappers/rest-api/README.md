# LibRS Rest API server
## Overview

This Python library provides a convenient wrapper to interact with the [RealSense REST API], a FastAPI-based service that exposes the functionality of the Intel RealSense SDK (librealsense) over a network.

It simplifies remote control and data streaming from RealSense devices by handling communication protocols for:

1.  **RESTful Operations:** For device discovery, configuration, and control.
2.  **WebRTC:** For efficient, browser-compatible live video streaming (RGB, Depth).
3.  **Socket.IO:** For real-time streaming of frame metadata and point cloud data.
    * In order to decode point cloud data in the client, follow those steps:
        * Decode Base64 string to Uint8Array
        * 'bytes' now holds the raw byte data that originated from the NumPy array
        * Interpret bytes as Float32Array.
        IMPORTANT ASSUMPTION: We assume the original NumPy array used float32.
        If it used float64 (Python's default float), use Float64Array here.
        The ArrayBuffer gives access to the raw binary data buffer of the Uint8Array.
        The Float32Array constructor creates a *view* over that buffer, interpreting
        groups of 4 bytes as 32-bit floats without copying data.
        * Now you can use the 'vertices' Float32Array
    * Motion raw data is also propogated using this socket.io channel

## Support Features

*   **Device Management:**
    *   List connected RealSense devices.
    *   Get detailed information about specific devices (name, serial, firmware, etc.).
    *   Perform hardware resets remotely.
*   **Sensor & Option Control:**
    *   List available sensors on a device.
    *   Get detailed sensor information and supported stream profiles.
    *   List and retrieve current values for sensor options (e.g., exposure, gain).
    *   Update writable sensor options.
*   **REST-based Stream Control:**
    *   Start/Stop streams with specific configurations (resolution, format, FPS).
    *   Check the streaming status of a device.
*   **WebRTC Video Streaming:**
    *   Initiate WebRTC sessions with the API to receive live video feeds.
    *   Handles the offer/answer and ICE candidate exchange mechanisms required for WebRTC setup (via REST endpoints).
*   **Socket.IO Data Streaming:**
    *   Establish a Socket.IO connection to the API.
    *   Receive real-time events containing:
        *   Frame metadata (timestamps, frame numbers, etc.).
        *   Point cloud data streams.

## Missing Features
* Not all SDK capabilities are yet supported(e.g. Texture Mapping, Post processing filters,...)

**All endpoints and supported features are documented and available for interactive exploration at the `/docs` endpoint using the built-in OpenAPI (Swagger) UI.**

## Installation


1. **Create a virtual environment:**

   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```

2. **Install dependencies:**

   ```bash
   pip3 install -r requirements.txt
   ```

3. **Run API server:**

   ```bash
   run start_server.sh
   ```

4. **Run tests(inside tests folder):**

   ```bash
   pytest test_api_service.py
   ```


## Testing

1. **To enable testing add the following to the requirements.txt**
    ```bash
    pytest==8.3.5
    pytest-asyncio==0.26.0
    typeguard==4.4.4
    jinja2==3.1.6
    pyyaml==6.0.2
    lark==1.2.2
    httpx==0.28.1
    ```