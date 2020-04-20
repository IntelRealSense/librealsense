#!/bin/bash
# this statement checks if there is an instance of the EtherSenseServer running
if [[ ! `ps -eaf | grep "python EtherSenseServer.py" | grep -v grep` ]]; then
# if not, EtherSenseServer is started with the PYTHONPATH set due to cron not passing Env 
    PYTHONPATH=$HOME/.local/lib/python2.7/site-packages python EtherSenseServer.py
fi
