ulimit -c unlimited
ulimit -n 65535

killall preServer
killall UdpRecv

sleep 1
nohup ./preServer&
if [ "$1" != "" ]
then
	nohup ./UdpRecv&
fi
