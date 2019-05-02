#ifndef CHARPTR_ARRAY_H
#define CHARPTR_ARRAY_H
/*
 * fhash_map / C++ universal template-based hash data structure
 */

#include <cstddef>
#include <cstring>
#include <cstdarg>

/**
 * A simple class for building dynamical arrays of C-style strings
 * and comfortable handling.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class charptr_array 
{
private:
	/** A struct for the linked list underlying the array */
	struct list_T { const char * str; list_T * next; };

private:
	/** Length of the linked list */
	size_t list_len_;
	/** head of the linked list */
	list_T * head_;
	/** tail of the linked list */
	list_T * tail_;
	/** internal representation used for get() */
	mutable char ** charptrptr_;
	/** internal representation used for concat() */
	mutable char * concatptr_;

public:
	/**
	 * Default constructor.
	 */
	charptr_array();

	/**
	 * Copy constructor
	 *
	 * @param copy charptr_array object to copy
	 */
	charptr_array(charptr_array const & copy);

	/**
	 * Constructor.
	 * Initializes the array using a null-terminated list of
	 * C-style strings.
	 *
	 * @param null terminated array of C-style strings
	 */
	charptr_array(char const ** ptrptr);

	/**
	 * Destructor.
	 */
	~charptr_array();
	
private:
	/**
	 * A Quick-Sort implementation.
	 *
	 * @param lo lower bound
	 * @param hi upper bound
	 */
	void qsort(int lo, int hi);

public:
	/**
	 * A const_iterator class.
	 */
	class const_iterator
	{
		friend class charptr_array;
	private:
		charptr_array const * pobj_;
		list_T * pos_;
	private:
		inline const_iterator(charptr_array const * pobj, list_T * pos)
			: pobj_(pobj), pos_(pos) { }
	public:
		inline const_iterator() : pobj_(0), pos_(0) { }

		// pre-increment operator
		inline const_iterator & operator++()
		{
			if (pobj_ == 0 || pos_ == 0) return *this; // not ok
			pos_ = pos_->next; return *this; // ok
		}

		// post-increment operator
		inline const_iterator operator++(int)
		{
			const_iterator tmp = *this;
			++*this;
			return tmp;
		}

		inline char const * operator*() const
		{
			if (pobj_ == 0 || pos_ == 0) return 0;
			return pos_->str;
		}

		inline char const * operator->() const { return operator*(); }
		inline bool operator==(const_iterator const & rval) const
		{
			return pos_==rval.pos_;
		}
		inline bool operator!=(const_iterator const & rval) const
		{
			return pos_!=rval.pos_;
		}
	};

public:
	/**
	 * Adds a complete array to the end of the array.
	 *
	 * @param P charptr_array object
	 * @return iterator pointing to the first string added
	 */
	const_iterator add(charptr_array const & P);
	
	/**
	 * Adds a string to the front of the array.
	 *
	 * @param p string to add
	 * @return iterator pointing to the newly added string
	 */
	const_iterator addFront(char const * p);
	
	/**
	 * Adds a formatted string to the end of the array.
	 *
	 * @param fmt format string
	 * @return iterator pointing to the newly added string
	 */
	const_iterator add(char const * fmt, ...);

	/**
	 * Uniquely adds a string to the end array.
	 * Returns end() of nothing was added.
	 *
	 * @param p string to add
	 * @return iterator pointing to the newly added string
	 */
	inline const_iterator addUnique(char const * p)
	{
		if (not exists(p)) return add(p);
		return end();
	}
	
	/**
	 * Uniquely adds the elements of another array.
	 *
	 * @param P another array
	 * @return iterator pointing to the first added string
	 */
	const_iterator addUnique(charptr_array const & P);

	/**
	 * Returns the size of the array.
	 *
	 * @return size of the array
	 */
	inline size_t size() const { return list_len_; }

	/**
	 * Adds a string to the array and returns a reference to the array.
	 *
	 * @param p string to add
	 * @return reference to the array
	 */
	inline charptr_array & operator, (char const * p)
	{
		add(p);
		return *this;
	}

	/**
	 * Assignment operator.
	 * Clears the array and adds a string.
	 *
	 * @param p string to add
	 * @return reference to the array
	 */
	inline charptr_array & operator= (char const * p)
	{
		clear(); add(p);
		return *this;
	}

	/**
	 * Assignment operator.
	 *
	 * @param rval array to copy
	 * @return reference to copy of rval
	 */
	charptr_array & operator= (charptr_array const & rval);

	/**
	 * Array access operator.
	 *
	 * @param idx Index
	 * @return string
	 */
	char const * operator[] (size_t idx) const;

	/**
	 * Tests for existence of a string.
	 *
	 * @param p string
	 * @return true, iff p is in the array
	 */
	bool exists(char const * p) const;

	/**
	 * find() operation.
	 *
	 * @param p string to search
	 * @return const_iterator pointing to the found element
	 */
	const_iterator find(char const * p) const;

	/**
	 * insert() operator.
	 *
	 * @param p string to insert
	 * @param i position to insert after
	 * @return const_iterator pointing to newly inserted element
	 */
	const_iterator insert(char const * p, const_iterator & i);

	/**
	 * Finds the first index of a string.
	 *
	 * @param string to search
	 * @return first index of the string or -1 if not found
	 */
	int findIndex(char const * p) const;

	/**
	 * Creates a C-style, null-terminated "char**" array of strings.
	 *
	 * @return null-terminated array of strings
	 */
	char const ** get() const;

	/**
	 * Concatenates the elements of the array using a "glue" string.
	 * Complementary to split().
	 *
	 * @param glue a "glue" used for concatenation of array elements
	 * @return pointer to an internal representation of the concatenated
	 * 	array
	 */
	char const * concat(char const * glue = 0) const;

	/**
	 * Factory-Method.
	 * Constructs an array by splitting a string.
	 *
	 * @param s string
	 * @param delims a string  containing possible delimiters
	 * @return charptr_array object representing the splitted string
	 */
	static charptr_array split(char const * s, char const * delims);

	/**
	 * Factory-Method.
	 * Constructs an array by splitting a string into pieces of specified
	 * length.
	 *
	 * @param s string
	 * @param n length of resulting sub-strings
	 */
	static charptr_array splitn(char const * s, size_t n);

	/**
	 * Sorts the array.
	 */
	void sort();

	/**
	 * Empties the array.
	 */
	void clear();

	/**
	 * Returns a const_iterator object pointing to the first array element.
	 *
	 * @return const_iterator object pointing to the first element
	 */
	inline const_iterator begin() const { return const_iterator(this,head_); }

	/**
	 * Returns a const_iterator object pointing the end of the array.
	 *
	 * @return const_iterator object pointing to the end of the array
	 */
	inline const_iterator end() const { return const_iterator(); }
};

#endif

