#!/usr/bin/env perl

use 5.12.0;
use warnings;
use autodie 'open';

my $BINFILE  = "bin/picus.bin";
my $FATFILE  = @ARGV     ? $ARGV[0] : "../mustore/test/_test_fat12.bin";
my $FATSTART = @ARGV > 1 ? $ARGV[1] : 0x8000;

die "Binary does not exist" unless -f $BINFILE;

open my $fat, "<",  $FATFILE;
open my $bin, ">>", $BINFILE;

my $BINSIZE = -s $BINFILE;
if ($BINSIZE < $FATSTART) {
    say "Appending " . ($FATSTART - $BINSIZE) . " bytes";

    syswrite $bin, "\x00"x($FATSTART - $BINSIZE);
}

die "Binary too large\n" if $BINSIZE > $FATSTART;

say "Writing FAT image";
while (my $read = sysread($fat, my $buffer, 1024)) {
    syswrite $bin, $buffer;
}

close $fat;
close $bin;
