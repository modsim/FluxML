#ifndef BITARRAY_IMPL_H
#define BITARRAY_IMPL_H

#include "Error.h"
#include <cstring> // strlen

inline BitArray::BitArray()
	: length_(1), blength_(0), bstr_(0)
{
	barray_ = new uint64_t[length_];
	zeros();
}
	
inline BitArray::BitArray(size_t blength)
	: length_((blength>>6)+1), blength_(blength), bstr_(0)
{
	barray_ = new uint64_t[length_];
	zeros();
}

inline BitArray::BitArray(BitArray const & copy)
	: length_((copy.blength_>>6)+1), blength_(copy.blength_), bstr_(0)
{
	size_t i;
	barray_ = new uint64_t[length_];
	for (i=0; i<length_; ++i)
		barray_[i] = copy.barray_[i];
}

inline BitArray::BitArray(BitProxy const & slice)
	: length_(1), blength_(0), bstr_(0)
{
	barray_ = new uint64_t[1];
	BitArray S(slice.j_-slice.i_+1);
	S(0,slice.j_-slice.i_) = slice;
	operator=(S);
}


inline BitArray::~BitArray()
{
	delete[] bstr_;
	delete[] barray_;
}

inline BitArray & BitArray::operator= (BitArray const & copy)
{
	size_t i;
	delete[] barray_;
	if (bstr_) { delete[] bstr_; bstr_ = 0; }
	blength_ = copy.blength_;
	length_ = (copy.blength_>>6)+1;
	barray_ = new uint64_t[length_];
	for (i=0; i<length_; ++i)
		barray_[i] = copy.barray_[i];
	return *this;
}

inline BitArray & BitArray::operator= (uint64_t ival)
{
	uint64_t pwr2 = 1, tmp;
	unsigned int log2 = 0;

	while (pwr2<ival) pwr2<<=1;
	tmp = ival;
	while (tmp>1) { tmp>>=1; log2++; }
	
	delete[] barray_;
	blength_ = log2+1;
	blength_++; // extra-0-bit für Vorzeichen
	length_ = 1; //(blength_>>6)+1;
	barray_ = new uint64_t[length_];
	*barray_ = ival;
	return *this;
}

inline bool BitArray::get(size_t idx) const
{
	if (idx >= blength_) return false;
	return bool((barray_[idx>>6] >> (idx & 63)) & 1);
}

inline BitArray::BitProxy BitArray::operator[] (size_t idx)
{
	return BitProxy(this,idx);
}

inline BitArray::BitProxy BitArray::operator() (size_t i, size_t j)
{
	return BitProxy(this,i,j);
}

inline BitArray & BitArray::operator=(BitProxy const & slice)
{
	BitArray S(slice.j_-slice.i_+1);
	S(0,slice.j_-slice.i_) = slice;
	return operator=(S);
}

inline void BitArray::set(size_t idx, bool val)
{
	if (idx >= blength_) { fASSERT_NONREACHABLE(); return; }

	if (val)
		barray_[idx>>6] |= (1ull<<(idx & 63));
	else
		barray_[idx>>6] &= ~(1ull << (idx & 63));
}

inline void BitArray::set(size_t offset, BitArray const & ba)
{
	if (size() < offset+ba.size())
		resize(offset+ba.size(),false);
	for (size_t i=0; i<ba.size(); i++)
		set(offset+i,ba.get(i));
}

inline bool BitArray::allZeros() const
{
	size_t i;
	for (i=0; i<(blength_>>6); i++)
		if (barray_[i]!=0) return false;
	for (i=((blength_>>6)<<6); i<blength_; i++)
		if (get(i)) return false;
	return true;
}

inline bool BitArray::allZeros(size_t i, size_t j) const
{
	size_t k;
	if (blength_ == 0)
		return true;
	if (j>=blength_) j=blength_-1;
	for (k=i; k<=j; k++)
		if (get(k)) return false;
	return true;
}

inline bool BitArray::allOnes() const
{
	size_t i;
	for (i=0; i<(blength_>>6); i++)
		if (barray_[i]!=0xffffffffffffffffull) return false;
	for (i=((blength_>>6)<<6); i<blength_; i++)
		if (not get(i)) return false;
	return true;
}

inline bool BitArray::allOnes(size_t i, size_t j) const
{
	size_t k;
	if (blength_ == 0)
		return true;
	if (j>=blength_) j=blength_-1;
	for (k=i; k<=j; k++)
		if (not get(k)) return false;
	return true;
}

