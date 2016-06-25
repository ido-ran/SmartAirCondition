# SmartAirConditionServer

This is the server application for SmartAirCondition project.
It is very simple and support only turning it on and off.

## /
The main page - show last 10 operations and allow to perform one

## /do
POST the new operation

## /api
Open url that reutnr the operation (0-off, 1-on) and a number represent as "last-update" to know if another operation of the same type was requrested.

## Dependencies
- webapp2
- jinja2
- Twitter Bootstrap
