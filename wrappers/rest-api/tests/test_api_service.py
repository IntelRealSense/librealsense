import pytest
import numpy as np
from unittest.mock import AsyncMock, MagicMock, patch
from fastapi.testclient import TestClient
from fastapi import FastAPI
from .setup_fake_devices import setup_fake_devices
from .mock_dependencies import patch_dependencies, DummyOfferStat
from .pyrealsense_mock import camera_info
from main import app

# Create test client
client = TestClient(app)

class TestRealSenseAPI:
    @classmethod
    def setup_class(cls):
        mock_setup = setup_fake_devices()
        cls.fake_devices = mock_setup["devices"]
        cls.depth_sensors = mock_setup["depth_sensors"]
        cls.color_sensors = mock_setup["color_sensors"]


    @pytest.fixture
    def setup_mock_managers(self, patch_dependencies):
        rs_manager = patch_dependencies["rs_manager"]
        webrtc_manager = patch_dependencies["webrtc_manager"]

        # Configure mock RealSenseManager
        def mock_start_stream(device_id, configs, align_to=None):
            # Set up mock frame queues
            rs_manager.active_streams[device_id] = set(
                [config.stream_type for config in configs]
            )
            rs_manager.frame_queues[device_id] = {}

            for config in configs:
                stream_type = config.stream_type
                rs_manager.frame_queues[device_id][stream_type] = []

                # Add a fake frame to the queue
                if stream_type.lower() == "depth":
                    # Create a fake depth frame (grayscale)
                    frame = np.random.randint(
                        0,
                        255,
                        (config.resolution.height, config.resolution.width),
                        dtype=np.uint8,
                    )
                    # Colorize it to simulate depth colorization
                    frame_rgb = np.zeros(
                        (config.resolution.height, config.resolution.width, 3),
                        dtype=np.uint8,
                    )
                    frame_rgb[:, :, 2] = (
                        frame  # Add to blue channel for colorization effect
                    )
                    frame_data = frame_rgb
                elif stream_type.lower() == "color":
                    # Create a fake color frame
                    frame_data = np.random.randint(
                        0,
                        255,
                        (config.resolution.height, config.resolution.width, 3),
                        dtype=np.uint8,
                    )
                else:
                    # Generic frame for other stream types
                    frame_data = np.random.randint(
                        0,
                        255,
                        (config.resolution.height, config.resolution.width, 3),
                        dtype=np.uint8,
                    )

                metadata = {
                    "timestamp": 12345678,
                    "frame_number": 42,
                    "width": config.resolution.width,
                    "height": config.resolution.height,
                }

                rs_manager.frame_queues[device_id][stream_type].append(
                    (frame_data, metadata)
                )

            # Update pipelines to indicate streaming
            rs_manager.pipelines[device_id] = MagicMock()

            # Return status
            return rs_manager.get_stream_status(device_id)

        def mock_refresh_devices():
            # Populate the devices dictionary with our mock devices
            for dev in self.fake_devices:
                device_id = dev.get_info(camera_info.serial_number)
                rs_manager.devices[device_id] = dev

                # Create device info
                name = dev.get_info(camera_info.name)
                firmware_version = "1.0.0"  # Mock firmware version

                # Get sensors
                sensors = []
                for sensor in dev.sensors:
                    try:
                        sensor_name = sensor.get_info(camera_info.name)
                        sensors.append(sensor_name)
                    except RuntimeError:
                        pass

                # Create device info object
                device_info = {
                    "device_id": device_id,
                    "name": name,
                    "serial_number": device_id,
                    "firmware_version": firmware_version,
                    "physical_port": "USB",
                    "usb_type": "3.0",
                    "product_id": "001",
                    "sensors": sensors,
                    "is_streaming": device_id in rs_manager.pipelines,
                }

                # Convert to DeviceInfo model
                from app.models.device import DeviceInfo

                rs_manager.device_infos[device_id] = DeviceInfo(**device_info)

            return list(rs_manager.device_infos.values())

        # Replace the actual methods
        rs_manager.start_stream = mock_start_stream
        rs_manager.refresh_devices = mock_refresh_devices

        # Configure mock WebRTCManager
        async def mock_create_offer(device_id, stream_types):
            session_id = f"test-session-{device_id}"

            mock_stats_dict = {
                "stat1": DummyOfferStat(type="candidate", id="1234", value=42),
                "stat2": DummyOfferStat(type="track", id="5678", value=99),
            }
            pc = MagicMock()
            pc.getStats = AsyncMock(return_value=mock_stats_dict)

            webrtc_manager.sessions[session_id] = {
                "session_id": session_id,
                "device_id": device_id,
                "stream_types": stream_types,
                "connected": False,
                "pc": pc,
            }

            # Mock offer
            offer = {
                "sdp": "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n",
                "type": "offer",
            }

            return session_id, offer

        webrtc_manager.create_offer = mock_create_offer

        # Add methods to handle webrtc operations
        async def mock_process_answer(session_id, sdp, type):
            if session_id not in webrtc_manager.sessions or type != "answer" or not sdp:
                return False
            return True

        async def mock_add_ice_candidate(session_id, candidate, sdpMid, sdpMLineIndex):
            if session_id not in webrtc_manager.sessions or not candidate:
                return False
            return True

        async def mock_close_session(session_id):
            if session_id in webrtc_manager.sessions:
                del webrtc_manager.sessions[session_id]
            return True

        webrtc_manager.process_answer = mock_process_answer
        webrtc_manager.add_ice_candidate = mock_add_ice_candidate
        webrtc_manager.close_session = mock_close_session

        # Populate with initial data
        rs_manager.refresh_devices()

        return {"rs_manager": rs_manager, "webrtc_manager": webrtc_manager}

    # ----- Tests for the RealSense API -----

    def test_get_devices(self, setup_mock_managers):
        # Test the /devices endpoint
        response = client.get("/api/devices")
        assert response.status_code == 200

        devices = response.json()
        assert len(devices) == 2
        assert devices[0]["name"] == "Test Device 1"
        assert devices[1]["name"] == "Test Device 2"

    def test_get_device_by_id(self, setup_mock_managers):
        # Test the /devices/{device_id} endpoint
        response = client.get("/api/devices/device1")
        assert response.status_code == 200

        device = response.json()
        assert device["device_id"] == "device1"
        assert device["name"] == "Test Device 1"

        # Test with non-existent device
        response = client.get("/api/devices/nonexistent")
        assert response.status_code == 404

    def test_get_sensors(self, setup_mock_managers):
        # Test the /devices/{device_id}/sensors endpoint
        response = client.get("/api/devices/device1/sensors")
        assert response.status_code == 200

        sensors = response.json()
        assert len(sensors) == 2
        assert sensors[0]["type"] in ["Depth Sensor", "RGB Camera"]
        assert sensors[1]["type"] in ["Depth Sensor", "RGB Camera"]

    def test_get_sensor_by_id(self, setup_mock_managers):
        # Test the /devices/{device_id}/sensors/{sensor_id} endpoint
        response = client.get("/api/devices/device1/sensors/device1-sensor-0")
        assert response.status_code == 200

        sensor = response.json()
        assert sensor["sensor_id"] == "device1-sensor-0"

        # Test with non-existent sensor
        response = client.get("/api/devices/device1/sensors/nonexistent")
        assert response.status_code == 404

    def test_get_sensor_options(self, setup_mock_managers):
        # Test the /devices/{device_id}/sensors/{sensor_id}/options endpoint
        response = client.get("/api/devices/device1/sensors/device1-sensor-0/options")
        assert response.status_code == 200

        options = response.json()
        assert len(options) > 0

    def test_get_option_by_id(self, setup_mock_managers):
        # Test the /devices/{device_id}/sensors/{sensor_id}/options/{option_id} endpoint
        # First get available options
        response = client.get("/api/devices/device1/sensors/device1-sensor-0/options")
        options = response.json()
        option_id = options[0]["option_id"]

        response = client.get(
            f"/api/devices/device1/sensors/device1-sensor-0/options/{option_id}"
        )
        assert response.status_code == 200

        option = response.json()
        assert option["option_id"] == option_id

    def test_set_option(self, setup_mock_managers):
        # Test the /devices/{device_id}/sensors/{sensor_id}/options/{option_id} PUT endpoint
        # First get available options
        response = client.get("/api/devices/device1/sensors/device1-sensor-0/options")
        options = response.json()
        option_id = options[0]["option_id"]

        response = client.put(
            f"/api/devices/device1/sensors/device1-sensor-0/options/{option_id}",
            json={"value": 0.5},
        )
        assert response.status_code == 200

    def test_start_stream(self, setup_mock_managers):
        # Test the /devices/{device_id}/stream POST endpoint
        stream_config = {
            "configs": [
                {
                    "sensor_id": "device1-sensor-0",
                    "stream_type": "depth",
                    "format": "z16",
                    "resolution": {"width": 640, "height": 480},
                    "framerate": 30,
                }
            ]
        }

        response = client.post("/api/devices/device1/stream/start", json=stream_config)
        assert response.status_code == 200

        result = response.json()
        assert result["device_id"] == "device1"
        assert result["is_streaming"] == True
        assert "depth" in result["active_streams"]

    def test_stop_stream(self, setup_mock_managers):
        # Test the /devices/{device_id}/stream DELETE endpoint
        # First start streaming
        stream_config = {
            "configs": [
                {
                    "sensor_id": "device1-sensor-0",
                    "stream_type": "depth",
                    "format": "z16",
                    "resolution": {"width": 640, "height": 480},
                    "framerate": 30,
                }
            ]
        }
        response = client.post("/api/devices/device1/stream/stop", json=stream_config)

        assert response.status_code == 200

        result = response.json()
        assert result["device_id"] == "device1"
        assert result["is_streaming"] == False

    def test_get_stream_status(self, setup_mock_managers):
        # Test the /devices/{device_id}/stream GET endpoint
        response = client.get("/api/devices/device1/stream/status")
        assert response.status_code == 200

        status = response.json()
        assert status["device_id"] == "device1"
        assert "is_streaming" in status

    # ----- Tests for the WebRTC API -----

    @pytest.mark.asyncio
    async def test_create_webrtc_offer(self, setup_mock_managers):
        # First start streaming
        stream_config = {
            "configs": [
                {
                    "sensor_id": "device1-sensor-0",
                    "stream_type": "depth",
                    "format": "z16",
                    "resolution": {"width": 640, "height": 480},
                    "framerate": 30,
                }
            ]
        }
        client.post("/api/devices/device1/stream", json=stream_config)

        # Test the /webrtc/offer POST endpoint
        webrtc_config = {"device_id": "device1", "stream_types": ["depth"]}

        response = client.post("/api/webrtc/offer", json=webrtc_config)
        assert response.status_code == 200

        result = response.json()
        assert "session_id" in result
        assert "sdp"
        assert "type"
        assert result["session_id"] == "test-session-device1"
        assert result["type"] == "offer"

    @pytest.mark.asyncio
    async def test_process_webrtc_answer(self, setup_mock_managers):
        # First create offer
        webrtc_config = {"device_id": "device1", "stream_types": ["depth"]}
        response = client.post("/api/webrtc/offer", json=webrtc_config)
        session_id = response.json()["session_id"]

        # Test the /webrtc/sessions/{session_id}/answer POST endpoint
        answer = {
            "session_id": session_id,
            "sdp": "v=0\r\no=- 0 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\n",
            "type": "answer",
        }

        response = client.post(f"/api/webrtc/answer", json=answer)
        assert response.status_code == 200
        assert response.json()["success"] == True

    @pytest.mark.asyncio
    async def test_add_ice_candidate(self, setup_mock_managers):
        # First create offer
        webrtc_config = {"device_id": "device1", "stream_types": ["depth"]}
        response = client.post("/api/webrtc/offer", json=webrtc_config)
        session_id = response.json()["session_id"]

        # Test the /webrtc/sessions/{session_id}/ice POST endpoint
        ice_candidate = {
            "session_id": session_id,
            "candidate": "candidate:0 1 UDP 2122260223 192.168.1.1 49152 typ host",
            "sdpMid": "0",
            "sdpMLineIndex": 0,
        }

        response = client.post(f"/api/webrtc/ice-candidates", json=ice_candidate)
        assert response.status_code == 200
        assert response.json()["success"] == True

    @pytest.mark.asyncio
    async def test_get_webrtc_session(self, setup_mock_managers):
        # First create offer
        webrtc_config = {"device_id": "device1", "stream_types": ["depth"]}
        response = client.post("/api/webrtc/offer", json=webrtc_config)
        session_id = response.json()["session_id"]

        # Test the /webrtc/sessions/{session_id} GET endpoint
        response = client.get(f"/api/webrtc/sessions/{session_id}")
        assert response.status_code == 200

        result = response.json()
        assert result["session_id"] == session_id
        assert result["device_id"] == "device1"
        assert "depth" in result["stream_types"]

    @pytest.mark.asyncio
    async def test_close_webrtc_session(self, setup_mock_managers):
        # First create offer
        webrtc_config = {"device_id": "device1", "stream_types": ["depth"]}
        response = client.post("/api/webrtc/offer", json=webrtc_config)
        session_id = response.json()["session_id"]

        # Test the /webrtc/sessions/{session_id} DELETE endpoint
        response = client.delete(f"/api/webrtc/sessions/{session_id}")
        assert response.status_code == 200
        assert response.json()["success"] == True

        # Verify session is closed
        response = client.get(f"/api/webrtc/sessions/{session_id}")
        assert response.status_code == 404
