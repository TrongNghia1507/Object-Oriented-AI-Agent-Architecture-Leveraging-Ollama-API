#!/bin/bash
if [ -f "model.env" ]; then
    export $(cat model.env | xargs)
fi

exec "$@"