inline bool BitArray::anyZeros() const
{
	return not allOnes();
}

inline bool BitArray::anyZeros(size_t i, size_t j) const
{
	return not allOnes(i,j);
}

inline bool BitArray::anyOnes() const
{
	return not allZeros();
}

inline bool BitArray::anyOnes(size_t i, size_t j) const
{
	return not allZeros(i,j);
}

inline void BitArray::zeros()
{
	size_t i;
	for (i=0; i<length_; i++)
		barray_[i]=0;
}

inline void BitArray::zeros(size_t i, size_t j)
{
	size_t k;
	if (blength_ == 0) return;
	if (j>=blength_) j = blength_-1;
	for (k=i; k<=j; k++)
		barray_[k>>6] &= ~(1ull<<(k&63));
}

inline void BitArray::ones()
{
	size_t i;
	for (i=0; i<length_; i++)
		barray_[i]=0xffffffffffffffffull;
}

inline void BitArray::ones(size_t i, size_t j)
{
	size_t k;
	if (blength_ == 0) return;
	if (j>=blength_) j = blength_-1;

	for (k=i; k<=j; k++)
		barray_[k>>6] |= (1<<(k&63));
}

inline size_t BitArray::countOnes() const
{
	size_t i,n;
	for (i=0,n=0; i<blength_; i++)
		if (get(i)) n++;
	return n;
}

inline size_t BitArray::countZeros() const
{
	return blength_-countOnes();
}

inline int BitArray::highestBit() const
{
	int i;
	for (i=blength_-1; i>=0; i--)
		if (get(i)) break;
	return i;
}

inline int BitArray::lowestBit() const
{
	size_t i;
	for (i=0; i<blength_; i++)
		if (get(i)) break;
	return i;
}

inline char const * BitArray::toString(char zero, char one) const
{
	size_t i;
	delete[] bstr_;
	bstr_ = new char[blength_+1];
	for (i=0; i<blength_; i++)
		bstr_[i] = get(i) ? one : zero;
	bstr_[blength_] = '\0';
	return bstr_;
}

inline char const * BitArray::toHexString(bool ucase) const
{
	char const hex[] = "0123456789abcdef";
	char const HEX[] = "0123456789ABCDEF";
	char const * h = ucase ? HEX : hex;
	int q;
	size_t i,j,k;
	delete[] bstr_;
	if (blength_ == 0)
	{
		bstr_ = new char[1];
		bstr_[0] = '\0';
		return bstr_;
	}
	bstr_ = new char[((blength_+4)>>2)+1];
	for (i=j=k=0,q=0; i<blength_; i++,j++)
	{
		if (j == 4)
		{
			bstr_[k++] = h[q];
			q = 0;
			j = 0;
		}
		if (get(i)) q |= (1<<j);
	}
	if (q != 0)
		bstr_[k++] = h[q];
	bstr_[k] = '\0';
	return bstr_;
}

inline char const * BitArray::toStringRev(char zero, char one) const
{
	size_t i;
	delete[] bstr_;
	bstr_ = new char[blength_+1];
	for (i=0; i<blength_; i++)
		bstr_[blength_-i-1] = get(i) ? one : zero;
	bstr_[blength_] = '\0';
	return bstr_;
}

inline char const * BitArray::toHexStringRev(bool ucase) const
{
	size_t i,len = strlen(toHexString(ucase));
	char t;
	for (i=0; i<(len+1)/2; i++)
	{
		t = bstr_[i];
		bstr_[i] = bstr_[len-1-i];
		bstr_[len-1-i] = t;
	}
	return bstr_;
}

inline BitArray * BitArray::parseHex(char const * h)
{
	size_t len = 4*strlen(h)+1;
	char const * w = h;
	char c;
	int q, i;
	size_t offset = 0;
	BitArray * res = new BitArray(len);

	while (*w != '\0')
	{
		c = *w;
		if ((c >= '0') and (c <= '9'))
			q = c - '0';
		else if ((c >= 'A') and (c <= 'F'))
			q = c - 'A' + 10;
		else if ((c >= 'a') and (c <= 'f'))
			q = c - 'a' + 10;
		else
		{
			// Fehler
			delete res;
			return 0;
		}

		for (i=0; i<4; i++)
			if ((q>>i)&1) res->set(offset+i);
		offset += 4;
		w++;
	}
	return res;
}

