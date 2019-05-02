#ifndef FHASHMAP_H
#define FHASHMAP_H
/*
 * fhash_map / C++ universal template-based hash data structure
 */

#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>

// Threshold for growing the hash table when using auto rehashing
#define HASHMAP_AUTO_REHASH_LOAD_GROW .75f
#define HASHMAP_AUTO_REHASH_LOAD_SHRINK .25f
// Growth factor when using auto rehashing
#define HASHMAP_AUTO_REHASH_GROW 2.f
#define HASHMAP_AUTO_REHASH_SHRINK .5f
// Initial size of the hash tables when using auto rehashing
#define HASHMAP_AUTO_REHASH_START 16
// Minimal hash table size
#define HASHMAP_MINSIZE 16

/**
 * A pair of references consisting of a constant key and a non-constant
 * value. References an element of the hash. This struct is the result
 * when dereferencing a fhash_map<...>::iterator.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
template < typename Key, typename Val > struct hpair
{
	/** A const reference of the key */
	Key const & key;
	/** A non-const reference of the value */
	Val & value;

	/**
	 * Constructor (called by fhash_map<...>::iterator)
	 *
	 * @param k const pointer to key
	 * @param v pointer to value
	 */
	inline hpair(Key const & k, Val & v) : key(k), value(v) {}

	/**
	 * Dereferencing operator for struct access.
	 * This implementation complements the arrow operator of the
	 * fhash_map<...>::iterator class since the arrow operator is
	 * defined to be transitive ...
	 *
	 * @return a const pointer to the struct
	 */
	inline hpair const * operator->() const { return this; }

};


/**
 * A universal hash function.
 * Used by default of no better hash function is provided.
 * Computes a hash value from the memory area underlying the key value.
 *
 * @param key Key value
 * @return hash value
 */
template< typename T > size_t fhash_map_universal_hashf(T const & key)
{
        size_t a=0;
        char const * str = reinterpret_cast< char const * >(&key);
        for (size_t i=0; i<sizeof(T); i++)
        {
                a += str[i]<<4;
                a += str[i]>>4;
                a *= 11;
        }
        return a;
}

/**
 * A universal hash data structure using the simple "chaining" method.
 * Supports const and non-const iterators and the operations:
 *
 * <ul>
 *   <li>insert(Key,Value)</li>
 *   <li>erase(Key), erase(iterator)</li>
 *   <li>clear()</li>
 *   <li>rehash(newSize)</li>
 *   <li>find(Key)</li>
 *   <li>operator[](Key)</li>
 *   <li>exists(Key)</li>
 *   <li>size_t=size(), float=fillFactor(), float=collisionStats()</li>
 * </ul>
 *
 * @param Key a key type
 * @param Val a value type
 * @param hashf hash function <tt>size_t hash(const Key &)</tt> (optional)
 * @param functor for key comparison (optional)
 * @author Michael Weitzel <info@13cflux.net>
 */
template<
	typename Key,
	typename Val,
	size_t (*hashf)(Key const &) = fhash_map_universal_hashf< Key >,
	typename KeyCompare = std::equal_to<Key>
	>
class fhash_map
{
protected:
	/**
	 * An element of the hash table.
	 */
	struct helement_
	{
		Key key_;
		Val val_;
		helement_ * next_; // for chaining
		helement_(Key key, Val val, helement_ * next=0)
			: key_(key), val_(val), next_(next) {}
	};

public:
	class iterator;
	
	/**
	 * A const_iterator for iterating over the elements of the hash.
	 */
	class const_iterator
	{
		/** methods of the fhash_map class may access the private constructors */
		friend class fhash_map;
		friend class iterator;
	private:
		/** the fhash_map object the iterator points to */
		fhash_map< Key,Val,hashf,KeyCompare > const * pobj_;
		/** Current position in the hash table of the fhash_map object */
		size_t slot_;
		/** Current position in the collision list of the fhash_map object */
		helement_ const * pos_;

	private:
		/**
		 * Private constructor.
		 * Called by fhash_map<...>::begin(). Not accessible from outside
		 * the fhash_map class. slot_ is set to the first non-null entry
		 * of the hash table.
		 * 
		 * @param pobj const pointer to fhash_map object
		 */
		inline const_iterator(fhash_map< Key,Val,hashf,KeyCompare > const * pobj)
			: pobj_(pobj), slot_(0), pos_(0)
		{
			while (slot_<pobj_->hsize_
				&& pobj_->htable_[slot_]==0)
				slot_++;
			if (slot_ != pobj_->hsize_)
				pos_ = pobj_->htable_[slot_];
		}

