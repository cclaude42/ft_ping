#!/bin/sh
make

echo && echo "Running ping..."
timeout 2.5 ping $@

echo && echo "Running ft_ping..."
sudo timeout 2.5 ./ft_ping $@
