##

while true
do
ps aux | head -n 1
ps aux | grep preServer | grep -v grep
ps aux | grep UdpRecv | grep -v grep

echo "================================================================================="
netstat -anp | head -n 2
netstat -anp | grep :8000 | head -n 5

echo "================================================================================="
netstat -anp | head -n 2
netstat -anp | grep :8001

echo "================================================================================="
netstat -an | head -n 2
netstat -an | grep :8002

sleep 2
clear 
done
