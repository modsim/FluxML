#include <cmath>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstddef>
// vorläufig:
#include <string>
#include <sstream>
//#include <gmpxx.h>
#include "cstringtools.h"
#include "Combinations.h"

extern "C"
{
#include <stdint.h>
}

void strtrunc_inplace(char * str, size_t len)
{
	size_t i,j,l,r,slen = strlen(str);
	if (slen<len) return;
	if (len<3) { str[len]='\0'; return; }
	l = (len-2+1)/2;
	r = (len-2)/2;
	i = l;
	str[i] = '.';
	str[++i] = '.';
	for (j=slen-r; j<slen; j++)
		str[++i] = str[j];
	str[++i] = '\0';
}

void strtrim_inplace(char * str)
{
	char *c, *pp;
	if (str)
	{
		// rechte Seite
		for (c=pp=str; (*c)!='\0'; ++c)
			if (!isspace(*c)) pp=c+1;
		*pp='\0';
		// linke Seite
		for (c=str; isspace(*c); ++c)
			continue;
		// "+1": Terminator '\0' auch kopieren
		memmove(str,c,strlen(c)+1);
	}
}

void strLtrim_inplace(char * str)
{
	char *c;
	if (str)
	{
		for (c=str; isspace(*c); ++c)
			continue;
		// "+1": Terminator '\0' auch kopieren
		memmove(str,c,strlen(c)+1);
	}
}

void strRtrim_inplace(char * str)
{
	char *c, *pp;
	if (str)
	{
		for (c=pp=str; (*c)!='\0'; ++c)
			if (!isspace(*c)) pp=c+1;
		*pp='\0';
	}
}

char * strdup_alloc(char const * todup)
{
	if (todup == 0) return 0;
	char *ret = new char[strlen(todup)+1];
	return ret ? strcpy(ret, todup) : 0;
}

/* ungetestet; wird zur Zeit nicht benötigt!
void strcat_alloc(char ** tocat, char const * str)
{
	if (tocat == 0 || str == 0) return;
	size_t len_tocat = strlen(tocat);
	size_t len_str = strlen(str);
	if (len_str == 0) return;
	*tocat = realloc(*tocat, len_tocat+len_str+1);
	strcat(*tocat, str);
}
*/

int strcmp_ignorecase(char const * s1, char const * s2)
{
	while (*s1 && tolower(*s1)==tolower(*s2))
		s1++, s2++;
	return tolower(*(unsigned char *)s1)-tolower(*(unsigned char *)s2);
}

int strtoutf8(char const * nptr, char ** endptr, bool compress)
{
	int offset, bytes, lbyte, nb, b;
	int shift = 8, mask = 0xff;
	unsigned short bom[] = {0xef, 0xbb, 0xbf};

	/* ggfs. komprimierte Darstellung ohne leading bits */
	if (compress)
	{
		shift = 6;
		mask = 0x3f;
	}

	if (endptr)
		*endptr = (char*)nptr;
	/* 0-Pointer übergeben */
	if (nptr == 0)
		return -1;

	/* BOM überlesen */
	if ((unsigned short)(nptr[0])==bom[0]
		&& (unsigned short)(nptr[1]==bom[1])
		&& (unsigned short)(nptr[2]==bom[2]))
		offset = 3;
	else
		offset = 0;
	
	lbyte = *(nptr+offset) & 0xff;
	if ((lbyte >> 7) == 0x00) /* 1-byte-char */
		nb = 1;
	else if ((lbyte >> 5) == 0x06) /* 2-byte-char */
		nb = 2;
	else if ((lbyte >> 4) == 0x0e) /* 3-byte-char */
		nb = 3;
	else if ((lbyte >> 3) == 0x1e) /* 4-byte-char */
		nb = 4;
	else
		return -1;

	bytes = lbyte;
	for (b=1; b<nb; b++)
	{
		lbyte = *(nptr+offset+b) & 0xff;
		if ((lbyte >> 6) != 0x02) return -1;
		bytes = (bytes << shift) + (lbyte & mask);
	}
	
	if (endptr) *endptr = (char*)nptr+offset+nb;
	return bytes;
}

int dbl2str(char * s, double f, size_t len)
{
	char buf[32];
	if (len < 32) { if (len) *s = '\0'; return -1; }
	std::stringstream ss;
	ss << f;
	strcpy(s,ss.str().c_str());
	return strlen(s);
#if 0
	// libgmp-Krücke:
	bool neg = f<0.;
	if (neg) f = -f;
	mpf_class mpf(f);
	mp_exp_t e;
	std::string mps = mpf.get_str(e,10,len-(neg?1:0)-7);
	int E = e - 1;

	if (mps.size() == 0)
	{
		strcpy(s,"0");
		return 1;
	}

	std::stringstream ss;
	if (neg) ss << "-";
	ss << mps[0];
	if (mps.size() > 1)
		ss << "." << mps.substr(1) << "e" << E;
	ss >> mps;
	strcpy(s,mps.c_str());
	return mps.size();
#endif
}

