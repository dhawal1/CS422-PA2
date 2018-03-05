#!/usr/bin/env perl

# Author: Jared Stark;   Created: Mon Aug 16 11:38:57 PDT 2004
# Description: Script for generating a report of the mispredict rates.

if (-1 == $#ARGV) {
    $trace_type = 'without-values';
}
elsif ((0 == $#ARGV) && ('with-values' eq $ARGV[0])) {
    $trace_type = 'with-values';
}
elsif ((0 == $#ARGV) && ('without-values' eq $ARGV[0])) {
    $trace_type = 'without-values';
}
else {
    die qq{usage: $0 [ with-values | without-values ]\n};
}

@trace_list = (
    
    
    'DIST-INT-1',
    'DIST-INT-2',
    'DIST-INT-3',
    
    'DIST-MM-1',
    'DIST-MM-2',
    'DIST-SERV-1',
    'DIST-SERV-2',
    'DIST-SERV-3',
    'DIST-SERV-4',
    'DIST-SERV-5'
);


foreach $trace (@trace_list) {
    print $trace, "\n";
    $output = `./predictor traces/$trace_type/$trace`;
    print $output;
    print "\n";
}