		/**
		 * Private constructor.
		 * Called by fhash_map<...>::insert(). Not accessible from outside
		 * the fhash_map class.
		 * 
		 * @param pobj const pointer to fhash_map object
		 * @param slot slot of the hash table (index)
		 * @param pos a position in the collision list
		 */
		inline const_iterator(
			fhash_map< Key,Val,hashf,KeyCompare > const * pobj,
			size_t slot,
			helement_ const * pos
			) : pobj_(pobj), slot_(slot), pos_(pos) {}

	public:
		/**
		 * Constructor.
		 * Constructs an iterator object poiting to arbitrary "end()"
		 */
		inline const_iterator() : pobj_(0), slot_(0), pos_(0) {}

	public:
		/**
		 * Pre-increment operator.
		 * Increments the iterator and returns a reference of the iterator
		 * object.
		 *
		 * @return iterator object pointing to the next entry
		 */
		const_iterator & operator++()
		{
			if (pobj_ == 0 || slot_ == pobj_->hsize_ || pos_ == 0)
				return *this; // not ok
			if (pos_->next_) { pos_ = pos_->next_; return *this; } // ok
			pos_ = 0;
			do if (++slot_ == pobj_->hsize_) return *this; // not ok
			while (pobj_->htable_[slot_] == 0);
			pos_ = pobj_->htable_[slot_];
			return *this; // ok
		}

		/**
		 * Post-increment operator.
		 * Returns a copy of the current iterator object and increments it
		 * as a side effect.
		 *
		 * @return iterator object pointing to the current entry
		 */
		inline const_iterator operator++(int)
		{
			const_iterator tmp(*this);
			++*this;
			return tmp;
		}

		/**
		 * Dereferencing operator.
		 * Returns a Pair_T object containing key and value of the hash entry.
		 *
		 * @return Pair_T object with key and value of the hash entry.
		 */
		inline hpair< Key,const Val > operator*() const
		{
			if (!pos_) throw std::exception();
			return hpair< Key,const Val >(pos_->key_,pos_->val_);
		}

		/**
		 * Dereferencing operator.
		 * Uses the arrow operator of Pair_T (transitive!).
		 *
		 * @return Pair_T object with key and value of the hash entry
		 */
		inline hpair< Key,const Val > operator->() const { return operator*(); }

		/**
		 * Comparison (equality).
		 * Two iterator objects are equal iff their collision list
		 * pointers are equal.
		 *
		 * @param rval iterator object to compare
		 * @return true iff rval is equal
		 */
		inline bool operator==(const_iterator const & rval) const { return pos_==rval.pos_; }

		/**
		 * Comparison (inequality).
		 *
		 * @param rval iterator object to compare
		 * @return true iff rval is not equal
		 */
		inline bool operator!=(const_iterator const & rval) const { return pos_!=rval.pos_; }

		/**
		 * A cast operator for casting an iterator into bool.
		 * Returns true iff the iterator points to data.
		 *
		 * @return true iff *this!=end()
		 */
		inline operator bool () const
		{
			return pobj_ != 0
				and slot_ != pobj_->hsize_
				and pos_ != 0;
		}

	}; // class const_iterator
	
	/**
	 * An iterator for iterating over the elements of the hash.
	 */
	class iterator
	{
		/** methods of the fhash_map class may access the private constructors */
		friend class fhash_map;
	private:
		/** the fhash_map object the iterator points to */
		fhash_map< Key,Val,hashf,KeyCompare > * pobj_;
		/** Current position in the hash table of the fhash_map object */
		size_t slot_;
		/** Current position in the collision list of the fhash_map object */
		helement_ * pos_;

	private:
		/**
		 * Private constructor.
		 * Called by fhash_map<...>::begin(). Not accessible from outside
		 * the fhash_map class. slot_ is set to the first non-null entry
		 * of the hash table.
		 * 
		 * @param pobj const pointer to fhash_map object
		 */
		inline iterator(fhash_map< Key,Val,hashf,KeyCompare > * pobj)
			: pobj_(pobj), slot_(0), pos_(0)
		{
			while (slot_<pobj_->hsize_
				&& pobj_->htable_[slot_]==0)
				slot_++;
			if (slot_ != pobj_->hsize_)
				pos_ = pobj_->htable_[slot_];
		}

