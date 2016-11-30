#!/bin/bash

CALLER=$1
IP=$2
NS_ID=$3

# create virtual ethernet devices
HOST_VETH=veth0-aucont-$NS_ID # for host
CONT_VETH=veth1-aucont-$NS_ID # for container

headip=`echo $IP | cut -d"." -f1-3`
tailip=`echo $IP | cut -d"." -f4`
if (( $tailip == 255 )); then
    echo "Invalid ip passed: last octet should be less than 255"
    exit 1
fi

HOST_IP=$headip.$(( $tailip + 1 ))


if [ $CALLER = "host" ]; then
    # this branch is to be run by daemon
    # since daemon runs under sudo, we are cool here
    PID=$4
    ## Create veth link
    ip link add $HOST_VETH type veth peer name $CONT_VETH
    # add CONT_VETH to new namespace
    ip link set $CONT_VETH netns $PID
    # Setup IP address of v-eth1.
    ip addr add $HOST_IP/24 dev $HOST_VETH
    ip link set $HOST_VETH up
    # Enable IP-forwarding.
    echo 1 > /proc/sys/net/ipv4/ip_forward
elif [ $CALLER = "cont" ]; then
    # Setup IP address of v-peer1.
    ip addr add $IP/24 dev $CONT_VETH
    ip link set $CONT_VETH up
    # add loopback
    ip link set dev lo up

    ip route add default via $HOST_IP
fi