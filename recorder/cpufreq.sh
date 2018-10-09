lscpu | grep "GHz" | awk '{print $NF}' | sed 's/GHz//'
