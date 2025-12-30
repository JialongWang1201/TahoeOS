#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

python3 -m py_compile tools/ble_proto.py

req_status="$(python3 tools/ble_proto.py encode-request --seq 7 --command GET_STATUS --payload '')"
printf '%s\n' "$req_status" | grep '^#'
status_json="$(python3 tools/ble_proto.py decode "$req_status")"
printf '%s\n' "$status_json" | grep '"type": "request"'
printf '%s\n' "$status_json" | grep '"cmd": "GET_STATUS"'
printf '%s\n' "$status_json" | grep '"seq": 7'

req_time="$(python3 tools/ble_proto.py encode-request --seq 8 --command SET_TIME --payload 'datetime=20260308153045')"
time_json="$(python3 tools/ble_proto.py decode "$req_time")"
printf '%s\n' "$time_json" | grep '"cmd": "SET_TIME"'
printf '%s\n' "$time_json" | grep 'datetime=20260308153045'

req_cfg="$(python3 tools/ble_proto.py encode-request --seq 9 --command SET_CFG --payload 'ble=1,wrist=1,light=80')"
cfg_json="$(python3 tools/ble_proto.py decode "$req_cfg")"
printf '%s\n' "$cfg_json" | grep '"cmd": "SET_CFG"'
printf '%s\n' "$cfg_json" | grep 'ble=1,wrist=1,light=80'

req_notify="$(python3 tools/ble_proto.py encode-request --seq 10 --command PUSH_NOTIFY --payload 'title=Phone,body=Ping from app')"
notify_json="$(python3 tools/ble_proto.py decode "$req_notify")"
printf '%s\n' "$notify_json" | grep '"cmd": "PUSH_NOTIFY"'
printf '%s\n' "$notify_json" | grep 'title=Phone,body=Ping from app'
req_lock_get="$(python3 tools/ble_proto.py encode-request --seq 11 --command GET_LOCK --payload '')"
lock_get_json="$(python3 tools/ble_proto.py decode "$req_lock_get")"
printf '%s\n' "$lock_get_json" | grep '"cmd": "GET_LOCK"'

req_lock_set="$(python3 tools/ble_proto.py encode-request --seq 12 --command SET_LOCK --payload 'enabled=1,pin=2580')"
lock_set_json="$(python3 tools/ble_proto.py decode "$req_lock_set")"
printf '%s\n' "$lock_set_json" | grep '"cmd": "SET_LOCK"'
printf '%s\n' "$lock_set_json" | grep 'enabled=1,pin=2580'
req_nfc_get="$(python3 tools/ble_proto.py encode-request --seq 13 --command GET_NFC --payload '')"
nfc_get_json="$(python3 tools/ble_proto.py decode "$req_nfc_get")"
printf '%s\n' "$nfc_get_json" | grep '"cmd": "GET_NFC"'

req_nfc_set="$(python3 tools/ble_proto.py encode-request --seq 14 --command SET_NFC --payload 'enabled=1,slot=1')"
nfc_set_json="$(python3 tools/ble_proto.py decode "$req_nfc_set")"
printf '%s\n' "$nfc_set_json" | grep '"cmd": "SET_NFC"'
printf '%s\n' "$nfc_set_json" | grep 'enabled=1,slot=1'
req_sport="$(python3 tools/ble_proto.py encode-request --seq 15 --command GET_SPORT_RECORDS --payload '')"
sport_json="$(python3 tools/ble_proto.py decode "$req_sport")"
printf '%s\n' "$sport_json" | grep '"cmd": "GET_SPORT_RECORDS"'
printf '%s\n' "$sport_json" | grep '"seq": 15'
req_weather="$(python3 tools/ble_proto.py encode-request --seq 16 --command GET_WEATHER --payload '')"
weather_json="$(python3 tools/ble_proto.py decode "$req_weather")"
printf '%s\n' "$weather_json" | grep '"cmd": "GET_WEATHER"'
printf '%s\n' "$weather_json" | grep '"seq": 16'

gb_notify="$(python3 tools/ble_proto.py encode-gb --seq 21 --event '{"t":"notify","title":"Phone","body":"Ping from Gadgetbridge"}')"
gb_notify_json="$(python3 tools/ble_proto.py decode "$gb_notify")"
printf '%s\n' "$gb_notify_json" | grep '"cmd": "PUSH_NOTIFY"'
printf '%s\n' "$gb_notify_json" | grep 'title=Phone,body=Ping from Gadgetbridge'

gb_weather="$(python3 tools/ble_proto.py encode-gb --seq 22 --event '{"t":"weather","temp":24,"low":19,"high":27,"txt":"Rain"}')"
gb_weather_json="$(python3 tools/ble_proto.py decode "$gb_weather")"
printf '%s\n' "$gb_weather_json" | grep '"cmd": "SET_WEATHER"'
printf '%s\n' "$gb_weather_json" | grep 'temp=24,low=19,high=27,text=Rain'

gb_find="$(python3 tools/ble_proto.py encode-gb --seq 23 --event '{"t":"find","n":true}')"
gb_find_json="$(python3 tools/ble_proto.py decode "$gb_find")"
printf '%s\n' "$gb_find_json" | grep '"cmd": "FIND_WATCH"'
printf '%s\n' "$gb_find_json" | grep '"payload": "state=1"'

gb_time="$(python3 tools/ble_proto.py encode-gb --seq 24 --event '{"t":"time","datetime":"20260320153045"}')"
gb_time_json="$(python3 tools/ble_proto.py decode "$gb_time")"
printf '%s\n' "$gb_time_json" | grep '"cmd": "SET_TIME"'
printf '%s\n' "$gb_time_json" | grep 'datetime=20260320153045'

resp_ok="$(python3 tools/ble_proto.py encode-response --seq 8 --ack ACK --code 0 --payload 'datetime=20260308153045')"
resp_json="$(python3 tools/ble_proto.py decode "$resp_ok")"
printf '%s\n' "$resp_json" | grep '"type": "response"'
printf '%s\n' "$resp_json" | grep '"ack": "ACK"'
printf '%s\n' "$resp_json" | grep '"code": 0'

resp_sport="$(python3 tools/ble_proto.py encode-response --seq 15 --ack ACK --code 0 --payload '2;03130815,642,1245,88,132,18;03121840,1800,3280,121,168,-6')"
resp_sport_json="$(python3 tools/ble_proto.py decode "$resp_sport")"
printf '%s\n' "$resp_sport_json" | grep '"payload": "2;03130815,642,1245,88,132,18;03121840,1800,3280,121,168,-6"'
resp_weather="$(python3 tools/ble_proto.py encode-response --seq 16 --ack ACK --code 0 --payload 'valid=1,temp=26,low=20,high=31,text=Cloudy')"
resp_weather_json="$(python3 tools/ble_proto.py decode "$resp_weather")"
printf '%s\n' "$resp_weather_json" | grep '"payload": "valid=1,temp=26,low=20,high=31,text=Cloudy"'

printf 'ble_proto_host_tests: OK\n'
