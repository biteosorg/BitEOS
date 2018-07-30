#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./clbes.sh
# Run single cmd:  sudo ./clbes.sh <clbes paramers>

PREFIX="docker-compose exec nodbesd clbes"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "clbes " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX "$@"
fi
