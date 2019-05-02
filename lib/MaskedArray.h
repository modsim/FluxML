#ifndef MASKEDARRAY_H
#define MASKEDARRAY_H

#include <exception>
#include "Error.h"
#include "BitArray.h"
#include "Array.h"
#include "InputProfile.h"

/**
 * Generisches maskiertes Array.
 * Ein Array der Länge 2^n, wobei nur auf bestimmte, durch eine
 * Bit-Maske vorgegebene Indizes zugegriffen werden darf.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template < typename T > class GenMaskedArray
{
private:
	size_t n_;
	BitArray mask_;
	T * values_;
public:
	struct MaskedArrayElem
	{
		BitArray const & idx;
		T & value;
		MaskedArrayElem(BitArray const & i, T * v)
			: idx(i), value(*v) {}
		MaskedArrayElem const * operator->() { return this; }
	};

	class iterator
	{
		friend class GenMaskedArray< T >;
	private:
		GenMaskedArray< T > * pobj_;
		uint32_t raw_idx_;
		uint32_t max_idx_;
		mutable BitArray idx_;
	private:
		iterator(GenMaskedArray< T > * pobj)
			: pobj_(pobj), raw_idx_(0), max_idx_(pobj->getRawSize()-1) { }
	public:
		iterator() : pobj_(0), raw_idx_(0) { };
	public:
		inline iterator & operator++()
		{
			if (raw_idx_ < max_idx_)
				raw_idx_++;
			else { pobj_ = 0; raw_idx_ = 0; }
			return *this;
		}
		inline iterator operator++(int)
		{
			iterator tmp = *this;
			++*this;
			return tmp;
		}
		inline MaskedArrayElem operator*() const
		{
			if (!pobj_)
				throw std::exception();
			uint32_t i,j;
			BitArray mask = pobj_->getMask();
			idx_ = BitArray(pobj_->getLog2Size());

			// Bits gemäß Maske extrahieren:
			for (i=0,j=0; i<mask.size(); i++)
			{
				if (mask.get(i))
				{
					if (TSTBIT(raw_idx_,j)) idx_.set(i);
					j++;
				}
			}
			return MaskedArrayElem(idx_,&((pobj_->getRawArray())[raw_idx_]));
		}

		inline MaskedArrayElem operator->() const { return operator*(); }
		inline bool operator==(iterator const & rval) const
		{
			return pobj_==rval.pobj_ && raw_idx_==rval.raw_idx_;
		}
		inline bool operator!=(iterator const & rval) const
		{
			return pobj_!=rval.pobj_ || raw_idx_!=rval.raw_idx_;
		}
	};
public:
	inline GenMaskedArray()
		: n_(0), values_(new T[1]) { }

	inline GenMaskedArray(BitArray const & mask)
		: n_(mask.size()),
		  mask_(mask),
		  values_(new T[1<<mask.countOnes()])
	{
		for (int i=(1<<mask_.countOnes())-1; i>=0; i--)
			values_[i] = T();
	}

	inline GenMaskedArray(GenMaskedArray const & copy)
		: n_(copy.n_), mask_(copy.mask_),
		  values_(new T[1<<mask_.countOnes()])
	{
		for (int i=(1<<mask_.countOnes())-1; i>=0; i--)
			values_[i] = copy.values_[i];
	}

	inline ~GenMaskedArray() { delete[] values_; }
public:
	T & operator[](BitArray const & idx)
	{
		uint32_t i,j;
		size_t idxr = 0;
		// Index muss ein Masken-Index sein:
		//fASSERT( (idx & mask_) == idx );

		// Bits gemäß Maske extrahieren:
		for (i=0,j=0; i<mask_.size(); i++)
		{
			if (mask_.get(i))
			{
				if (idx.get(i)) idxr = SETBIT(idxr,j);
				j++;
			}
		}
		return values_[idxr];
	}
	GenMaskedArray & operator=(GenMaskedArray const & rval)
	{
		n_ = rval.n_;
		mask_ = rval.mask_;
		delete[] values_;
		values_ = new T[1<<mask_.countOnes()];
		for (int i=(1<<mask_.countOnes())-1; i>=0; i--)
			values_[i] = rval.values_[i];
		return *this;
	}
        T & getValueBxAbsoluteIndx(int const & idx)
	{
		return values_[idx];
	}
	inline T * getRawArray() { return values_; }
	inline void setRawArray(const T * new_values)
	{
		for (int i=0; i<(1<<mask_.countOnes()); i++)
			values_[i] = new_values[i];
	}
	inline size_t getRawSize() const { return (1<<mask_.countOnes()); }
	inline size_t getSize() const { return (1<<n_); }
	inline size_t getLog2Size() const { return n_; }
	BitArray const & getMask() const { return mask_; }
	iterator begin() { return iterator(this); }
	iterator end() { return iterator(); }
};

typedef GenMaskedArray< double > MaskedArray;
typedef GenMaskedArray<flux::data::InputProfile > MaskedProfile;
typedef GenMaskedArray< Array< double > > MaskedArray2D;

#if 0
/**
 * Maskiertes 2D-Array von double-Werten. Es handelt sich nicht wirklich
 * um ein 2D-Array, da die zweite Dimension variabel ist.
 * Ein double-Array der Länge 2^n, wobei nur auf bestimmte, durch eine
 * Bit-Maske vorgegebene Indizes zugegriffen werden darf.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MaskedArray2D
{
private:
	size_t n_;
	BitArray mask_;
	Array< double > * values_;
public:
	struct MaskedArray2DElem
	{
		BitArray const idx;
		Array< double > & values;
		MaskedArray2DElem(BitArray const & i, Array< double > * v)
			: idx(i), values(*v) {}
		MaskedArray2DElem()
			: values(*((Array< double > *)0)) { }
		MaskedArray2DElem const * operator->() { return this; }
	};

	class iterator
	{
		friend class MaskedArray2D;
	private:
		MaskedArray2D * pobj_;
		uint32_t raw_idx_;
		uint32_t max_idx_;
	private:
		iterator(MaskedArray2D * pobj)
			: pobj_(pobj), raw_idx_(0), max_idx_(pobj->getRawSize()-1) { }
	public:
		iterator() : pobj_(0), raw_idx_(0) { };
	public:
		inline iterator & operator++()
		{
			if (raw_idx_ < max_idx_)
				raw_idx_++;
			else { pobj_ = 0; raw_idx_ = 0; }
			return *this;
		}
		inline iterator operator++(int)
		{
			iterator tmp = *this;
			++*this;
			return tmp;
		}
		inline MaskedArray2DElem operator*() const
		{
			if (!pobj_) return MaskedArray2DElem();
			uint32_t i,j;
			BitArray mask = pobj_->getMask();
			BitArray idx(pobj_->getLog2Size());

			// Bits gemäß Maske extrahieren:
			for (i=0,j=0; i<mask.size(); i++)
			{
				if (mask.get(i))
				{
					if (TSTBIT(raw_idx_,j)) idx.set(i);
					j++;
				}
			}
			return MaskedArray2DElem(idx,&((pobj_->getRawArray())[raw_idx_]));
		}
		inline MaskedArray2DElem operator->() const { return operator*(); }
		inline bool operator==(iterator const & rval) const
		{
			return pobj_==rval.pobj_ && raw_idx_==rval.raw_idx_;
		}
		inline bool operator!=(iterator const & rval) const
		{
			return pobj_!=rval.pobj_ || raw_idx_!=rval.raw_idx_;
		}
	};
public:
	inline MaskedArray2D() : n_(0), values_(0) { }

	inline MaskedArray2D(BitArray const & mask)
		: n_(mask.size()), mask_(mask)
	{
		// default-Constructor von Array< double > verwenden:
		values_ = new Array< double >[1<<mask_.countOnes()];
	}

	inline MaskedArray2D(MaskedArray2D const & copy)
		: n_(copy.n_), mask_(copy.mask_)
	{
		values_ = new Array< double>[1<<mask_.countOnes()];
		for (int i=(1<<mask_.countOnes())-1; i>=0; i--)
			values_[i] = copy.values_[i];
	}

	inline ~MaskedArray2D()
	{
		delete[] values_;
	}
public:
	Array< double > & operator[](BitArray const & idx)
	{
		uint32_t i,j;
		size_t idxr = 0;
		// Index muss ein Masken-Index sein:
		fASSERT( (idx & mask_) == idx );
		//fASSERT( idx < (uint32_t)(1<<n_) );

		// Bits gemäß Maske extrahieren:
		for (i=0,j=0; i<mask_.size(); i++)
		{
			if (mask_.get(i))
			{
				if (idx.get(i)) idxr = SETBIT(idxr,j);
				j++;
			}
		}
		return values_[idxr];
	}
	MaskedArray2D & operator=(MaskedArray2D const & rval)
	{
		delete[] values_;
		n_ = rval.n_;
		mask_ = rval.mask_;
		values_ = new Array< double >[1<<mask_.countOnes()];
		for (int i=(1<<mask_.countOnes())-1; i>=0; i--)
			values_[i] = rval.values_[i];
		return *this;
	}
	inline Array< double > * getRawArray() { return values_; }
	inline void setRawArray(const Array< double > * new_values)
	{
		for (int i=(1<<mask_.countOnes())-1; i>=0; i--)
			values_[i] = new_values[i];
	}
	inline size_t getRawSize() const { return (1<<mask_.countOnes()); }
	inline size_t getSize() const { return (1<<n_); }
	inline size_t getLog2Size() const { return n_; }
	BitArray const & getMask() const { return mask_; }
	iterator begin() { return iterator(this); }
	iterator end() { return iterator(); }
};
#endif

#endif

