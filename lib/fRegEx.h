#ifndef FLX_REGEX_H
#define FLX_REGEX_H

#include <cstdio>
#include <cstring>
extern "C"
{
#include <regex.h>
}

namespace flux {
namespace lib {

class RegExException { };

/**
 * C++-Wrapper für POSIX Regular Expressions.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class RegEx
{
private:
	int reg_err_;
	regex_t reg_preg_;
	regmatch_t *reg_regmatches_;
	char * workstr_;
	int regex_flags_;
	int reg_nummatches_;
	char ** matchptr_;
public:
	RegEx(char const * rpattern);
	RegEx(char const * rpattern, int nummatches);
	~RegEx();

public:
	void compile(char const * rpattern, int nummatches);
	char const ** match(char const * cstr=0);
	bool matches(char const * cstr) const;
	void setflags(int flags);

};

bool operator==(RegEx const & rexp, char const * cstr);
bool operator!=(RegEx const & rexp, char const * cstr);
bool operator==(char const * cstr, RegEx const & rexp);
bool operator!=(char const * cstr, RegEx const & rexp);

} // namespace flux::lib
} // namespace flux

#endif

