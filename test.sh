#!/bin/sh
make

echo && echo "Running ping..."
timeout 2.5 ping $1 > out1.txt

echo && echo "Running ft_ping..."
sudo timeout 2.5 ./ft_ping $1 > out2.txt

echo && echo "Diff :"
diff out1.txt out2.txt