		/**
		 * Private constructor.
		 * Called by fhash_map<...>::insert(). Not accessible from outside
		 * the fhash_map class.
		 * 
		 * @param pobj const pointer to fhash_map object
		 * @param slot slot of the hash table (index)
		 * @param pos a position in the collision list
		 */
		inline iterator(
			fhash_map< Key,Val,hashf,KeyCompare > * pobj,
			size_t slot,
			helement_ * pos
			) : pobj_(pobj), slot_(slot), pos_(pos) {}

	public:
		/**
		 * Constructor.
		 * Constructs an iterator object poiting to arbitrary "end()"
		 */
		inline iterator() : pobj_(0), slot_(0), pos_(0) {}

	public:
		/**
		 * Pre-increment operator.
		 * Increments the iterator and returns a reference of the iterator
		 * object.
		 *
		 * @return iterator object pointing to the next entry
		 */
		iterator & operator++()
		{
			if (pobj_ == 0 || slot_ == pobj_->hsize_ || pos_ == 0)
				return *this; // not ok
			if (pos_->next_) { pos_ = pos_->next_; return *this; } // ok
			pos_ = 0;
			do if (++slot_ == pobj_->hsize_) return *this; // not ok
			while (pobj_->htable_[slot_] == 0);
			pos_ = pobj_->htable_[slot_];
			return *this; // ok
		}

		/**
		 * Post-increment operator.
		 * Returns a copy of the current iterator object and increments it
		 * as a side effect.
		 *
		 * @return iterator object pointing to the current entry
		 */
		inline iterator operator++(int)
		{
			iterator tmp(*this);
			++*this;
			return tmp;
		}

		/**
		 * Dereferencing operator.
		 * Returns a Pair_T object containing key and value of the hash entry.
		 *
		 * @return Pair_T object with key and value of the hash entry.
		 */
		inline hpair< Key,Val > operator*() const
		{
			if (!pos_) throw std::exception();
			return hpair< Key,Val >(pos_->key_,pos_->val_);
		}

		/**
		 * Dereferencing operator.
		 * Uses the arrow operator of Pair_T (transitive!).
		 *
		 * @return Pair_T object with key and value of the hash entry
		 */
		inline hpair< Key,Val > operator->() const { return operator*(); }

		/**
		 * Comparison (equality).
		 * Two iterator objects are equal iff their collision list
		 * pointers are equal.
		 *
		 * @param rval iterator object to compare
		 * @return true iff rval is equal
		 */
		inline bool operator==(iterator const & rval) const { return pos_==rval.pos_; }

		/**
		 * Comparison (inequality).
		 *
		 * @param rval iterator object to compare
		 * @return true iff rval is not equal
		 */
		inline bool operator!=(iterator const & rval) const { return pos_!=rval.pos_; }

		/**
		 * A cast operator for casting an iterator into a
		 * const_iterator. Establishes an assignment of type
		 * const_iterator=iterator.
		 *
		 * @return const_iterator object
		 */
		inline operator const_iterator() const
		{
			return const_iterator(pobj_,slot_,pos_);
		}

		/**
		 * A cast operator for casting an iterator into bool.
		 * Returns true iff the iterator points to data.
		 *
		 * @return true iff *this!=end()
		 */
		inline operator bool () const
		{
			return pobj_ != 0
				and slot_ != pobj_->hsize_
				and pos_ != 0;
		}

	}; // class iterator

protected:
	/** The hash table itself */
	helement_ ** htable_;
	/** Current length of the hash table */
	size_t hsize_;
	/** Number of elements contained in the hash */
	size_t hfill_;
	/** Key comparator */
	KeyCompare key_cmp_;
	/** Flag; automatic resizing by rehashing */
	bool auto_rehash_;

public:
	/**
	 * Array access operator taking a key value as parameter.
	 * In case no value corresponding to the key exists a new value is
	 * inserted using the default constructor of the value type.
	 * 
	 * In case this is not intended the find() operation should be used.
	 *
	 * @param key key of the value to be accessed
	 * @return reference to the value corresponding to the key
	 */
	inline Val & operator[] (Key const & key)
	{
		Val * dptr = findPtr(key);
		if (!dptr) return insert(key,Val())->value;
		return *dptr;
		// alternate implementation:
		//iterator i;
		//if ((i = find(key)) != end()) return i->value;
		//return insert(key,Val())->value;
	}

