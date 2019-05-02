/*
 * fhash_map / C++ universal template-based hash data structure
 */

#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include "charptr_array.h"

charptr_array::charptr_array()
	: list_len_(0), head_(0), tail_(0), charptrptr_(0),
	  concatptr_(0) { }

charptr_array::charptr_array(charptr_array const & copy)
	: list_len_(0), head_(0), tail_(0), charptrptr_(0),
	  concatptr_(0)
{
	list_T * walk = copy.head_;
	while (walk)
	{
		add(walk->str);
		walk = walk->next;
	}
}

charptr_array::charptr_array(char const ** ptrptr)
	: list_len_(0), head_(0), tail_(0), charptrptr_(0),
	  concatptr_(0)
{
	if (ptrptr == 0)
		return;
	char ** walk = (char **)ptrptr;
	while (*walk)
	{
		add(ptrptr[list_len_]);
		walk++;
	}
}

charptr_array::~charptr_array()
{
	clear();
	if (charptrptr_ != 0) delete[] charptrptr_;
	if (concatptr_ != 0) delete[] concatptr_;
}

void charptr_array::clear()
{
	list_T * prev, * walk = head_;
	while (walk)
	{
		delete[] walk->str; prev = walk;
		walk = walk->next; delete prev;
	}
	head_ = tail_ = 0;
	list_len_ = 0;
}

void charptr_array::qsort(int lo, int hi)
{
	int i,j;
	char * tmp;
	
	if (hi>lo)
	{
		i = lo-1; j = hi;
		do
		{
			do i++; while (strcmp(charptrptr_[i],charptrptr_[hi])<0);
			do j--; while (strcmp(charptrptr_[j],charptrptr_[hi])>0 && j);
			tmp = charptrptr_[i];
			charptrptr_[i] = charptrptr_[j];
			charptrptr_[j] = tmp;
		}
		while (j>i);
		charptrptr_[j] = charptrptr_[i];
		charptrptr_[i] = charptrptr_[hi];
		charptrptr_[hi] = tmp;
		qsort(lo, i-1);	qsort(i+1, hi);
	}
}

/*
// einfaches / altes add():
charptr_array::const_iterator charptr_array::add(char const * p)
{
	if (!p) return end();
	list_len_++;
	if (head_)
		tail_ = tail_->next = new list_T;
	else
		tail_ = head_ = new list_T;
	tail_->str = new char[strlen(p)+1];
	strcpy((char*)tail_->str,p);
	tail_->next = 0;
	return const_iterator(this,tail_);
}
*/

charptr_array::const_iterator charptr_array::add(charptr_array const & P)
{
	const_iterator i = P.begin();
	const_iterator j = end();
	if (i != P.end()) { j = add(*i); i++; }
	while (i != P.end()) { add(*i); i++; }
	return j;
}

charptr_array::const_iterator charptr_array::addFront(char const * p)
{
	if (!head_) return add(p);
	if (!p) return end();
	list_len_++;
	list_T * new_E = new list_T;
	new_E->next = head_;
	new_E->str = new char[strlen(p)+1];
	strcpy((char*)new_E->str,p);
	head_ = new_E;
	return const_iterator(this,head_);
}

charptr_array::const_iterator charptr_array::add(char const * fmt, ...)
{
	int len;
	va_list ap;

	if (!fmt) return end();
	list_len_++;
	if (head_)
		tail_ = tail_->next = new list_T;
	else
		tail_ = head_ = new list_T;

	va_start(ap,fmt);
	len = vsnprintf(0,0,fmt,ap);
	va_end(ap);

	char * str = new char[len+1];
	memset(str,0,len+1);
	
	va_start(ap,fmt);
	vsnprintf(str,len+1,fmt,ap);
	va_end(ap);

	tail_->str = str;
	tail_->next = 0;
	return const_iterator(this,tail_);
}

charptr_array & charptr_array::operator= (charptr_array const & rval)
{
	// catch use of the comma operator:
	if (&rval == this) return *this;
	clear();
	add(rval);
	return *this;
}

char const * charptr_array::operator[] (size_t idx) const
{
	if (idx>list_len_) return 0;
	for (list_T * walk = head_;
		walk;
		walk = walk->next, idx--)
		if (idx == 0) return walk->str;
	return 0;
}

