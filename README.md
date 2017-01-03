# SmartAirCondition

Internet Controlled AirCondition with Arduino and Google AppEngine

## Overview

This project allows to control any IR controllable device such as air-condition, TV, etc from the internet.  
This system does not build the IR command but record specific commands using IR Received LED and send them using IR Sender LED.

## Arduino

An Arduino board has a small firmware that poll on backend server and when see a new command send it using an IR Sender LED attached to it.

## Server

The server side app is written in Python and runs on Google App Engine. It expose human UI to control the device from one side and REST API on the other side to which the Arduino talks to.
