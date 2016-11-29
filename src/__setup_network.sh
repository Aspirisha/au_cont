#!/bin/bash

CALLER=$1
IP=$2
NS_ID=$3
NS_NAME=aucontnet-$NS_ID

# create virtual ethernet devices
VETH0=veth0-aucont-$NS_ID # for host
VETH1=veth1-aucont-$NS_ID # for container

headip=`echo $IP | cut -d"." -f1-3`
tailip=`echo $IP | cut -d"." -f4`
if (( $tailip == 255 )); then
    echo "Invalid ip passed: last octet should be less than 255"
    exit 1
fi

HOST_IP=$headip.$(( $tailip + 1 ))

if [ $CALLER = "host" ]; then
    ## Create veth link
    ip link add $VETH0 type veth peer name $VETH1
    # add VETH1 to new namespace
    ip link set $VETH1 netns $NS_NAME
    # Setup IP address of v-eth1.
    ip addr add $HOST_IP/24 dev $VETH0
    ip link set $VETH0 up
    # Enable IP-forwarding.
    echo 1 > /proc/sys/net/ipv4/ip_forward
elif [ $CALLER = "cont" ]; then
    # Setup IP address of v-peer1.
    ip addr add $IP/24 dev $VETH1
    ip link set $VETH1 up
    # add loopback
    ip link set dev lo up

    ip route add default via $HOST_IP
fi