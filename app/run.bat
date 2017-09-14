@echo off
set query_for_time=0
if %query_for_time% equ 1 (
	start clock 2010/09/15-00:00:00 60
) else (
	echo 1-1-10>e:\time_init.txt
	echo 60>>e:\time_init.txt
)
create_process target