inline BitArray * BitArray::parseBin(char const * b)
{
	size_t i,len = strlen(b);
	if (len == 0)
		return 0;
	BitArray * res = new BitArray(len+1);
	for (i=0; b[i]!='\0'; i++)
		if (b[i]=='1') res->set(i);
	return res;
}

inline BitArray & BitArray::incMask(BitArray const & mask)
{
	size_t i, len = blength_;
	if (mask.blength_ < len)
		len = mask.blength_;
	for (i=0; i<len; i++)
	{
		if (not mask.get(i)) // springe inaktive bits
			continue;
		if (not get(i))
		{
			set(i, true);
			break;
		}
		set(i, false);
	}
	return *this;
}

inline BitArray & BitArray::operator++()
{
	size_t i;
	for (i=0; i<blength_; i++)
	{
		if (not get(i))
		{
			set(i, true);
			break;
		}
		set(i, false);
	}
	return *this;
}

inline BitArray BitArray::operator++(int)
{
	BitArray P(*this);
	operator++();
	return P;
}

inline BitArray & BitArray::operator--()
{
	size_t i;
	for (i=0; i<blength_; i++)
	{
		if (get(i))
		{
			set(i, false);
			break;
		}
		set(i, true);
	}
	return *this;
}

inline BitArray BitArray::operator--(int)
{
	BitArray P(*this);
	operator--();
	return P;
}

inline bool BitArray::operator==(BitArray const & rval) const
{
	size_t i;
	for (i=0; i<(blength_>>6); i++)
		if (barray_[i] != rval.barray_[i]) return false;
	for (i=((blength_>>6)<<6); i<blength_; i++)
		if (get(i) != rval.get(i)) return false;
	return true;
}

inline bool BitArray::operator<(BitArray const & rval) const
{
	size_t maxlen = 1 + (blength_>rval.blength_?blength_:rval.blength_);
	BitArray L(*this), R(rval);

	// resize MIT sign-extend
	L.resize(maxlen,true);
	R.resize(maxlen,true);

	BitArray A = L-R;
	if (A.get(A.size()-1))
		return true;
	return false;
}

inline bool BitArray::operator<=(BitArray const & rval) const
{
	size_t maxlen = 1 + (blength_>rval.blength_?blength_:rval.blength_);
	BitArray L(*this), R(rval);

	// resize mit sign-extend
	L.resize(maxlen,true);
	R.resize(maxlen,true);

	BitArray A = L-R;
	if (A.get(A.size()-1) or A.allZeros())
		return true;
	return false;
}

inline BitArray & BitArray::operator+=(BitArray const & rval)
{
	if (blength_ == 0)
		return (*this = rval);
	if (rval.blength_ == 0)
		return *this;
	
	size_t len = (blength_>rval.blength_?blength_:rval.blength_);
	size_t mlen = (blength_<rval.blength_?blength_:rval.blength_);
	size_t k;
	bool a,b,c = false;
	resize(len,true);

	k = 0;
	for (k=0; k<mlen; k++)
	{
		a = get(k);
		b = rval.get(k);
		set(k, (a != b) != c);
		c = (a and (c or b)) or (b and c);
	}

	for (; k<blength_; k++)
	{
		a = get(k);
		set(k, a != c);
		c = a && c;
	}

	for (; k<rval.blength_; k++)
	{
		b = rval.get(k);
		set(k, b != c);
		c = b && c;
	}
	return *this;
}

inline BitArray & BitArray::operator-=(BitArray const & rval)
{
	if (blength_ == 0)
		return (*this = rval);
	if (rval.blength_ == 0)
		return *this;
	
	size_t len = (blength_>rval.blength_?blength_:rval.blength_);
	size_t mlen = (blength_<rval.blength_?blength_:rval.blength_);
	size_t k;
	bool a,b,c = false;
	resize(len,true);

	k = 0;
	for (k=0; k<mlen; k++)
	{
		a = get(k);
		b = rval.get(k);
		set(k, a == (b == c));
		c = (b and c) or (not a and (b or c));
	}

	for (; k<blength_; k++)
	{
		a = get(k);
		set(k, a xor c);
		c = not a and c;
	}

	for (; k<rval.blength_; k++)
	{
		b = rval.get(k);
		set(k, b xor c);
		c = c or b;
	}
	return *this;
}

