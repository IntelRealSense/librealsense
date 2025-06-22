from fastapi import APIRouter, Depends, HTTPException
from typing import List


from app.models.device import DeviceInfo
from app.services.rs_manager import RealSenseManager
from app.api.dependencies import get_realsense_manager

router = APIRouter()

@router.get("/", response_model=List[DeviceInfo])
async def get_devices(
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
    #user: dict = Depends(get_current_user)  -> enable this if security is needed
):
    """
    Get a list of all connected RealSense devices.
    """
    return rs_manager.get_devices()

@router.get("/{device_id}", response_model=DeviceInfo)
async def get_device(
    device_id: str,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
    #user: dict = Depends(get_current_user) -> enable this if security is needed
):
    """
    Get details of a specific RealSense device.
    """
    try:
        return rs_manager.get_device(device_id)
    except Exception as e:
        raise HTTPException(status_code=404, detail=str(e))

@router.post("/{device_id}/hw_reset", response_model=bool)
async def hw_reset_device(
    device_id: str,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
    #user: dict = Depends(get_current_user) -> enable this if security is needed
):
    """
    Perform a hardware reset on a specific RealSense device.
    """
    try:
        return rs_manager.reset_device(device_id)
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))