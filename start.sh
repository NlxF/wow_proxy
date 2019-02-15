#!/usr/bin/env bash

set -u

# nohup /azeroth-server/bin/proxy_server &>/dev/null &
nohup /azeroth-server/bin/proxy_server &

/azeroth-server/bin/worldserver