	/**
	 * Array access operator taking a key value as parameter.
	 * In case no value corresponding to the key exists std::exception
	 * is thrown.
	 *
	 * @param key key of the value to be accessed
	 * @return reference to the value corresponding to the key
	 */
	inline Val const & operator[] (Key const & key) const
	{
		Val * dptr = findPtr(key);
		if (!dptr) throw std::exception();
		return *dptr;
	}

	/**
	 * Returns an iterator object pointing to "the first" element
	 * of the hash.
	 *
	 * @return iterator pointing to the first element of the hash
	 */
	inline virtual iterator begin()
	{
		return (hfill_ == 0) ? iterator() : iterator(this);
	}

	/**
	 * Returns a const_iterator object pointing to "the first" element
	 * of the hash.
	 *
	 * @return const_iterator pointing to the first element of the hash
	 */
	inline virtual const_iterator begin() const
	{
		return (hfill_ == 0) ? const_iterator() : const_iterator(this);
	}

	/**
	 * Returns an iterator object pointing to "the end" of the hash.
	 *
	 * @return iterator pointing to "the end" of the hash
	 */
	inline virtual iterator end() { return iterator(); }
	
	/**
	 * Returns a const_iterator object pointing to "the end" of the hash.
	 *
	 * @return const_iterator pointing to "the end" of the hash
	 */
	inline virtual const_iterator end() const { return const_iterator(); }

public:
	/**
	 * Constructor.
	 * Creates a hash table of a specified size. Automatic resizing by
	 * rehashing can be controlled by an additional flag.
	 *
	 * @param hsize initial size of the hash table
	 * @param auto_rehash optional flag to enable/disable automatic resizing
	 * 	(default: false)
	 */
	inline fhash_map(size_t hsize, bool auto_rehash = false)
		: hsize_(hsize), hfill_(0), key_cmp_(), auto_rehash_(auto_rehash)
	{
		if (hsize_ < HASHMAP_MINSIZE)
			hsize_ = HASHMAP_MINSIZE;
		htable_ = new helement_*[hsize_];
		for (unsigned int i=0; i<hsize_; i++) htable_[i] = 0;
	}

	/**
	 * Constructor.
	 * Creates a hash table with the initial size of HASHMAP_AUTO_REHASH_START
	 * and resizes the hash table dynamically when new elements are inserted
	 * or erased.
	 */
	inline fhash_map()
		: hsize_(HASHMAP_AUTO_REHASH_START), hfill_(0), key_cmp_(),
		  auto_rehash_(true)
	{
		htable_ = new helement_*[hsize_];
		for (unsigned int i=0; i<hsize_; i++) htable_[i] = 0;
	}

	/**
	 * Copy-Constructor.
	 * Creates an exact copy of a hash table.
	 *
	 * @param copy hash table object to copy
	 */
	inline fhash_map(fhash_map< Key,Val,hashf,KeyCompare > const & copy)
		: hsize_(copy.hsize_), hfill_(copy.hfill_), key_cmp_(),
		  auto_rehash_(copy.auto_rehash_)
	{
		htable_ = new helement_*[hsize_];
		for (size_t i=0; i<hsize_; i++)
		{
			// copy of collision lists
			helement_ * cur, * curcp, * last;

			cur = copy.htable_[i];
			if (cur == 0) { htable_[i] = 0; continue; }
			last = 0;
			while (cur)
			{
				curcp = new helement_(*cur);
				if (last == 0)
					htable_[i] = curcp;
				else
					last->next_ = curcp;
				last = curcp;
				cur = cur->next_;
			}
			if (last) last->next_ = 0;
		}
	}

	/**
	 * Rehashing-Copy-Constructor.
	 * Creates a copy of a hash table using a hash table of differenz size.
	 *
	 * @param copy hash table object to copy
	 * @param hsize size of the new hash table
	 */
	fhash_map(fhash_map< Key,Val,hashf,KeyCompare > const & copy, size_t hsize)
		: hsize_(hsize), hfill_(copy.hfill_), key_cmp_(),
		  auto_rehash_(copy.auto_rehash_)
	{
		size_t i, hfval;
		helement_ * walk, * newE;
		htable_ = new helement_*[hsize_];

		// initialization of the new hash table
		for (i=0; i<hsize_; i++) htable_[i] = 0;
		// copy of contents
		for (i=0; i<copy.hsize_; i++)
		{
			walk = copy.htable_[i];
			while (walk)
			{
				hfval = hashf(walk->key_) % hsize_;
				newE = new helement_(walk->key_,walk->val_);
				newE->next_ = htable_[hfval];
				htable_[hfval] = newE;
				walk = walk->next_;
			}
		}
	}

