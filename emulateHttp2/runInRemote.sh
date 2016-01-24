#!/bin/bash
script="sudo perl set_bw_limit.pl 10mbit"
ssh -t labserver "$script"
