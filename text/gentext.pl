#!/usr/bin/perl -w

# states:
#  0: header
#  1: outside char
#  2: inside char
#  3: insize bitmap

use strict 'refs';

$state = 0;
my %chars;
my %char;
my @charbits;
my ($firstline, $lastline, $line);

sub transpose(@) {
    my @a = @_;
    my @ret = ();
    
    #print "a=".join(', ', @a)."\n";
 outer:
    foreach $i (0..7) {
	$ret[$i] = 0;
	foreach $a (@a) {
	    $ret[$i] >>= 1;
	    $ret[$i] |= ($a & 0x80);
 	    $a = ($a << 1) & 0xff;

	    #print "$i: a=$a ret[$i] = $ret[$i]\n";
	}
	foreach $a (@a) {
	    next outer if $a != 0;
	}
	last;
    }

    #print "a=".join(', ', @a)."\n";
    #print "ret=".join(', ', @ret)."\n";
    return @ret;
}

die "Need font" unless defined $ARGV[0];

open FONT, $ARGV[0] or die "can't open font";

while(<FONT>) {
    chomp;
    my @bits = split / +/, $_;

    #print "$_: state=$state\n";
    
    if ($state == 0) {
	$state = 1 if /^STARTFONT 2\.1/;
    } elsif ($state == 1) {
	$state = 2 if /^STARTCHAR /;
	$state = 0 if /^ENDFONT/;

	#$name = $bits[1] if $bits[0] eq "FONT";
    } elsif ($state == 2) {
	if (/^BITMAP/) {
	    $state = 3;
	    @charbits = ();

	    $firstline = -1;
	    $lastline = -1;
	    $line = 0;
	}
	$state = 3 if /^BITMAP/;

	$thischar = chr $bits[1] if $bits[0] eq "ENCODING";
	#($width, $height) = @bits[1,2] if $bits[0] eq "BBX";
    } elsif ($state == 3) {
	if (/^ENDCHAR/ || !/^[0-9a-fA-F][0-9a-fA-F]/) {
	    $state = 1;

	    next if $firstline == -1 && $thischar ne " ";

	    @charbits = transpose(@charbits);
	    #print "charbits $thischar: ".join(", ", @charbits)."\n";
	    $char{$thischar} = [@charbits];
	    @charbits=();
	    next;
	}

	my $pix = hex $bits[0];

	$firstline = $line if $firstline == -1 and $pix != 0;
	$lastline = $line if $pix != 0;

	$line++;

	push @charbits, $pix;
    }
}
close FONT;

while(<STDIN>) {
    chomp;

    push @messages, $_;
}

my %used;
my @data = ();
my $idx = 0;
foreach $s (@messages) {
    foreach $c (split //, $s) {
	my $d = $char{$c};

	if (!defined $char{$c}) {
	    $d = $char{'_'};
	}

	#print "d{$c}=$d ".join(" ", @$d)."\n";
	if (!defined($used{$c})) {
	    $used{$c} = @data;
	    $off{@data} = $c;
	    $width{$c} = scalar @$d;
	    push @data, scalar @$d;
	    push @data, @$d;

	    die "Too much character data" if @data >= 255;
	}

	#print "used[$c] = $used{$c}\n";
    }
}

print <<EOF;
#ifdef USE_FLASH
#include <progmem.h>
#define FLASH PROGMEM
#else
#define FLASH /* */
#endif

const unsigned char chardata[] FLASH = {
EOF

foreach $i (0..$#data) {
    $c = $data[$i];
    print "\n\t/* $off{$i} */ " if defined $off{$i};
    print "$c, ";
}
print "\n};\n\n";

my @strings;
print "const unsigned char strdata[] FLASH = {\n";
print "\t255, /* initial pad */";

my $col = 0;
my $outoff = 1;
my $width = 0;
foreach $s (@messages) {
    print "\n/* $s */\n";
    $col = 0;
    $s .= chr 255;
    push @strings, $outoff;
    foreach $c (split //, $s) {
	print "\t" if $col == 0;
	if ($c ne chr 255) {
	    print "$used{$c}, ";
	    $width += $width{$c} + 1;
	} else {
	    print "255, ";
	}

	if (++$col > 20) {
	    print "\n";
	    $col = 0;
	}
	$outoff++;
	#die "Too much text" if $outoff >= 255;
    }
    print "/* ($width) */";
    print "\n" unless $col == 0;
}
print "\n" unless $col == 0;
print "};\n\n";

print "const unsigned char strings[] FLASH = {\n\t";

foreach $s (@strings) {
    print "$s, ";
}
print "0";
print "\n};\n";
