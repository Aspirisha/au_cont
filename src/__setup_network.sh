#!/bin/bash

IP=$1
NS_NAME=$2
echo $NS_NAME

NS_ID=`echo $NS_NAME | cut -d"-" -f2`
ip netns add $NS_NAME

# add loopback
ip netns exec $NS_NAME ip link set dev lo up

# create virtual ethernet devices
VETH0=veth0-aucont-$NS_ID
VETH1=veth1-aucont-$NS_ID
ip link add $VETH0 type veth peer name $VETH1
ip link set $VETH1 netns $NS_NAME


headip=`echo $IP | cut -d"." -f1-3`
tailip=`echo $IP | cut -d"." -f4`
if (( $tailip == 255 )); then
    echo "Invalid ip passed: last octet should be less than 255"
    exit 1
fi

HOST_IP=$headip.$(( $tailip + 1 ))
echo "host ip is $HOST_IP"

ip netns exec $NS_NAME ifconfig $VETH1 $IP netmask 255.255.255.0 up
ifconfig $VETH0 $HOST_IP netmask 255.255.255.0 up

# Setting up gateway for myspace namespace
ip netns exec $NS_NAME route add default gw $HOST_IP

# setup NAT
echo 1 > /proc/sys/net/ipv4/ip_forward
iptables -t nat -A POSTROUTING -o brm -j MASQUERADE
