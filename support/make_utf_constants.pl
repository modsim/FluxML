#!/usr/bin/perl

use Text::Wrap qw(wrap 75);
use strict;

my $utf_cname;
my $utf_value;
my $utf_def;
my $line;
my $c;
my $i;
my %sym_map = (
	' ' => "chSpace",
	'&' => "chAmpersand",
	'@' => "chAt",
	'!' => "chBang",
	'^' => "chCaret",
	'.' => "chPeriod",
	',' => "chComma",
	'$' => "chDollarSign",
	'"' => "chDoubleQuote",
	'=' => "chEqual",
	"'" => "chGrave",
	'%' => "chPercent",
	'|' => "chPipe",
	'?' => "chQuestion",
	':' => "chColon",
	';' => "chSemiColon",
	'-' => "chDash",
	'+' => "chPlus",
	'*' => "chAsterisk",
	'/' => "chForwardSlash",
	'\\' => "chBackSlash",
	'(' => "chOpenParen",
	')' => "chCloseParen",
	'[' => "chOpenSquare",
	']' => "chCloseSquare",
	'{' => "chOpenCurly",
	'}' => "chCloseCurly",
	'<' => "chOpenAngle",
	'>' => "chCloseAngle"
	);

open(FH, "sort utf_constants.txt | uniq | tee utf_constants.new |");
open(OFH, ">XMLUnicodeConstants.h");
open(OFC, ">XMLUnicodeConstants.cc");

print OFH <<END;
#ifndef XMLUNICODECONSTANTS_H
#define XMLUNICODECONSTANTS_H

#include <xercesc/util/XMLUniDefs.hpp>

/* Diese Datei wurde automatisch von einem Script generiert.
 * Nicht bearbeiten - stattdessen die Datei utf_constants.txt
 * anpassen und danach "$0" aufrufen.
 */

/* Xerces C++ Namespace */
XERCES_CPP_NAMESPACE_BEGIN

END

print OFC <<END;
#include <xercesc/util/XMLUniDefs.hpp>

/* Diese Datei wurde automatisch von einem Script generiert.
 * Nicht bearbeiten - stattdessen die Datei utf_constants.txt
 * anpassen und danach "$0" aufrufen.
 */

/* Xerces C++ Namespace */
XERCES_CPP_NAMESPACE_BEGIN

END

LINE: while (<FH>) {
	chop;
	$line = $_;
	$line =~ s/^[ ]*(.*)[ ]*$/$1/;

	next LINE unless length $line;
	print "[$line]";

	if (exists $sym_map{$line}) {
		$utf_value = $sym_map{$line} . ', chNull';
		$utf_cname = $sym_map{$line};
		$utf_cname =~ s/^ch(.*)$/$1/;
		$utf_cname = 'utf_' . $utf_cname;
	} else {
		$utf_cname = $line;
		$utf_cname =~ s/[^a-zA-Z0-9]/_/g;
		$utf_cname =~ s/_+/_/g;
		$utf_cname = 'utf_' . $utf_cname;
	
		$utf_value = '';
		for ($i=0; $i<length $line; $i++) {
			$c = substr $line, $i, 1;
		
			if (exists $sym_map{$c}) {
				$utf_value .= $sym_map{$c} . ', ';
			} elsif ($c =~ /[a-zA-Z]/) {
				$utf_value .= 'chLatin_' . $c . ', ';
			} elsif ($c =~ /[0-9]/) {
				$utf_value .= 'chDigit_' . $c . ', ';
			} else {
				die "unbekanntes Zeichen '$c'";
			}
		}
		$utf_value .= 'chNull';
	}
	$utf_def = "XMLCh $utf_cname\[\] = {\n$utf_value\n};\n";
	print OFC wrap("", "\t", $utf_def) . "\n";
	print OFH "extern XMLCh $utf_cname\[\];\n";
}
close FH;

print OFC <<END;
XERCES_CPP_NAMESPACE_END

END

print OFH <<END;

XERCES_CPP_NAMESPACE_END

#endif

END

close OFC;
close OFH;

rename "utf_constants.new", "utf_constants.txt";
print "\n";