char const * escape_chr(int ch)
{
	static char buf[32] = "";
	static const char ctrl[][4] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		 "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN",  "EM", "SUB", "ESC",  "FS",  "GS",  "RS",  "US"
		};

	if (ch < 0 or ch > 127)
	{
		if (ch < 0)
			snprintf(buf,32,"chr(0x%8x)",ch);
		else
		{
			unsigned int bits = log_base_2(next_pwr_2(ch));
			if (bits <= 8)
				snprintf(buf,32,"%%%02x",ch);
			else if (bits <= 16)
				snprintf(buf,32,"%%%04x",ch);
			else if (bits <= 24)
				snprintf(buf,32,"%%%06x",ch);
			else if (bits <= 32)
				snprintf(buf,32,"%%%08x",ch);
			else
				snprintf(buf,32,"%x",ch);
		}
	}
	else if (ch < 32)
		snprintf(buf,32,"<%s>",ctrl[ch]);
	else if (ch == 127)
		snprintf(buf,32,"<DEL>");
	else if (ch == '\\')
		snprintf(buf,32,"\\\\");
	else if (ch == '"')
		snprintf(buf,32,"\\\"");
	else if (ch == '\'')
		snprintf(buf,32,"\\'");
	else
		snprintf(buf,32,"%c", char(ch));
	return buf;
}

void formatdbl(char * buf, size_t blen, double d, char const * fmt)
{
#define FMT_SHORT	"%10.5f"
#define FMT_LONG	"%20.15f"
#define FMT_SHORT_E	"%10.5e"
#define FMT_LONG_E	"%20.15e"
#define FMT_SHORT_G	"%10.5g"
#define FMT_LONG_G	"%20.15g"

	int f;
	if (strcmp(fmt,"s")==0)
		f = 0;
	else if (strcmp(fmt,"l")==0)
		f = 1;
	else if (strcmp(fmt,"se")==0)
		f = 2;
	else if (strcmp(fmt,"le")==0)
		f = 3;
	else if (strcmp(fmt,"sg")==0)
		f = 4;
	else if (strcmp(fmt,"lg")==0)
		f = 5;
	else if (strcmp(fmt,"a")==0)
		f = 6;
	else
		f = 0;

	if (f==0)
	{
		snprintf(buf,blen,FMT_SHORT,d);
		if (strlen(buf) <= 10)
			return;
		f = 2;
	}

	if (f==1)
	{
		snprintf(buf,blen,FMT_LONG,d);
		if (strlen(buf) <= 20)
			return;
		f = 3;
	}

	if (f==2)
	{
		snprintf(buf,blen,FMT_SHORT_E,d);
		return;
	}

	if (f==3)
	{
		snprintf(buf,blen,FMT_LONG_E,d);
		return;
	}

	if (f==4)
	{
		snprintf(buf,blen,FMT_SHORT_G,d);
		return;
	}

	if (f==5)
	{
		snprintf(buf,blen,FMT_LONG_G,d);
		return;
	}

	if (f==6)
	{
		dbl2str(buf,d,blen);
		return;
	}
}

#define GETLINE_BUFSIZE	64
char * fgetline(FILE * stream, size_t max_len, size_t * lenp)
{
	int c;
	size_t len = 0;
	struct bchain_t
	{
		char * buf;
		struct bchain_t * next;
	} * head = 0, * entry, * newentry;

	while ((c = fgetc(stream)) != EOF)
	{
		if (len == max_len and max_len != 0)
			break;

		if (len % GETLINE_BUFSIZE == 0)
		{
			newentry = new bchain_t;
			newentry->buf = new char[GETLINE_BUFSIZE];
			newentry->next = 0;
			if (head == 0)
				head = newentry;
			else
				entry->next = newentry;
			entry = newentry;
		}

		entry->buf[len % GETLINE_BUFSIZE] = (char)c;
		len++;
		if (c == '\n')
			break;
	}
	if (lenp)
		*lenp = len;
	if (len == 0)
		return 0;

	char * str = new char[len+1];
	str[len] = '\0';
	c = 0;
	while (head)
	{
		entry = head;
		memcpy(str+(c*GETLINE_BUFSIZE), entry->buf,
			(len-c*GETLINE_BUFSIZE<GETLINE_BUFSIZE)
				?(len-c*GETLINE_BUFSIZE)
				:GETLINE_BUFSIZE);
		head = head->next;
		delete[] entry->buf;
		delete entry;
		c++;
	}
	return str;
}

// Der Horspool-Algorithmus (1980)
char const * strsearch(
	char const * y,
	int n,
	char const * x,
	int m
	)
{
#define ALPHABETSIZE	256
	int a, i, j, bm_bc[ALPHABETSIZE];
	char  ch, lastch;

	for (a=0; a<ALPHABETSIZE; ++a) bm_bc[a] = m;
	for (j=0; j<m-1; ++j) bm_bc[size_t(x[j])]=m-j-1;

	lastch=x[m-1];
	i=0;
	while (i<=n-m)
	{
		ch=y[i+m-1];
		if (ch==lastch)
			if (memcmp(&y[i],x,m-1) == 0)
				return &y[i];
		i += bm_bc[size_t(ch)];
	}
	return 0;
}
