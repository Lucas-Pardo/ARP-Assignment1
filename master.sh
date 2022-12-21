#! /usr/bin/bash

./bin/motorx &
pid_mx=$(ps | pgrep motorx)
echo $pid_mx
./bin/motorz &
pid_mz=$(ps | pgrep motorz)
echo $pid_mz
./bin/world &
pid_world=$(ps | pgrep world)
echo $pid_world
/usr/bin/gnome-terminal -e ./bin/command 
pid_cmd=$(ps | pgrep command)
echo $pid_cmd
/usr/bin/gnome-terminal -x ./bin/inspection $pid_mx $pid_mz $pid_cmd 
pid_ins=$(ps | pgrep inspection)
echo $pid_ins
./bin/watchdog $pid_mx $pid_mz $pid_world $pid_cmd $pid_ins &
pid_watch=$(ps | pgrep watchdog)
# echo $pid_watch
# echo $(date +%T)
# wait $pid_cmd
# echo $(date +%T)
# wait $pid_ins
# echo $(date +%T)
# kill -SIGTERM pid_watch 
# echo "Finished"
