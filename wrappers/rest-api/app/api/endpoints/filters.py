# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

from fastapi import APIRouter, Depends, HTTPException
from typing import Any, Dict
import logging
from starlette.concurrency import run_in_threadpool

from app.models.option import OptionInfo, OptionUpdate
from app.services.rs_manager import RealSenseManager, RealSenseError
from app.api.dependencies import get_realsense_manager

router = APIRouter()


@router.get("/", response_model=Dict[str, Any])
async def get_sensor_filters(
    device_id: str,
    sensor_id: str,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
):
    """A sensor's post-processing filters, keyed by filter name, each with its options."""
    try:
        return await run_in_threadpool(rs_manager.get_sensor_filters, device_id, sensor_id)
    except RealSenseError as e:
        raise HTTPException(status_code=e.status_code, detail=e.detail)
    except Exception:
        logging.exception("Unexpected error reading filters for %s/%s", device_id, sensor_id)
        raise HTTPException(status_code=500, detail="Unexpected error while reading filters")


# Ahead of the per-option route below, which would otherwise take "enabled" for a field.
@router.put("/{filter_name}/enabled/")
async def set_filter_enabled(
    device_id: str,
    sensor_id: str,
    filter_name: str,
    body: OptionUpdate,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
):
    """Bypass or apply one filter."""
    try:
        return await run_in_threadpool(
            rs_manager.set_filter_enabled, device_id, sensor_id, filter_name, body.value
        )
    except RealSenseError as e:
        raise HTTPException(status_code=e.status_code, detail=e.detail)
    except Exception:
        logging.exception("Unexpected error toggling a filter for %s/%s", device_id, sensor_id)
        raise HTTPException(status_code=500, detail="Unexpected error while toggling a filter")


@router.put("/{filter_name}/{field}/", response_model=OptionInfo)
async def set_filter_option(
    device_id: str,
    sensor_id: str,
    filter_name: str,
    field: str,
    body: OptionUpdate,
    rs_manager: RealSenseManager = Depends(get_realsense_manager),
):
    """Set one option on one filter; returns it as the device reports it afterwards."""
    try:
        return await run_in_threadpool(
            rs_manager.set_filter_option, device_id, sensor_id, filter_name, field, body.value
        )
    except RealSenseError as e:
        raise HTTPException(status_code=e.status_code, detail=e.detail)
    except Exception:
        logging.exception("Unexpected error writing filters for %s/%s", device_id, sensor_id)
        raise HTTPException(status_code=500, detail="Unexpected error while writing filters")