inline BitArray & BitArray::operator|= (BitArray const & rval)
{
	if (blength_ == 0)
		return (*this=rval);
	if (rval.blength_ == 0)
		return *this;
	
	size_t i, len = blength_>rval.blength_?blength_:rval.blength_;
	size_t mlen = blength_<rval.blength_?blength_:rval.blength_;

	len = (len>>6)+1;
	mlen = (mlen>>6)+1;

	for (i=0; i<mlen; i++)
		barray_[i] |= rval.barray_[i];

	len = blength_>rval.blength_?blength_:rval.blength_;
	mlen = blength_<rval.blength_?blength_:rval.blength_;
	if (blength_<rval.blength_)
	{
		resize(rval.blength_,false);
		for (i=mlen; i<len; i++)
			set(i,rval.get(i));
	}

	return *this;
}

inline BitArray & BitArray::operator&= (BitArray const & rval)
{
	if (blength_ == 0)
		return (*this=rval);
	if (rval.blength_ == 0)
		return *this;
	
	size_t i, len = blength_>rval.blength_?blength_:rval.blength_;
	size_t mlen = blength_<rval.blength_?blength_:rval.blength_;

	len = (len>>6)+1;
	mlen = (mlen>>6)+1;

	for (i=0; i<mlen; i++)
		barray_[i] &= rval.barray_[i];

	len = blength_>rval.blength_?blength_:rval.blength_;
	mlen = blength_<rval.blength_?blength_:rval.blength_;
	if (blength_<rval.blength_)
	{
		resize(rval.blength_,false);
		for (i=mlen; i<len; i++)
			clear(i);
	}

	return *this;
}

inline BitArray & BitArray::operator^= (BitArray const & rval)
{
	if (blength_ == 0)
		return (*this=rval);
	if (rval.blength_ == 0)
		return *this;
	
	size_t i, len = blength_>rval.blength_?blength_:rval.blength_;
	size_t mlen = blength_<rval.blength_?blength_:rval.blength_;

	len = (len>>6)+1;
	mlen = (mlen>>6)+1;

	for (i=0; i<mlen; i++)
		barray_[i] ^= rval.barray_[i];

	len = blength_>rval.blength_?blength_:rval.blength_;
	mlen = blength_<rval.blength_?blength_:rval.blength_;
	if (blength_<rval.blength_)
	{
		resize(rval.blength_,false);
		for (i=mlen; i<len; i++)
			set(i,rval.get(i));
	}

	return *this;
}

inline BitArray & BitArray::operator>>= (size_t shr)
{
	size_t i;
	size_t rshr = shr % 64;
	size_t dshr = shr / 64;
	uint64_t sh = 0ull, nextsh;

	if (dshr)
	{
		for (i=dshr; i<length_; i++)
			barray_[i-dshr] = barray_[i];
		for (i=length_-dshr; i<length_; i++)
			barray_[i] = 0ull;
	}
	
	for (i=length_,sh=0ull; i>0; i--)
	{
		nextsh = barray_[i-1];
		barray_[i-1] >>= rshr;
		barray_[i-1] |= (sh << (64-rshr));
		sh = nextsh;
	}
	barray_[length_-1] &= ((uint64_t)(-1) >> rshr);
	
	return *this;
}

/*
inline BitArray & BitArray::operator<<= (size_t shl) // nicht ok
{
	size_t i;
	size_t rshl = shl % 64;
	size_t dshl = shl / 64;
	uint64_t sh = 0ull, nextsh;

	if (dshl)
	{
		for (i=length_-dshl; i>0 ; i--)
			barray_[i+dshl-1] = barray_[i-1];
		for (i=0; i<dshl; i++)
			barray_[i] = 0ull;
	}
	
	for (i=0,sh=0ull; i<length_; i++)
	{
		nextsh = barray_[i];
		barray_[i] <<= rshl;
		barray_[i] |= (sh >> (64-rshl));
		sh = nextsh;
	}
	barray_[0] &= ((uint64_t)(-1) << rshl);
	barray_[length_-1] &= ((uint64_t)(-1) >> (62-rshl));
	return *this;
}
*/

inline uint64_t BitArray::toUnsignedInt64() const
{
	size_t i;
	uint64_t ival, mask;
	for (i=0,mask=1,ival=0; i<blength_; i++,(mask<<=1))
		if (get(i)) ival |= mask;
	return ival;
}

