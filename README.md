# ESP8266-HA-MOON

MOON: ESP8266 + Home Assistant

configuration.yaml :

mqtt:
    broker: 192.168.1.10
    port: 1883
    client_id: home-assistant-1
    keepalive: 60
    username: device
    password: password
    protocol: 3.1

light:
  - platform: mqtt
    name: "Moon"
    state_topic: "room/light/moon/state"
    command_topic: "room/light/moon/state/set"
    payload_on: "1" 
    payload_off: "0"
    brightness_state_topic: "room/light/moon/brightness"
    brightness_command_topic: "room/light/moon/brightness/set"
    optimistic: false
    qos: 0

switch:
  - platform: mqtt
    name: "Blood Moon"
    state_topic: "room/light/moon/color"
    command_topic: "room/light/moon/color/set"
    payload_on: "1" 
    payload_off: "0"
    state_on: "1"
    state_off: "0"
    optimistic: false
    qos: 0
    retain: true
