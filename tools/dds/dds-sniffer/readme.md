# dds-sniffer tool

## Goal
This tool monitors the requested DDS domain.
It helps discovering active DDS participants and the topics that they publish/subscribe to.

## Description
The dds-sniffer will print a message for every data reader/writer entity it locates in the domain. The messsage will include the topic name and type, helping the user to match entities that are capable of intercommunication.

## Command Line Parameters
| Flag | Description | Default|
|---|---|---|
|'-h --help'|Show command line help menu||
|'-d --domain < ID >'|dds-sniffer will monitor domain < ID >|0|
|'-s --snapshot'|run momentarily taking a snapshot of the domain||

For example:

'dds-sniffer --domain 42 --snapshot'

will take a snapshot of DDS domain 42

'dds-sniffer'

will monitor DDS domain 0 until stopped by user
