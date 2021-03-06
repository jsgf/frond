#!/usr/bin/perl

my @giz = ();

print "/* Generated by mkgizmo.pl: do not edit */\n";
print "#include \"pwm.h\"\n";

foreach $i (@ARGV) {
    $i =~ s,^.*/,,;
    $i =~ s,\.o$,,;

    push @giz, $i;

    print "extern giz_init_t ${i}_init;\n";
    print "extern giz_pix_t ${i}_pix;\n\n";
}

print "static const struct gizmo gizmos[] = {\n";

foreach $i (@giz) {
    print "\t{ ${i}_init, ${i}_pix },\n";
}
print "};\n";
print "#define ngizmos (sizeof(gizmos)/sizeof(*gizmos))\n";
