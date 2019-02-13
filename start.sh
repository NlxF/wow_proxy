#!/usr/bin/env bash

set -u

nohup /azeroth-server/bin/proxy_server &>/dev/null &

/azeroth-server/bin/worldserver
