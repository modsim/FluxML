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
my $mml_xmlns_uri = 'http://www.w3.org/1998/Math/MathML';

sub toUTF($) {
	my $str = shift;
	my ($i,$c);
	my $utf_value = '';
	for ($i=0; $i<length $str; $i++) {
		$c = substr $str, $i, 1;

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
	return $utf_value . 'chNull'
}

open(FH, "sort mml_constants.txt | uniq | tee mml_constants.new |");
open(OFH, ">MathMLUnicodeConstants.h");
open(OFC, ">MathMLUnicodeConstants.cc");

print OFH <<END;
#ifndef MATHMLUNICODECONSTANTS_H
#define MATHMLUNICODECONSTANTS_H

#include <xercesc/util/XMLUniDefs.hpp>

/* Diese Datei wurde automatisch von einem Script generiert.
 * Nicht bearbeiten - stattdessen die Datei mml_constants.txt
 * anpassen und danach "$0" aufrufen.
 */

/* Xerces C++ Namespace */
XERCES_CPP_NAMESPACE_BEGIN

END

print OFC <<END;
#include <xercesc/util/XMLUniDefs.hpp>

/* Diese Datei wurde automatisch von einem Script generiert.
 * Nicht bearbeiten - stattdessen die Datei mml_constants.txt
 * anpassen und danach "$0" aufrufen.
 */

/* Xerces C++ Namespace */
XERCES_CPP_NAMESPACE_BEGIN

END

# xmlns URI von MathML
$utf_cname = 'mml_xmlns_uri';
$utf_def = "XMLCh $utf_cname\[\] = {\n" . toUTF($mml_xmlns_uri) . "\n};\n";
print OFC wrap("", "\t", $utf_def) . "\n";
print OFH "extern XMLCh $utf_cname\[\];\n";

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
		$utf_cname = 'mml_' . $utf_cname;
	} else {
		$utf_cname = $line;
		$utf_cname =~ s/[^a-zA-Z0-9]/_/g;
		$utf_cname =~ s/_+/_/g;
		$utf_cname = 'mml_' . $utf_cname;
	
		$utf_value = toUTF($line)
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

rename "mml_constants.new", "mml_constants.txt";
print "\n";

