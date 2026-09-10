# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

from fastapi import APIRouter, Depends, HTTPException
from typing import List
import logging
from starlette.concurrency import run_in_threadpool

from app.models.option import OptionInfo, OptionUpdate
from app.services.rs_manager import RealSenseManager, RealSenseError
from app.api.dependencies import get_realsense_manager

router = APIRouter()


@router.get("/", response_model=List[OptionInfo])
async def get_colorizer_options(
    device_id: str,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
):
    """The depth colorizer's controls for a device: color scheme, distances, histogram eq."""
    try:
        return await run_in_threadpool(rs_manager.get_colorizer_options, device_id)
    except RealSenseError as e:
        raise HTTPException(status_code=e.status_code, detail=e.detail)
    except Exception:
        logging.exception("Unexpected error reading colorizer options for %s", device_id)
        raise HTTPException(status_code=500, detail="Unexpected error while reading colorizer options")


@router.put("/{field}/", response_model=OptionInfo)
async def set_colorizer_option(
    device_id: str,
    field: str,
    body: OptionUpdate,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
):
    """Set one colorizer option; returns it as the device reports it afterwards."""
    try:
        return await run_in_threadpool(rs_manager.set_colorizer_option, device_id, field, body.value)
    except RealSenseError as e:
        raise HTTPException(status_code=e.status_code, detail=e.detail)
    except Exception:
        logging.exception("Unexpected error writing colorizer options for %s", device_id)
        raise HTTPException(status_code=500, detail="Unexpected error while writing colorizer options")