bool charptr_array::exists(char const * p) const
{
	for (list_T * walk = head_; walk; walk = walk->next)
		if (strcmp(p,walk->str) == 0) return true;
	return false;
}

charptr_array::const_iterator charptr_array::find(char const * p) const
{
	list_T * walk;
	for (walk = head_; walk != 0; walk = walk->next)
		if (strcmp(p,walk->str) == 0)
			return const_iterator(this,walk);
	return const_iterator();
}

charptr_array::const_iterator charptr_array::insert(
	char const * p,
	const_iterator & i
	)
{
	if (!p) return end();
	if (i.pobj_!=this || !head_) return add(p);
	list_len_++;
	list_T * new_E = new list_T;
	new_E->str = new char[strlen(p)+1];
	strcpy((char*)new_E->str,p);
	new_E->next = i.pos_->next;
	i.pos_->next = new_E;
	return const_iterator(this,new_E);
}

int charptr_array::findIndex(char const * p) const
{
	int idx;
	list_T * walk;
	for (idx = 0, walk = head_; walk != 0; idx++, walk = walk->next)
		if (strcmp(p,walk->str) == 0) return idx;
	return -1;
}

charptr_array::const_iterator charptr_array::addUnique(charptr_array const & P)
{
	list_T * walk;
	const_iterator first = end();
	for (walk=P.head_; walk!=0; walk=walk->next)
	{
		if (first == end())
			first = addUnique(walk->str);
		else
			addUnique(walk->str);
	}
	return first;
}

char const ** charptr_array::get() const
{
	size_t i = 0;
	list_T * walk = head_;
	if (charptrptr_)
		delete[] charptrptr_;
	charptrptr_ = new char*[list_len_+1];
	while (walk)
	{
		charptrptr_[i++] = (char *)walk->str;
		walk = walk->next;
	}
	charptrptr_[i] = 0;
	return (char const **)charptrptr_;
}

char const * charptr_array::concat(char const * glue) const
{
	size_t len = 0;
	size_t gluelen = glue?strlen(glue):0;
	list_T * walk = head_;
	char * endptr;

	if (concatptr_)
		delete[] concatptr_;
	if (list_len_ == 0)
	{
		concatptr_ = new char[1];
		concatptr_[0] = '\0';
		return concatptr_;
	}
	while (walk)
	{
		len += strlen(walk->str)+gluelen;
		walk = walk->next;
	}
	len -= gluelen;
	concatptr_ = new char[len+1];
	memset(concatptr_,0,len+1); // concatptr_[len] = '\0';
	endptr = concatptr_;
	walk = head_;
	while (walk)
	{
		len = strlen(walk->str);
		memcpy(endptr,walk->str,len);
		endptr += len;
		walk = walk->next;
		if (walk && gluelen)
		{
			memcpy(endptr,glue,gluelen);
			endptr += gluelen;
		}
	}
	return concatptr_;
}

void charptr_array::sort()
{
	char ** w;
	list_T * prev, * walk = head_;

	if (list_len_ == 0)
		return;
	
	w = (char**)get();
	
	while (walk)
	{
		prev = walk; walk = walk->next;
		delete prev;
	}
	qsort(0,list_len_-1);
	if (*w)
	{
		tail_ = head_ = new list_T;
		tail_->str = *(w++);
	}	
	while (*w)
	{
		tail_ = tail_->next = new list_T;
		tail_->str = *(w++);
	}
	tail_->next = 0;
}

charptr_array charptr_array::split(char const * s, char const * delims)
{
	charptr_array result;
	char const * p = s;
	char * buf = 0;
	size_t bsize = 0, b = 0;
	
	do
	{
		b = strspn(p,delims);
		p += b;
		b = strcspn(p,delims);
		
		if (b == 0)
			break;

		if (bsize < b+1)
		{
			bsize = b+1;
			buf = (char*)realloc(buf, bsize);
		}
		
		strncpy(buf,p,b);
		buf[b] = '\0';
		
		result.add(buf);
		p += b;
	}
	while (*p != '\0');
	free(buf);
	return result;
}

charptr_array charptr_array::splitn(char const * s, size_t n)
{
	charptr_array result;
	char * buf = new char[n+1];
	char const * p = s;

	buf[n] = '\0';
	while (*p)
	{
		strncpy(buf,p,n);
		result.add(buf);
		p += strlen(buf);
	}
	delete[] buf;
	return result;
}

