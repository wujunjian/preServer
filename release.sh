

tar cvfz `cat /etc/issue | awk '{print $1 $3}' | head -n 1`_`date +%Y%m%d%H%M%S`_preServer.tar.gz ./preServer ./UdpRecv ./Start.sh ./config.ini ./MonitorProcess.sh ./chkprocess.sh
