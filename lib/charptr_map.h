#ifndef CHARPTR_MAP_H
#define CHARPTR_MAP_H
/*
 * fhash_map / C++ universal template-based hash data structure
 */

#include <cstring>
#include "fhash_map.h"
#include "charptr_array.h"

/* portable replacement for strdup */
#define DUP_STR(str)	strcpy(new char[strlen(str)+1],str)

/** A type for a const pointer to char */
typedef char const * charptr;

/**
 * A hash function for C style strings ("R5-Hash" from ReiserFS).
 *
 * @param str C style string
 * @return hash value
 */
size_t charptr_hashf(charptr const & str);

/** A comparison functor for C style strings. */
struct charptr_eq : public std::equal_to<charptr>
{
	bool operator()(charptr const & x, charptr const & y) const
	{ return strcmp(x,y)==0; }
};

/**
 * A Dictionary.
 * Maps a C style string (char const *) to a type T.
 *
 * @author Michael Weitzel <info@13cflux.net>
 * @param T value-type of the dictionary
 */
template< typename T > class charptr_map
	: public fhash_map< charptr,T,charptr_hashf,charptr_eq >
{
public:
	typedef fhash_map< charptr,T,charptr_hashf,charptr_eq > ch_map_t;

	/**
	 * Constructor.
	 * Initializes the charptr_map as a hash-table of preallocated length.
	 *
	 * @param hsize size of the hash table
	 */
	inline charptr_map(size_t hsize) : ch_map_t(hsize) { }

	/**
	 * Constructor.
	 */
	inline charptr_map() : ch_map_t() { }

	/**
	 * Destructor.
	 */
	inline ~charptr_map()
	{
		typename ch_map_t::iterator i;
		// deallocate keys before calling destructor of fhash_map
		for (i=ch_map_t::begin(); i!=ch_map_t::end(); i++)
			delete[] i->key;
	}

	/**
	 * Copy constructor.
	 * Copies a charptr_map by copying the underlying fhash_map.
	 * An additional run duplicates the keys.
	 *
	 * @param copy charptr_map object
	 */
	inline charptr_map(charptr_map< T > const & copy) : ch_map_t(copy)
	{
		dup_keys();
	}

public:
	inline T * findPtr(charptr const & key) const
	{
		if (key == 0) return 0;
		return ch_map_t::findPtr(key);
	}

	inline typename ch_map_t::iterator find(charptr const & key)
	{
		if (key == 0) return typename ch_map_t::iterator();
		return ch_map_t::find(key);
	}

	inline typename ch_map_t::const_iterator find(charptr const & key) const
	{
		if (key == 0) return ch_map_t::end();
		return ch_map_t::find(key);
	}

	typename ch_map_t::iterator insert(charptr const & key, T const & value)
	{
		if (key == 0) return ch_map_t::end();
		// prevent multiple allocation of keys
		if (findPtr(key))
			return ch_map_t::insert(key, value);
		charptr copy_key = DUP_STR(key);
		return ch_map_t::insert(copy_key,value);
	}

	typename ch_map_t::iterator insert(hpair< charptr,T > const & key_val_pair)
	{
		return insert(key_val_pair->key,key_val_pair->value);
	}

	bool erase(charptr const & key)
	{
		typename ch_map_t::iterator i;
		if ((i = find(key)) == ch_map_t::end())
			return false;
		delete[] i->key;
		// erase via iterator does not rely on the key:
		return ch_map_t::erase(i);
	}

	bool erase(typename ch_map_t::iterator const & iter)
	{
		if (iter == ch_map_t::end())
			return false;
		charptr key = iter->key;
		if (ch_map_t::erase(iter))
		{
			delete[] key;
			return true;
		}
		return false;
	}

	inline void clear()
	{
		typename ch_map_t::iterator i;
		for (i=ch_map_t::begin(); i!=ch_map_t::end(); i++)
			delete[] i->key;
		ch_map_t::clear();
	}

	inline T & operator[] (charptr const & key)
	{
		typename ch_map_t::iterator i;
		if (key == 0)
			return insert("",T())->value;
		if ((i = find(key)) != ch_map_t::end())
			return i->value;
		return insert(key,T())->value;
	}

	inline T const & operator[] (charptr const & key) const
	{
		typename ch_map_t::const_iterator i;
		if (key == 0 and (i = find("")) != ch_map_t::end())
			return i->value;
		if ((i = find(key)) != ch_map_t::end())
			return i->value;
		throw std::exception();
	}

	inline charptr_array getKeys() const
	{
		typename ch_map_t::const_iterator i;
		charptr_array keys;
		for (i=this->begin(); i!=this->end(); ++i)
			keys.add(i->key);
		return keys;
	}

	inline charptr_map< T > operator=(charptr_map< T > const & rval)
	{
		clear();
		ch_map_t::operator=(rval);
		dup_keys();
		return *this;
	}

private:
	void dup_keys()
	{
		for (size_t i=0; i<this->hsize_; i++)
		{
			typename ch_map_t::helement_ * cur =
				this->htable_[i];
			while (cur)
			{
				cur->key_ = DUP_STR(cur->key_);
				cur = cur->next_;
			}
		}
	}
};

#endif