inline void BitArray::resize(size_t nblength, bool sign_extend)
{
	if (nblength > blength_)
	{
		BitArray R(nblength);
		size_t i;
		size_t mlen = blength_ < nblength ? blength_ : nblength;

		for (i=0; i<(mlen>>6); i++)
			R.barray_[i] = barray_[i];
		for (i=((mlen>>6)<<6); i<mlen; i++)
			if (get(i)) R.set(i);

		// sign extend bei Vergrößerung:
		if (sign_extend and nblength > blength_ and get(blength_-1))
			for (i=blength_-1; i<nblength; i++)
				R.set(i);

		uint64_t * t = R.barray_;
		R.barray_ = barray_;
		barray_ = t;

		blength_ = nblength;
		length_ = R.length_;
	}
	else if (nblength < blength_)
	{
		blength_ = nblength;
	}

	if (bstr_) { delete[] bstr_; bstr_ = 0; }
}

inline size_t BitArray::hashValue() const
{
	size_t h = 0;
	size_t i,j;
	char byte;

	for (i=0; i<(blength_>>6); i++)
	{
		char const * bytes =
			reinterpret_cast< char const* >(&(barray_[i]));
		for (j=0; j<sizeof(uint64_t); j++)
		{
			// R5-Hash
			h += (bytes[j]<<4);
			h += (bytes[j]>>4);
			h *= 11;

			//// CRC 16
			//h = (h >> 8) | (h << 8);
			//h ^= bytes[j];
			//h ^= ((h & 0xff) >> 4);
			//h ^= ((h << 8) << 4);
			//h ^= (((h & 0xff) << 4) << 1);
		}
	}
	
	i = ((blength_>>6)<<6);
	while (i<blength_)
	{
		byte = 0;
		j = i;
		while (j<blength_ and j-i<8)
		{
			byte <<= 1;
			if (get(j)) byte |= 1;
			j++;
		}

		// R5-Hash
		h += (byte<<4);
		h += (byte>>4);
		h *= 11;
		
		//// CRC16
		//h = (h >> 8) | (h << 8);
		//h ^= byte;
		//h ^= ((h & 0xff) >> 4);
		//h ^= ((h << 8) << 4);
		//h ^= (((h & 0xff) << 4) << 1);

		i = j;
	}
	return h;
}

inline BitArray operator+ (BitArray const & lval, BitArray const & rval)
{
	if (lval.blength_ == 0)
		return rval;
	if (rval.blength_ == 0)
		return lval;
	
	size_t len = (lval.blength_>rval.blength_?lval.blength_:rval.blength_);
	size_t mlen = (lval.blength_<rval.blength_?lval.blength_:rval.blength_);
	size_t k;
	bool a,b,c = false;
	BitArray S(len);

	k = 0;
	for (k=0; k<mlen; k++)
	{
		a = lval.get(k);
		b = rval.get(k);
		S.set(k, (a != b) != c);
		c = (a and (c or b)) or (b and c);
	}

	for (; k<lval.blength_; k++)
	{
		a = lval.get(k);
		S.set(k, a != c);
		c = a && c;
	}

	for (; k<rval.blength_; k++)
	{
		b = rval.get(k);
		S.set(k, b != c);
		c = b && c;
	}
	return S;
}

inline BitArray operator- (BitArray const & lval, BitArray const & rval)
{
	if (lval.blength_ == 0)
		return -rval;
	if (rval.blength_ == 0)
		return lval;

	return lval+(-rval);
}

inline BitArray operator- (BitArray const & lval)
{
	BitArray mlval = ~lval;
	return ++mlval;
}

inline BitArray operator* (BitArray const & lval, BitArray const & rval)
{
	if (lval.blength_ == 0 or rval.blength_ == 0)
		return BitArray();

	int i, lb, rb;
	bool last, bit;

	for (lb=lval.blength_-1; lb>=0; lb--)
		if (lval.get(lb)) break;
	lb++;
	for (rb=rval.blength_-1; rb>=0; rb--)
		if (rval.get(rb)) break;
	rb++;
	// -1 in lval erkennen
	if (lb==int(lval.blength_) and lval.allOnes())
		lb = 1;
	if (lb == 0 or rb == 0)
		return BitArray();
	BitArray L(lval);
	BitArray LR(lb+rb+1);
	L.resize(lb+rb+1,true);

	for (i=0,last=false; i<lb+rb; i++)
	{
		bit = rval.get(i);
		if (bit != last)
		{
			LR = bit ? LR - L : LR + L;
			last = bit;
		}
		L = L << 1;
	}
	return LR;
}

