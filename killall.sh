#! /usr/bin/bash

pid_mx=$(ps | pgrep motorx)
kill -SIGTERM $pid_mx
pid_mz=$(ps | pgrep motorz)
kill -SIGTERM $pid_mz
pid_world=$(ps | pgrep world)
kill -SIGTERM $pid_world
pid_watch=$(ps | pgrep watchdog)
kill -SIGTERM $pid_watch