	/**
	 * Destructor.
	 * Deallocates hash table and collision lists.
	 */
	virtual ~fhash_map()
	{
		for (size_t i=0; i<hsize_; i++)
		{
			if (htable_[i])
			{
				helement_ * cur, * next;
				cur = htable_[i];
				next = htable_[i]->next_;

				while (next)
				{
					delete cur;
					cur = next;
					next = next->next_;
				}
				delete cur;
			}
			
		}
		delete[] htable_;
	}
public:
	/**
	 * Insert operation.
	 * An iterator pointing to the inserted key and value is returned.
	 *
	 * @param key a key for the value to insert
	 * @param value a value to insert
	 * @return iterator pointing to the inserted key and value
	 */
	virtual iterator insert(Key const & key, Val const & value)
	{
		helement_ * newE, * walk;
		size_t hfval;
		
		if (auto_rehash_)
			rehash();

		hfval = hashf(key) % hsize_;

		if (!htable_[hfval])
		{
			// table entry was empty
			newE = new helement_( key,value );
			htable_[hfval] = newE;
		}
		else
		{
			walk = htable_[hfval];
			while (walk)
			{	
				if (key_cmp_(key, walk->key_))
				{
					// adjust the value
					walk->val_ = value;
					return iterator(this, hfval, walk);
				}
				walk = walk->next_;
			}
			newE = new helement_( key,value );
			// previous element becomes successor
			newE->next_ = htable_[hfval];
			// new element becomes first in list
			htable_[hfval] = newE;
		}
		hfill_++;
		return iterator(this, hfval, newE);
	}

	/**
	 * Insert operation.
	 * A pair of references to key and value are returned.
	 *
	 * @param key_val_pair pair of key and value
	 * @return a pair of references to the inserted element
	 */
	inline virtual iterator insert(hpair< Key,Val > const & key_val_pair)
	{
		return insert(key_val_pair->key,key_val_pair->value);
	}

	/**
	 * Deletes a value from the hash.
	 * 
	 * @param key Key of the value to delete
	 * @return true, if successful
	 */
	virtual bool erase(Key const & key)
	{
		helement_ *walk, *last;
		size_t hfval = hashf(key) % hsize_;

		if (!htable_[hfval])
			return false; // not found

		last = walk = htable_[hfval];
		while (walk)
		{
			if (key_cmp_(key, walk->key_))
			{
				// found
				if (walk == last)
				{
					if (walk->next_)
						// next element becomes first
						// of the list
						htable_[hfval] = walk->next_;
					else
						// the slot is empty again
						htable_[hfval] = 0;
				}
				else
				{
					// somewhere in the list...
					// any successors?
					if (walk->next_)
						// next element becomes next
						// element of the previous
						// element
						last->next_ = walk->next_;
					else
						// the previous element is
						// the last element
						last->next_ = 0;
				} // if (walk == last)
				
				delete walk;
				hfill_--;
				if (auto_rehash_)
					rehash();
				return true; // found and erased
			}
			else
			{
				last = walk;
				walk = walk->next_;
			} // if (key == walk->key)
		} // while (walk)
		return false; // not found
	}

	/**
	 * Deletes a value from the hash.
	 *
	 * @param iter iterator object
	 * @return true, if successful
	 */
	virtual bool erase(iterator const & iter)
	{
		if (iter.pobj_ != this)
			return false;

		helement_ * walk = htable_[iter.slot_];
		helement_ * prev = 0;

		while (walk)
		{
			if (walk == iter.pos_)
			{
				if (prev == 0)
					htable_[iter.slot_] = walk->next_;
				else
					prev->next_ = walk->next_;
				delete walk;
				hfill_--;
				if (auto_rehash_)
					rehash();
				return true;
			}

			prev = walk;
			walk = walk->next_;
		}
		return false;
	}

