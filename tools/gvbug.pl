#!/usr/bin/perl

if( $#ARGV != 0 ) {
  print "No input file.\n";
  exit;
}

$tmpfile = "/tmp/u2ps.$$";

open(PS, $ARGV[0]) || die "Can't open\n";
open(OUT, "> $tmpfile") || die "Can't open\n";
while(<PS>) {
  if( /^\/PageSize \[(\S+) (\S+)\]/ ) {
    print OUT "/PageSize \[841 841\]\n";
    next;
  }
  print OUT;
}
close(PS);
close(OUT);
system("gv -landscape -scale 2 $tmpfile");
unlink($tmpfile);
