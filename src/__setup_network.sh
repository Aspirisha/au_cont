#!/bin/bash

IP=$1
NS_NAME=$2
echo $NS_NAME

NS_ID=`echo $NS_NAME | cut -d"-" -f2`

# create new network namespace
ip netns add $NS_NAME

# create virtual ethernet devices
VETH0=veth0-aucont-$NS_ID # for host
VETH1=veth1-aucont-$NS_ID # for container

## Create veth link
ip link add $VETH0 type veth peer name $VETH1

# add VETH1 to new namespace
ip link set $VETH1 netns $NS_NAME


headip=`echo $IP | cut -d"." -f1-3`
tailip=`echo $IP | cut -d"." -f4`
if (( $tailip == 255 )); then
    echo "Invalid ip passed: last octet should be less than 255"
    exit 1
fi

HOST_IP=$headip.$(( $tailip + 1 ))
echo "host ip is $HOST_IP"

#ip netns exec $NS_NAME ifconfig $VETH1 $IP netmask 255.255.255.0 up
#ifconfig $VETH0 $HOST_IP netmask 255.255.255.0 up

# Setting up gateway for myspace namespace
#ip netns exec $NS_NAME route add default gw $HOST_IP

# setup NAT
#echo 1 > /proc/sys/net/ipv4/ip_forward
#iptables -t nat -A POSTROUTING -o brm -j MASQUERADE

# Setup IP address of v-eth1.
ip addr add $HOST_IP/24 dev $VETH0
ip link set $VETH0 up

# Setup IP address of v-peer1.
ip netns exec $NS_NAME ip addr add $IP/24 dev $VETH1
ip netns exec $NS_NAME ip link set $VETH1 up
# add loopback
ip netns exec $NS_NAME ip link set dev lo up

ip netns exec $NS_NAME ip route add default via $HOST_IP

# Share internet access between host and NS.

# Enable IP-forwarding.
echo 1 > /proc/sys/net/ipv4/ip_forward


# Flush forward rules, policy DROP by default.
iptables -P FORWARD DROP
iptables -F FORWARD

# Flush nat rules.
iptables -t nat -F
# Enable masquerading of 10.200.1.0.
iptables -t nat -A POSTROUTING -s 10.200.1.0/255.255.255.0 -o eth0 -j MASQUERADE

# Allow forwarding between eth0 and v-eth1.
iptables -A FORWARD -i eth0 -o $VETH0 -j ACCEPT
iptables -A FORWARD -o eth0 -i $VETH0 -j ACCEPT