	/**
	 * Deflates the hash table.
	 */
	virtual void clear()
	{
		for (size_t i=0; i<hsize_; i++)
		{
			if (htable_[i])
			{
				helement_ * cur, * next;
				cur = htable_[i];
				next = htable_[i]->next_;
				while (next) { delete cur; cur = next; next = next->next_; }
				delete cur;
				htable_[i] = 0;
			}
		}
		hfill_ = 0;
		if (auto_rehash_)
			rehash();
	}

	/**
	 * Searches a value by its key and returns a iterator.
	 *
	 * @param key Key of the Value to search
	 * @return iterator object pointing to the value or end()
	 */
	inline virtual iterator find(Key const & key)
	{
		helement_ * walk;
		size_t hfval = hashf(key) % hsize_;

		// sequential search of the collision list
		if ((walk = htable_[hfval]) == 0) return iterator(); // not found

		while (walk)
		{
			if (key_cmp_(key, walk->key_))
				return iterator(this, hfval, walk); // found
			walk = walk->next_;
		}
		return iterator(); // not found
	}

	/**
	 * Searches a value by its key and returns a const_iterator.
	 *
	 * @param key Key of the Value to search
	 * @return const_iterator object pointing to the value or end()
	 */
	inline virtual const_iterator find(Key const & key) const
	{
		helement_ * walk;
		size_t hfval = hashf(key) % hsize_;

		// search by iterating through the collision list
		if ((walk = htable_[hfval]) == 0) return const_iterator(); // not found

		while (walk)
		{
			if (key_cmp_(key, walk->key_))
				return const_iterator(this, hfval, walk); // found
			walk = walk->next_;
		}
		return const_iterator(); // not found
	}

	/**
	 * Searches a value by its key and returns a pointer to the value,
	 * or a null pionter in case no element is found.
	 *
	 * @param key Key of the Value to search
	 * @return Pointer to the found object or null pointer
	 */
	inline virtual Val * findPtr(Key const & key) const
	{
		helement_ *walk;
		size_t hfval = hashf(key) % hsize_;

		// search by iterating through the collision list
		if ((walk = htable_[hfval]) == 0) return 0; // not found

		while (walk)
		{
			if (key_cmp_(key, walk->key_))
				return &(walk->val_); // found
			walk = walk->next_;
		}
		return 0; // not found
	}

	/**
	 * Tests for existence of a key in the hash table.
	 *
	 * @param key Key
	 */
	inline virtual bool exists(Key const & key) const { return findPtr(key) != 0; }

	/**
	 * Rehashing. Grows or shrinks the hash table.
	 * Running time is O(max{hfill_,hsize_,newhsize}).
	 * If no new hash table size is given the new size is determined
	 * automatically.
	 *
	 * @param newhsize new size of the hash table (optional).
	 */
	virtual void rehash(size_t newhsize = 0)
	{
		if (newhsize == 0)
		{
			if (fillFactor() >= HASHMAP_AUTO_REHASH_LOAD_GROW)
				newhsize = size_t(hsize_ * HASHMAP_AUTO_REHASH_GROW);
			else if (fillFactor() <= HASHMAP_AUTO_REHASH_LOAD_SHRINK)
				newhsize = size_t(hsize_ * HASHMAP_AUTO_REHASH_SHRINK);
			else
				return;
			if (newhsize < HASHMAP_MINSIZE)
				newhsize = HASHMAP_MINSIZE;
		}

		size_t i, hfval;
		helement_ * walk, * next, * newE;
		helement_ ** newhtable = new helement_*[newhsize];

		for (i=0; i<newhsize; i++) newhtable[i] = 0;
		for (i=0; i<hsize_; i++)
		{
			walk = htable_[i];
			while (walk)
			{
				hfval = hashf(walk->key_) % newhsize;
				newE = walk;
				next = walk->next_;
				newE->next_ = newhtable[hfval] ? newhtable[hfval] : 0;
				newhtable[hfval] = newE;
				walk = next;
			}
		}

		delete[] htable_;
		htable_ = newhtable;
		hsize_ = newhsize;
	}

