#!/bin/bash
lsof -w -n -i tcp:29512 | tail -n 1 | cut -d\  -f 2 | xargs kill
lsof -w -n -i tcp:29513 | tail -n 1 | cut -d\  -f 2 | xargs kill
lsof -w -n -i tcp:29514 | tail -n 1 | cut -d\  -f 2 | xargs kill
