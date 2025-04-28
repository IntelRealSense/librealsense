#!/bin/bash

# Start the FastAPI server
uvicorn main:combined_app --host 0.0.0.0 --port 8000