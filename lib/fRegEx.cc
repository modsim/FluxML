#include "Error.h"
#include "fRegEx.h"

namespace flux {
namespace lib {

bool operator==(RegEx const & rexp, char const * cstr) 
{
	return rexp.matches(cstr);
}

bool operator!=(RegEx const & rexp, char const * cstr)
{
	return not rexp.matches(cstr);
}

bool operator==(char const * cstr, RegEx const & rexp)
{
	return rexp.matches(cstr);
}

bool operator!=(char const * cstr, RegEx const & rexp)
{
	return not rexp.matches(cstr);
}

RegEx::RegEx(char const * rpattern)
{
	workstr_ = 0;
	matchptr_ = 0;
	reg_regmatches_ = 0;
	regex_flags_ = REG_EXTENDED;
	compile(rpattern, 0);
}

RegEx::RegEx(char const * rpattern, int nummatches)
{
	workstr_ = 0;
	matchptr_ = 0;
	reg_regmatches_ = 0;
	regex_flags_ = REG_EXTENDED;
	compile(rpattern, nummatches);
}

RegEx::~RegEx()
{
	int i;

	regfree(&reg_preg_);
	if (reg_regmatches_) delete [] reg_regmatches_;
	if (matchptr_)
	{
		for (i=0; matchptr_[i]!=0; i++)
			delete[] matchptr_[i];
		delete[] matchptr_;
	}
}

void RegEx::compile(char const * rpattern, int nummatches)
{
	reg_nummatches_ = nummatches+1;

	if (reg_regmatches_)
		delete [] reg_regmatches_;

	reg_regmatches_ = new regmatch_t[reg_nummatches_];

	reg_err_ = regcomp(&reg_preg_, rpattern, regex_flags_);
	if (reg_err_!=0)
	{
		char reg_err_buf[256];
		regfree(&reg_preg_);
		regerror(reg_err_, &reg_preg_, reg_err_buf, sizeof(reg_err_buf));
		fERROR("compilation of regex \"%s\": %s", rpattern, reg_err_buf);
		fTHROW(RegExException);
	}
}

char const ** RegEx::match(char const * cstr)
{
	int i;

	if (matchptr_)
	{
		for (i=0; matchptr_[i]!=0; i++)
			delete[] matchptr_[i];
		delete[] matchptr_;
		matchptr_ = 0;
	}

	if (cstr) workstr_ = const_cast< char * >(cstr);

	if (workstr_)
	{
		reg_err_ = regexec(&reg_preg_, workstr_, reg_nummatches_, reg_regmatches_, 0);
		if (reg_err_ == REG_NOMATCH)
		{
			//char reg_err_buf[256];
			//regerror(reg_err_, &reg_preg_, reg_err_buf, sizeof(reg_err_buf));
			//fWARNING(reg_err_buf);
			return 0;
		}
		fASSERT(reg_err_ == 0);

		matchptr_ = new char * [reg_nummatches_+1];
		for (i=0; i<reg_nummatches_ and reg_regmatches_[i].rm_so!=-1; ++i)
		{
			matchptr_[i] = new char[reg_regmatches_[i].rm_eo - reg_regmatches_[i].rm_so + 1];
			matchptr_[i][0] = '\0';
			strncpy(matchptr_[i],
				workstr_ + reg_regmatches_[i].rm_so,
				reg_regmatches_[i].rm_eo - reg_regmatches_[i].rm_so);
			matchptr_[i][reg_regmatches_[i].rm_eo - reg_regmatches_[i].rm_so] = '\0';
		}
		matchptr_[i] = 0;
	}
	else
		return 0;

	return (char const **)matchptr_;
}

bool RegEx::matches(char const * cstr) const
{
	return bool(!regexec(&reg_preg_, cstr, (size_t)0, (regmatch_t*)0, 0));
}

void RegEx::setflags(int flags)
{
	regex_flags_ = flags;
}

} // namespace flux::lib
} // namespace flux

#ifdef DEBUGREGEXP
int main()
{
	char **erg = 0;
	char **w;

	try
	{
		flux::lib::RegEx reg("Wir ([0-9]+) vom ([abcdr]+) Deich", 2);
		//reg.compile("Wir ([0-9]+) vom ([abcdr]+) Deich", 2);
		erg = reg.match("Wir 15 vom abracadabra Deich");
		if (erg)
		{
			w = erg;
			while (*w)
			{
				printf("[%s]\n", *w);
				w++;
			}
		}
		erg = reg.match("Wir 13 vom rabarba Deich");
		if (erg)
		{
			w = erg;
			while (*w)
			{
				printf("[%s]\n", *w);
				w++;
			}
		}

		if (reg == "Wir 19 vom baccdr Deich" && !(reg!= "Wir 19 vom baccdr Deich"))
		{
			printf ("Match!\n");
		}

	}
	catch (flux::lib::RegExException & e)
	{
		fprintf(stderr, "Exception: %s\n", e.errstr());
	}
}
#endif

