use strict;
use warnings;

if (@ARGV != 1 && @ARGV != 1) {
        print "Usage: set_out_bw.pl [bw]\n";
        print "Example: set_out_bw.pl 10mbit\n";
        print "Usage: set_out_bw.pl clean\n";
        die;
}

if (@ARGV == 1 && $ARGV[0] eq "clean") {
        system("tc qdisc del dev eth0 root");
        print "cleaned\n";
} else {
        die if (not $ARGV[0] =~ /mbit/);
        system("tc qdisc del dev eth0 root");

        if (@ARGV == 2) {
								die;
        } else {
                system("tc qdisc add dev eth0 root handle 1: cbq avpkt 1500 bandwidth 100mbit");
                system("tc class add dev eth0 parent 1: classid 1:1 cbq rate $ARGV[0] allot 1500 prio 1 bounded isolated");
                system("tc filter add dev eth0 parent 1: protocol ip prio 16 u32 match ip sport 6003 0xffff flowid 1:1"); 
                #system("tc qdisc add dev eth0 parent 1:1 handle 10: netem delay 50ms limit 2000");      #need "limit" for udp
                #system("tc qdisc add dev eth0 parent 1:1 handle 10: netem delay 50ms loss 2%");        #to further set latency and loss
                print "set to $ARGV[0]\n";
        }
}