/*
inline BitArray operator/ (BitArray const & lval, BitArray const & rval)
{
}
*/

inline BitArray operator| (BitArray const & lval, BitArray const & rval)
{
	if (lval.blength_ == 0)
		return rval;
	if (rval.blength_ == 0)
		return lval;
	
	size_t i, len = lval.blength_>rval.blength_?lval.blength_:rval.blength_;
	size_t mlen = lval.blength_<rval.blength_?lval.blength_:rval.blength_;
	BitArray OR(len);

	len = (len>>6)+1;
	mlen = (mlen>>6)+1;

	for (i=0; i<mlen; i++)
		OR.barray_[i] = lval.barray_[i] | rval.barray_[i];

	len = lval.blength_>rval.blength_?lval.blength_:rval.blength_;
	mlen = lval.blength_<rval.blength_?lval.blength_:rval.blength_;
	if (lval.blength_>rval.blength_)
	{
		for (i=mlen; i<len; i++)
			OR.set(i,lval.get(i));
	}
	else
	{
		for (i=mlen; i<len; i++)
			OR.set(i,rval.get(i));
	}

	return OR;
}

inline BitArray operator& (BitArray const & lval, BitArray const & rval)
{
	if (lval.blength_ == 0)
		return BitArray(rval.blength_);
	if (rval.blength_ == 0)
		return BitArray(lval.blength_);
	
	size_t i, len = lval.blength_>rval.blength_?lval.blength_:rval.blength_;
	size_t mlen = lval.blength_<rval.blength_?lval.blength_:rval.blength_;
	BitArray AND(len);

	len = (len>>6)+1;
	mlen = (mlen>>6)+1;

	for (i=0; i<mlen; i++)
		AND.barray_[i] = lval.barray_[i] & rval.barray_[i];

	len = lval.blength_>rval.blength_?lval.blength_:rval.blength_;
	mlen = lval.blength_<rval.blength_?lval.blength_:rval.blength_;
	if (lval.blength_>rval.blength_)
	{
		for (i=mlen; i<len; i++)
			AND.set(i,false);
	}
	else
	{
		for (i=mlen; i<len; i++)
			AND.set(i,false);
	}

	return AND;
}

inline BitArray operator^ (BitArray const & lval, BitArray const & rval)
{
	if (lval.blength_ == 0)
		return rval;
	if (rval.blength_ == 0)
		return lval;

	size_t i, len = lval.blength_>rval.blength_?lval.blength_:rval.blength_;
	size_t mlen = lval.blength_<rval.blength_?lval.blength_:rval.blength_;
	BitArray XOR(len);

	len = (len>>6)+1;
	mlen = (mlen>>6)+1;

	for (i=0; i<mlen; i++)
		XOR.barray_[i] = lval.barray_[i] ^ rval.barray_[i];

	len = lval.blength_>rval.blength_?lval.blength_:rval.blength_;
	mlen = lval.blength_<rval.blength_?lval.blength_:rval.blength_;
	if (lval.blength_>rval.blength_)
	{
		for (i=mlen; i<len; i++)
			XOR.set(i,lval.get(i));
	}
	else
	{
		for (i=mlen; i<len; i++)
			XOR.set(i,rval.get(i));
	}

	return XOR;
}

inline BitArray operator~ (BitArray const & lval)
{
	if (lval.blength_ == 0)
		return BitArray();

	size_t i,len = lval.blength_;
	BitArray NOT(len);
	len = (len>>6)+1;

	for (i=0; i<len; i++)
		NOT.barray_[i] = ~(lval.barray_[i]);
	return NOT;
}

inline BitArray operator>> (BitArray const & lval, size_t shr)
{
	if (lval.blength_ == 0)
		return BitArray();

	size_t i;
	BitArray S(lval.blength_);

	// sign extend
	if (lval.get(lval.blength_-1))
		for (i=0; i<shr; i++)
			S.set(lval.blength_-shr+i,true);

	for (i=shr; i<lval.blength_; i++)
		if (lval.get(i))
			S.set(i-shr,true);
	return S;
}

inline BitArray operator<< (BitArray const & lval, size_t shl)
{
	if (lval.blength_ == 0)
		return BitArray();
	
	size_t i;
	BitArray S(lval.blength_);

	for (i=0; i<lval.blength_-shl; i++)
		if (lval.get(i))
			S.set(i+shl,true);
		
	return S;
}

#endif

