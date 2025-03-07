#!/bin/bash
#Load SOS enviromental variables
source sosd.env.sourceme\

set -x

DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export SOS_WORK=${DIR}
export SOS_EVPATH_MEETUP=${DIR}
export SOS_BIN_PATH=/home/users/jalcaraz/src/sos_flow_mod/install/bin/


export TAU_PLUGINS=libTAU-sos-plugin.so
#########export TAU_PLUGINS_PATH=$HOME/tau2/x86_64/lib/shared-mpi-pthread-pdt-sos
export TAU_PLUGINS_PATH=$HOME/tau2/x86_64/lib/shared-papi-mpi-pthread-pdt-sos

start_sos_daemon()
{
    # start the SOS daemon

    echo "Work directory is: $SOS_WORK"
    rm -rf sosd.00000.* profile.* dump.*
    #export SOS_IN_MEMORY_DATABASE=1
    daemon="${SOS_BIN_PATH}sosd -l 0 -a 1 -k 0 -r aggregator -w ${SOS_WORK}"
    echo ${daemon}
    ${daemon} #>& sosd.log &
    sleep 1
}

stop_sos_daemon()
{
    # shut down the daemon.
    if pgrep -x "sosd" > /dev/null; then
        ${SOS_BIN_PATH}sosd_stop
    fi
    sleep 1
}

stop_sos_daemon
start_sos_daemon
