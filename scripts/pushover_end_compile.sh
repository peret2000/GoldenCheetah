#!/bin/bash

# Check whether .bashrc has been loaded (for example, cron does not load it)
if [[ -z "${ENV_LOADED}" ]]; then
        source $HOME/.profile
fi


[ -f "$2" ] && msg="$(cat $2)" || msg="$2"

if [[ -v PUSHOVERTOKEN && -v PUSHOVERUSER ]]; then
  curl -s \
    --form-string "token=$PUSHOVERTOKEN" \
    --form-string "user=$PUSHOVERUSER" \
    --form-string "title=$1" \
    --form-string "message=$msg" \
    https://api.pushover.net/1/messages.json
fi
