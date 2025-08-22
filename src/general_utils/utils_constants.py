class ShellCommands:
    INIT_ADDR_CMD = """/sbin/ifconfig |grep -B1 "inet" |awk '{ if ( $1 == "inet" ) { print $2 } else if ( $2 == "Link" ) { printf "%s:" ,$1 } }' |awk -F: '{ print $1 ": " $3 }'"""
    IFCONFIG_IB0 = """/sbin/ifconfig ib0 |grep -B1 "inet" |awk '{ if ( $1 == "inet" ) { print $2 } else if ( $2 == "Link" ) { printf "%s:" ,$1 } }' |awk -F: '{ print $1 ": " $3 }'"""
    IFCONFIG = "ifconfig"
    SSH_CMD = "ssh -o 'StrictHostKeyChecking=no' {user_name}@{device} '{command}'"
    SSH_CMD_IPV6 = "ssh -6 'StrictHostKeyChecking=no' {user_name}@{device} '{command}'"