	/**
	 * Assignment operator.
	 *
	 * @param rval hash table to copy
	 */
	fhash_map< Key,Val,hashf,KeyCompare > & operator=(
		fhash_map< Key,Val,hashf,KeyCompare > const & rval
		)
	{
		size_t i;
		clear();
		delete[] htable_;
		// create a copy
		hsize_ = rval.hsize_;
		hfill_ = rval.hfill_;
		auto_rehash_ = rval.auto_rehash_;
		htable_ = new helement_*[hsize_];
		for (i=0; i<hsize_; i++)
		{
			helement_ * cur, * curcp, * last;
			cur = rval.htable_[i];
			if (cur == 0) { htable_[i] = 0; continue; }
			last = 0;
			while (cur)
			{
				curcp = new helement_(*cur);
				if (last == 0)
					htable_[i] = curcp;
				else
					last->next_ = curcp;
				last = curcp;
				cur = cur->next_;
			}
			if (last) last->next_ = 0;
		}
		return *this;
	}

	/**
	 * Merge operator lval += rval.
	 * Merges the contents of rval into lval.
	 *
	 * @param rval right argument / source hash table
	 */
	fhash_map< Key,Val,hashf,KeyCompare > & operator+=(
		fhash_map< Key,Val,hashf,KeyCompare > const & rval
		)
	{
		size_t i;
		helement_ * walk;
		for (i=0; i<rval.hsize_; i++)
		{
			walk = rval.htable_[i];
			while (walk)
			{
				insert(walk->key_,walk->val_);
				walk = walk->next_;
			}
		}
		return *this;
	}
	
	/**
	 * Difference operator lval -= rval.
	 * Removes all entries corresponding to keys in rval from lval.
	 *
	 * @param rval right argument
	 */
	fhash_map< Key,Val,hashf,KeyCompare > & operator-=(
		fhash_map< Key,Val,hashf,KeyCompare > const & rval
		)
	{
		size_t i;
		helement_ * walk;
		for (i=0; i<rval.hsize_; i++)
		{
			walk = rval.htable_[i];
			while (walk)
			{
				erase(walk->key_);
				walk = walk->next_;
			}
		}
		return *this;
	}

	/**
	 * Returns the number of elements currently stored in the hash table.
	 *
	 * @return number of elements currently stored in the hash table
	 */
	inline virtual size_t size() const { return hfill_; }

	/**
	 * Returns the current size of the hash table.
	 *
	 * @return current size of the hash table
	 */
	inline virtual size_t tableSize() const { return hsize_; }

	/**
	 * Returns the fill factor of the hash table.
	 *
	 * @return fill factor of the hash table
	 */
	inline virtual float fillFactor() const { return float(hfill_)/float(hsize_); }

	/**
	 * Computes statistical parameters from the distribution of collision
	 * list lengths which helps to judge the quality of a hash function.
	 *
	 * A "p-value" for the computed chisq value may by computed by
	 *   p = 1-chi2cdf(chisq,chidof)
	 * A value of p>0.1 indicates that the hash values have approximately
	 * uniform distribution and the hash function works well. In detail:
	 *    p < 0.01:         very strong evidence against uniform dist.
	 *    0.01 <= p < 0.05: strong evidence against uniform distribution.
	 *    0.05 <= p <= 0.1: weak evidence against uniform distribution.
	 *    p > 0.1:          little or no evidence against uniform dist.
	 *
	 * @param N size of the hash table
	 * @param E estimated expectation of the collision list length
	 * @param V estimated variance of the distribution of collision
	 * 	list lengths
	 * @param chisq chi-square value of the distribution of
	 * 	collision list lengths
	 * @param chidof number of degrees of freedom of the chi-square
	 * 	distribution
	 */
	virtual void stats(
		int & N,
		double & E,
		double & V,
		double & chisq,
		int & chidof
		) const
	{
		helement_ * walk;
		size_t i,len;
		// expectation of collision list length
		double E_ = double(hfill_)/double(hsize_);
		// variance of collision list length
		double f1 = 0., f2 = 0.;
		// chi^2 value
		double chisq_ = 0.;

		for (i=0; i<hsize_; i++)
		{
			// determine length of collision list
			len = 0;
			walk = htable_[i];
			while (walk) { len++; walk = walk->next_; }
			// update of chi^2 value:
			chisq_ += (double(len)-E_)*(double(len)-E_)/E_;
			// update of variance:
			f1 += len*len; f2 += len;
		}
		N = hsize_;
		E = E_;
		V = (f1 - f2*f2/hsize_)/(hsize_-1.);
		chisq = chisq_;
		chidof = hsize_-1;
	}

};

#endif

