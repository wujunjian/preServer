#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
export PATH=$PATH:/usr/local/lib
cd /home/dypw

processname=(preServer g08 g1c gclient gtask)

#for check list of 8001
declare -i count
declare -i MEMBER 
count=0

chkprocess()
{
	#ps -ef| grep $1 | grep -v grep  | grep -v khungtaskd
	pid=`ps -ef | grep $1 | grep -v grep | grep -v khungtaskd | awk '{print $2}'`
	if [ "" == "$pid" ]
	then
		cd /home/dypw
		ulimit -c unlimited
		ulimit -n 10000
		echo `date '+%Y-%m-%d %H:%M:%S'` "process [$1] not exist,ready start"
		date
		cd ./$1
		nohup ./$1 &
		cd ..
	fi
}
maxcount=0
while true
do
	for i in "${processname[@]}"
	do
		#if [ $i == ${processname[0]} ]
		#then
			#ps -ef| head -n 1
		#fi
		chkprocess $i
	done


#MEMBER=`netstat -anp|grep g1c|grep 8001|awk '{print $2}'`
MEMBER=0
if [ $MEMBER -gt 10000 ]
then
    let "count=$count+1"
echo `date '+%Y-%m-%d %H:%M:%S'` "recv queue=$MEMBER,count=$count">>restart.log
    if [ $count -gt 10 ]
    then
echo `date '+%Y-%m-%d %H:%M:%S'` "recv queue=$MEMBER,count=$count,g1c killed">>restart.log
        killall g1c
	count=0
	fi
else
    count=0
fi

	sleep 3
	#clear
done

