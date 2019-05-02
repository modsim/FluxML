#ifndef BITARRAY_H
#define BITARRAY_H

#include <cstddef>
extern "C" {
#include <stdint.h>
}

/**
 * Klasse zur Abbildung eines Arrays von Bits.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class BitArray
{
	// TODO: diese friend-Operatoren könnten teilweise etwas effizienter
	//       als Methoden von BitArray implementiert werden.
	friend BitArray operator| (BitArray const &, BitArray const &);
	friend BitArray operator& (BitArray const &, BitArray const &);
	friend BitArray operator^ (BitArray const &, BitArray const &);
	friend BitArray operator+ (BitArray const &, BitArray const &);
	friend BitArray operator- (BitArray const &, BitArray const &);
	friend BitArray operator- (BitArray const &);
	friend BitArray operator* (BitArray const & lval, BitArray const & rval);
	friend BitArray operator/ (BitArray const & lval, BitArray const & rval);
	friend BitArray operator~ (BitArray const &);
	friend BitArray operator<<(BitArray const &, size_t);
	friend BitArray operator>>(BitArray const &, size_t);

public:
	/**
	 * Proxy-Klasse für einen Eintrag des BitArrays.
	 * Erlaubt die Zuweisung von BitArray-Elementen mittels
	 * operator[].
	 */
	class BitProxy
	{
		friend class BitArray;
	private:
		/** Zeiger auf übergeordnetes BitArray Objekt */
		BitArray * parent_;
		/** Unteres Intervall-Ende */
		size_t i_;
		/** Oberes Intervall-Ende */
		size_t j_;
	private:
		/**
		 * Privater Constructor (nur für BitArray)
		 * Erzeugt ein BitProxy, der auf einen einzelnen Eintrag
		 * "zeigt".
		 *
		 * @param parent übergeordnetes BitArray-Objekt
		 * @param i Index eines Eintrags im BitArray-Objekt parent
		 */
		inline BitProxy(BitArray * parent, size_t i)
			: parent_(parent), i_(i), j_(i) { }
		
		/**
		 * Privater Constructor (nur für BitArray)
		 * Erzeugt ein BitProxy, der auf einen Slice von Einträgen
		 * "zeigt".
		 *
		 * @param parent übergeordnetes BitArray-Objekt
		 * @param i Index; unteres Intervall-Ende (inklusive)
		 * @param j Index; oberes Intervall-Ende (inklusive)
		 */
		inline BitProxy(BitArray * parent, size_t i, size_t j)
			: parent_(parent), i_(i), j_(j) { }

	public:
		/**
		 * Cast-Operator nach bool.
		 *
		 * @return true, falls sich innerhalb des Slices gesetzte
		 * 	Bits befinden
		 */
		inline operator bool() const
		{
			for (size_t k=i_; k<=j_; k++)
				if (parent_->get(i_)) return true;
			return false;
		}

		/**
		 * Für Zuweisungen der Form X[i] = true.
		 *
		 * @param rval Wahrheitswert
		 * @return Referenz auf das BitProxy-Objekt
		 */
		inline BitProxy & operator=(bool const & rval)
		{
			for (size_t k=i_; k<=j_; k++)
				parent_->set(k,rval);
			return *this;
		}

		/**
		 * Für Zuweisungsketten der Form X[i] = X[j] = true.
		 * Werden hier überlappende Bereiche einander zugewiesen gibt
		 * es Probleme ... aber das sollte der Anwender erwarten ...
		 *
		 * @param rval BitProxy-Objekt
		 * @return Referenz auf das BitProxy-Objekt
		 */
		inline BitProxy & operator=(BitProxy const & rval)
		{
			for (size_t k=i_; k<=j_; k++)
				parent_->set(k,	rval.parent_->get(rval.i_+k-i_));
			return *this;
		}

		/**
		 * Zuweisung A(i,j) = B.
		 *
		 * @param rval BitArray-Objekt
		 */
		inline BitProxy & operator=(BitArray const & rval)
		{
			for (size_t k=i_; k<=j_; k++)
				parent_->set(k,	rval.get(k-i_));
			return *this;
		}

	}; // class BitArray::BitProxy

	/**
	 * Iterator für die Erzeugung aller Kombinationen; evtl. mit Maskierung
	 *
	 * @author Michael Weitzel <info@13cflux.net>
	 */
	class comb_iterator
	{
		friend class BitArray;

	private:
		/** Die aktuell vom Iterator verwaltete Kombination */
		BitArray * comb_;
		/** Initialisierung für Kombinationsgenerierung (k) */
		size_t comb_k_;
		/** Maskierung */
		BitArray * comb_mask_;
		/** Kombination für Maskierung */
		comb_iterator * comb_mask_iter_;
		/** Gültigkeit */
		bool is_valid_;

	private:
		/**
		 * Privater Constructor (nur für BitArray).
		 *
		 * @param mask Bit-Maske der Kombination (Gesamtheit)
		 * @param k Größe der Auswahl
		 * @param start_last falls true mit letzter Kombination beginnen
		 */
		inline comb_iterator(
			BitArray const & mask,
			size_t k,
			bool start_last
			) : comb_k_(k), comb_mask_iter_(0), is_valid_(true)
		{
			comb_ = new BitArray(mask.size());
			comb_mask_ = new BitArray(mask);
			if (start_last)
				last_comb();
			else
				first_comb();
		}

		/**
		 * Privater Constructor (nur für BitArray).
		 *
		 * @param n Größe der Gesamtheit
		 * @param k Größe der Auswahl (k<=n)
		 * @param start_last falls true mit letzter Kombination beginnen
		 */
		inline comb_iterator(size_t n, size_t k, bool start_last)
			: comb_k_(k), comb_mask_(0), comb_mask_iter_(0),
			  is_valid_(true)
		{
			comb_ = new BitArray(n);
			if (start_last)
				last_comb();
			else
				first_comb();
		}

	public:
		/**
		 * Default-Constructor.
		 */
		inline comb_iterator() : comb_(0), comb_k_(0),
			comb_mask_(0), comb_mask_iter_(0), is_valid_(false) { }

		/**
		 * Copy-Constructor.
		 *
		 * @param copy zu kopierendes comb_iterator Objekt
		 */
		inline comb_iterator(comb_iterator const & copy)
			: comb_(0), comb_k_(copy.comb_k_),
			  comb_mask_(0), comb_mask_iter_(0),
			  is_valid_(copy.is_valid_)
		{
			if (copy.comb_) comb_ = new BitArray(*(copy.comb_));
			if (copy.comb_mask_)
				comb_mask_ = new BitArray(*(copy.comb_mask_));
			if (copy.comb_mask_iter_)
				comb_mask_iter_ =
				new comb_iterator(*(copy.comb_mask_iter_));
		}
		
		/**
		 * Destructor.
		 */
		inline ~comb_iterator()
		{
			deallocate();
		}

		/**
		 * Zuweisungsoperator.
		 *
		 * @param copy zu kopierendes comb_iterator Objekt
		 */
		inline comb_iterator & operator=(comb_iterator const & copy)
		{
			deallocate();
			comb_k_ = copy.comb_k_;
			is_valid_ = copy.is_valid_;
			if (copy.comb_)
				comb_ = new BitArray(*(copy.comb_));
			if (copy.comb_mask_)
				comb_mask_ = new BitArray(*(copy.comb_mask_));
			if (copy.comb_mask_iter_)
				comb_mask_iter_ =
				new comb_iterator(*(copy.comb_mask_iter_));
			return *this;
		}
	
	private:
		/** Deallokiert die Kombination, die Maske und deren Iterator */
		inline void deallocate()
		{
			delete comb_;
			comb_ = 0;
			delete comb_mask_;
			comb_mask_ = 0;
			delete comb_mask_iter_;
			comb_mask_iter_ = 0;
		}

		/** Initialisiert die erste Kombination */
		inline void first_comb()
		{
			if (not is_valid_)
				return;

			if (comb_mask_ == 0)
			{
				if (comb_k_ > 0)
					comb_->ones(0,comb_k_-1);
				else
					comb_->zeros();
			}
			else
			{
				delete comb_mask_iter_;
				comb_mask_iter_ = new comb_iterator(
							comb_mask_->countOnes(),
							comb_k_,
							false
							);
			}
		}

		/** Initialisiert die letzte Kombination */
		inline void last_comb()
		{
			if (not is_valid_)
				return;

			if (comb_mask_ == 0)
			{
				if (comb_k_ > 0)
					comb_->ones(
						comb_->size()-comb_k_,
						comb_->size()-1
						);
				else
					comb_->zeros();
			}
			else
			{
				delete comb_mask_iter_;
				comb_mask_iter_ = new comb_iterator(
							comb_mask_->countOnes(),
							comb_k_,
							true
							);
			}
		}

		/** Lädt die Kombination aus comb_mask_comb_ */
		void load_comb()
		{
			if (not is_valid_)
				return;
			if (comb_mask_iter_ == 0 or
				comb_mask_iter_->is_valid_ == false)
			{
				is_valid_ = false;
				return;
			}

			size_t i,j;
			BitArray const * comb_mask_comb =
				comb_mask_iter_->comb_;
			comb_->zeros();
			for (i=0,j=0; i<comb_->size(); i++)
			{
				if (comb_mask_->get(i))
				{
					if (comb_mask_comb->get(j))
						comb_->set(i);
					j++;
				}
			}
		}

	public:
		/**
		 * Prä-Inkrement-Operator
		 *
		 * @return Inkrementiertes Iterator-Objekt (nächste Kombination)
		 */
		inline comb_iterator & operator++()
		{
			if (not is_valid_)
				return *this;

			if (comb_mask_ != 0)
			{
				++*comb_mask_iter_;
				if (not comb_mask_iter_->is_valid_)
					is_valid_ = false;
				return *this;
			}
			
			if (isLastComb())
			{
				// iterator wird ungültig (end())
				is_valid_ = false;
				return *this;
			}
			
			//BitArray hibit(comb_->size()), lobit(comb_->size());
			BitArray & comb = *comb_;

			// niedrigstes gesetztes Bit
			BitArray lobit(-comb); //lobit = comb & -comb;
			lobit &= comb;
			// ersetze niedrigsten 1er block d. 1 links davon
			comb += lobit; //comb = comb + lobit;
			// erstes gelöschtes Bit hinter niedrigsten Block
			BitArray hibit(-comb); // hibit = comb & -comb; 
			hibit &= comb;
			// niedrigster Block
			hibit -= lobit; //hibit = hibit - lobit;
			// schiebe Block ans untere Ende
			while (not hibit.get(0))
				hibit >>= 1; //hibit = hibit >> 1;
			hibit >>= 1;
			comb |= hibit; // comb = comb | (hibit>>1);
			return *this;
		}

		/**
		 * Post-Inkrement-Operator.
		 * (Inkrementierung ist ein Seiteneffekt).
		 *
		 * @return Iterator-Objekt vor Inkrementierung
		 */
		inline comb_iterator operator++(int)
		{
			if (comb_mask_)
			{
				comb_iterator tmp(*this);
				++(*comb_mask_iter_);
				if (comb_mask_iter_->is_valid_ == false)
					is_valid_ = false;
				return tmp;
			}
			comb_iterator tmp(*this);
			++*this;
			return tmp;
		}

		/**
		 * Prä-Dekrement-Operator
		 *
		 * @return Dekrementiertes Iterator-Objekt
		 * 	(vorherige Kombination)
		 */
		inline comb_iterator & operator--()
		{
			if (not is_valid_)
				return *this;

			if (comb_mask_ != 0)
			{
				--*comb_mask_iter_;
				if (not comb_mask_iter_->is_valid_)
					is_valid_ = false;
				return *this;
			}
			
			if (isFirstComb())
			{
				// iterator wird ungültig (end())
				is_valid_ = false;
				return *this;
			}
			
			*comb_ = ~(*comb_);
			comb_k_ = comb_->size() - comb_k_;
			++*this;
			comb_k_ = comb_->size() - comb_k_;
			*comb_ = ~(*comb_);
			return *this;
		}

		/**
		 * Post-Dekrement-Operator.
		 * (Dekrementierung ist ein Seiteneffekt).
		 *
		 * @return Iterator-Objekt vor Dekrementierung
		 */
		inline comb_iterator operator--(int)
		{
			if (comb_mask_)
			{
				comb_iterator tmp(*this);
				--(*comb_mask_iter_);
				if (comb_mask_iter_->is_valid_ == false)
					is_valid_ = false;
				return tmp;
			}
			comb_iterator tmp(*this);
			--*this;
			return tmp;
		}

		/**
		 * Dereferenzierungsoperator.
		 *
		 * @return const-Referenz auf erzeugte Kombination
		 */
		inline BitArray const & operator*()
		{
			if (comb_mask_)
				load_comb();
			return *comb_;
		}

		/**
		 * Dereferenzierungsoperator.
		 *
		 * @return const-Zeiger auf erzeugte Kombination
		 */
		inline BitArray const * operator->()
		{
			if (comb_mask_)
				load_comb();
			return comb_;
		}

		/**
		 * Vergleichsoperator.
		 *
		 * @param rval rechtes Argument
		 * @return true, falls *this == rval
		 */
		inline bool operator==(comb_iterator const & rval)
		{

			if ((not is_valid_) and (not rval.is_valid_))
				return true;
			if ((not is_valid_) != (not rval.is_valid_))
				return false;
			if (comb_mask_)
				load_comb();
			return *comb_ == *(rval.comb_);
		}

		/**
		 * Vergleichsoperator (Ungleichheit).
		 *
		 * @param rval rechtes Argument
		 * @return true, falls *this != rval
		 */
		inline bool operator!=(comb_iterator const & rval)
		{
			return not operator==(rval);
		}

		/**
		 * Testet, ob die aktuelle Kombination die lexikographisch
		 * letzte Kombination ist.
		 *
		 * @return true, falls aktuelle Kombination die letzte
		 * 	Kombination ist
		 */
		bool isLastComb()
		{
			if (not is_valid_)
				return false;
			if (comb_mask_)
				return comb_mask_iter_->isLastComb();
			return comb_->allOnes(
				comb_->size()-comb_k_,
				comb_->size()-1
				);
		}

		/**
		 * Testet, ob die aktuelle Kombination die lexikographisch
		 * erste Kombination ist.
		 *
		 * @return true, falls aktuelle Kombination die erste
		 * 	Kombination ist
		 */
		bool isFirstComb()
		{
			if (not is_valid_)
				return false;
			if (comb_mask_)
				return comb_mask_iter_->isFirstComb();
			return comb_->allOnes(0,comb_k_-1);
		}

		/**
		 * Cast-Operator nach bool.
		 *
		 * @return true, falls Iterator gültig ist (!= end())
		 */
		operator bool () { return is_valid_; }

	}; // class comb_iterator

private:
	/** interner Speicher für die Bits */
	uint64_t * barray_;
	
	/** Länge des internen Speichers barray_ in Bytes */
	size_t length_;
	
	/** Anzahl der in barray_ verwendeten Bits */
	size_t blength_;

	/** String-Repräsentation */
	mutable char * bstr_;

public:
	/**
	 * Default-Constructor.
	 * Initialisiert einen leeres Bit-Array.
	 */
	inline BitArray();

	/**
	 * Constructor.
	 * Legt ein Bit-Array mit blength Bits an.
	 *
	 * @param blength Anzahl der zu reservierenden Bits
	 */
	inline BitArray(size_t blength);

	/**
	 * Copy-Constructor.
	 * 
	 * @param copy zu kopierendes Bit-Array
	 */
	inline BitArray(BitArray const & copy);

	/**
	 * Constructor.
	 * Baut ein BitArray aus einem Slice.
	 *
	 * @param slice BitProxy-Objekt
	 */
	inline BitArray(BitProxy const & slice);

	/**
	 * Destructor.
	 */
	inline ~BitArray();

	/**
	 * Zuweisungs / Copy-Operator.
	 * Erstellt eine Kopie eines Bit-Arrays.
	 *
	 * @param copy zu kopierendes Bit-Array
	 */
	inline BitArray & operator= (BitArray const & copy);

	/**
	 * Zuweisung.
	 * Zuweisung eines vorzeichenlosen 64-Bit-Integers
	 *
	 * @param ival Wert des 64-Bit-Integers
	 */
	inline BitArray & operator= (uint64_t ival);

public:
	/**
	 * Gibt die Anzahl der Bits zurück
	 *
	 * @return Anzahl der Bits im Bit-Array
	 */
	inline size_t size() const { return blength_; }
	
	/**
	 * Greift auf die Komponente idx des Bit-Arrays zu und gibt den
	 * Bit-Wert als Wahrheitswert zurück.
	 *
	 * @param idx zuzugreifender Index
	 */
	inline bool get(size_t idx) const;

	/**
	 * Array-Zugriffsoperator.
	 * Greift auf die Komponente idx des Bit-Arrays zu und gibt den
	 * Bit-Wert als Wahrheitswert zurück.
	 *
	 * @param idx zuzugreifender Index
	 * @return Bit-Wert der Komponente idx
	 */
	inline BitProxy operator[] (size_t idx);

	/**
	 * Slice-Zugriff.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 * @return Slice
	 */
	inline BitProxy operator()(size_t i, size_t j);

	/**
	 * Zuweisung eines Slice.
	 *
	 * @param slice zuzuweisender Slice
	 */
	inline BitArray & operator=(BitProxy const & slice);

	/**
	 * Setzt das Bit idx auf den Wert val.
	 *
	 * @param idx zuzugreifender Index
	 * @param val neuer Wert des Bits
	 */
	inline void set(size_t idx, bool val = true);

	/**
	 * Fügt ein fremdes BitArray bei einem Offset ein.
	 * Die Größe des BitArrays wird dabei, falls nötig, angepasst.
	 *
	 * @param offset Offset, an dem das BitArray eingefügt werden soll
	 * @param ba einzufügendes BitArray
	 */
	inline void set(size_t offset, BitArray const & ba);

	/**
	 * Löschen des Bits idx.
	 *
	 * @param idx zuzugreifender Index
	 */
	inline void clear(size_t idx) { set(idx,false); }

	/**
	 * Flippen.
	 *
	 * @param idx zuzugreifender Index
	 */
	inline void flip(size_t idx) { set(idx,not get(idx)); }

	/**
	 * Testet, ob das Bit-Array mit 1en gefüllt ist.
	 *
	 * @return true, falls das Bit-Array mit 1en gefüllt ist
	 */
	inline bool allZeros() const;
	
	/**
	 * Testet, ob in einem Bereich ausschließlich gelöschte Bits sind.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 * @return true, falls alle Bits in [i,j] gelöscht
	 */
	inline bool allZeros(size_t i, size_t j) const;

	/**
	 * Testet, ob das Bit-Array mit 0en gefüllt ist.
	 *
	 * @return true, falls das Bit-Array mit 0en gefüllt ist
	 */
	inline bool allOnes() const;

	/**
	 * Testet, ob in einem Bereich ausschließlich gesetzte Bits sind.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 * @return true, falls alle Bits in [i,j] gesetzt
	 */
	inline bool allOnes(size_t i, size_t j) const;

	/**
	 * Testet, ob das Bit-Array 0en enthält.
	 *
	 * @return true, falls das Bit-Array irgendwo eine 0 enthält
	 */
	inline bool anyZeros() const;

	/**
	 * Testet, ob sich in einem Bereich gelöschte Bits befinden.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 * @return true, falls gelöschte Bits in [i,j]
	 */
	inline bool anyZeros(size_t i, size_t j) const;

	/**
	 * Testet, ob das Bit-Array 1en enthält.
	 *
	 * @return true, falls das Bit-Array irgendwo eine 1 enthält
	 */
	inline bool anyOnes() const;

	/**
	 * Testet, ob sich in einem Bereich gesetzte Bits befinden.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 * @return true, falls gesetzte Bits in [i,j]
	 */
	inline bool anyOnes(size_t i, size_t j) const;

	/**
	 * Füllt das BitArray mit 0en.
	 */
	inline void zeros();

	/**
	 * Löscht die Bits i bis einschließlich j.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 */
	inline void zeros(size_t i, size_t j);

	/**
	 * Füllt das BitArray mit 1en.
	 */
	inline void ones();
	
	/**
	 * Setzt die Bits i bis einschließlich j.
	 *
	 * @param i untere Grenze
	 * @param j obere Grenze
	 */
	inline void ones(size_t i, size_t j);

	/**
	 * Zählt die 1-Bits.
	 *
	 * @return Anzahl gesetzter Bits
	 */
	inline size_t countOnes() const;

	/**
	 * Zählt die 0-Bits.
	 *
	 * @return Anzahl nicht-gesetzter Bits
	 */
	inline size_t countZeros() const;
	
	/**
	 * Gibt den Index des höchsten gesetzten Bits zurück.
	 * Das entspricht floor(log2(x)). Wenn keine Bits gesetzt sind,
	 * wird -1 zurückgegeben (da log2(0) = -inf).
	 *
	 * @return Logarithmus zur Basis 2, falls non-zero
	 */
	int highestBit() const;

	/**
	 * Gibt den Index des niedrigsten gesetzten Bits zurück.
	 *
	 * @return Index des niedrigsten Bits; -1 falls es kein Bit gesetzt
	 */
	int lowestBit() const;

	/**
	 * Konvertierung in Binär-String (LSB zuerst).
	 *
	 * @param zero Zeichen für das 0-Bit
	 * @param one Zeichen für das 1-Bit
	 * @return Zeiger auf Zeichenkette (intern allokiert)
	 */
	inline char const * toString(char zero='0', char one='1') const;

	/**
	 * Konvertierung in einen Hexadezimal-String (LSB zuerst)
	 *
	 * @param ucase Großbuchstaben für Hex-Ziffern verwenden
	 * @return Zeiger auf Zeichenkette (intern allokiert)
	 */
	inline char const * toHexString(bool ucase = false) const;

	/**
	 * Konvertierung in String (MSB zuerst).
	 *
	 * @param zero Zeichen für das 0-Bit
	 * @param one Zeichen für das 1-Bit
	 * @return Zeiger auf Zeichenkette (intern allokiert)
	 */
	inline char const * toStringRev(char zero='0', char one='1') const;
	
	/**
	 * Konvertierung in einen Hexadezimal-String (MSB zuerst)
	 *
	 * @param ucase Großbuchstaben für Hex-Ziffern verwenden
	 * @return Zeiger auf Zeichenkette (intern allokiert)
	 */
	inline char const * toHexStringRev(bool ucase = false) const;

	/**
	 * Konveritiert einen String in Hexadezimaldarstellung in ein
	 * BitArray. Der hexadezimal-String wird als LSB-first angenommen.
	 * Das erzeugte BitArray hat die Länge 4*#hexdigits+1. Das
	 * überzählige Bit hat grundsätzlich den Wert 0 -- d.h. die
	 * übergebene Zeichenkette wird als positiver Integer interpretiert.
	 * Wird das nicht gewünscht, muss das überzählige Bit mittels resize
	 * nachträglich angeschnitten werden.
	 *
	 * @param h hexadezimal-String in LSB-first Reihenfolge
	 * @return Zeiger auf allokiertes BitArray (0-Zeiger bei Fehler)
	 */
	static inline BitArray * parseHex(char const * h);

	/**
	 * Konveritiert einen String in Binärdarstellung in ein BitArray.
	 * Der binär-String wird als LSB-first angenommen. Alle Zeichen
	 * außer 1 werden als 0 gewertet. Das erzeugte BitArray ist immer
	 * einen Eintrag größer als die Länge des übergebenen Strings und
	 * das überzählige Bit hat grundsätzlich den Wert 0 -- d.h. die
	 * übergebene Zeichenkette wird als positiver Integer
	 * interpretiert. Wird das nicht gewünscht, muss das überzählige
	 * Bit mittels resize nachträglich angeschnitten werden.
	 *
	 * @param h binär-String in LSB-first Reihenfolge
	 * @return Zeiger auf allokiertes BitArray (0-Zeiger bei Fehler)
	 */
	static inline BitArray * parseBin(char const * h);

	/**
	 * Initialisierung der lexikographisch ersten Kombination.
	 *
	 * @param n Gesamtheit
	 * @param k Auswahl
	 * @return comb_iterator-Objekt
	 */
	static inline comb_iterator firstComb(size_t n, size_t k)
	{
		if (k>n) return comb_iterator();
		return comb_iterator(n,k,false);
	}
	
	/**
	 * Initialisierung der lexikographisch letzten Kombination.
	 *
	 * @param n Gesamtheit
	 * @param k Auswahl
	 * @return comb_iterator-Objekt
	 */
	static inline comb_iterator lastComb(size_t n, size_t k)
	{
		if (k>n) return comb_iterator();
		return comb_iterator(n,k,true);
	}

	/**
	 * Initialisierung der lexikographisch ersten Kombination relativ
	 * zu einer angegebenen Maskierung.
	 *
	 * @param mask Maskierung
	 * @param k Auswahl
	 * @return comb_iterator-Objekt
	 */
	static inline comb_iterator firstComb(BitArray const & mask, size_t k)
	{
		if (k>mask.countOnes()) return comb_iterator();
		return comb_iterator(mask,k,false);
	}
	
	/**
	 * Initialisierung der lexikographisch letzten Kombination relativ
	 * zu einer angegebenen Maskierung.
	 *
	 * @param mask Maskierung
	 * @param k Auswahl
	 * @return comb_iterator-Objekt
	 */
	static inline comb_iterator lastComb(BitArray const & mask, size_t k)
	{
		if (k>mask.countOnes()) return comb_iterator();
		return comb_iterator(mask,k,true);
	}

	/**
	 * Ende-Symbol für einen comb_iterator.
	 *
	 * @return Ende-Symbol für einen comb_iterator
	 */
	static inline comb_iterator noComb()
	{
		return comb_iterator();
	}

	/**
	 * Inkrement mit Maske
	 *
	 * @param mask Maske
	 */
	inline BitArray & incMask(BitArray const & mask);

public:
	/**
	 * Präfix-Inkrement
	 *
	 * @return Referenz auf inkrementiertes *this
	 */
	inline BitArray & operator++();
	
	/**
	 * Postfix-Inkrement
	 *
	 * @return uninkrementierte Kopie
	 */
	inline BitArray operator++(int);
	
	/**
	 * Präfix-Dekrement
	 *
	 * @return Referenz auf dekrementiertes *this
	 */
	inline BitArray & operator--();
	
	/**
	 * Postfix-Dekrement
	 *
	 * @return undekrementierte Kopie
	 */
	inline BitArray operator--(int);
	
	/**
	 * Vergleich
	 *
	 * @param rval rechtes Argument
	 * @return true, falls rechtes Argument gleich
	 */
	inline bool operator== (BitArray const & rval) const;
	
	/**
	 * Vergleich (ungleich)
	 *
	 * @param rval rechtes Argument
	 * @return true, falls rechtes Argument ungleich
	 */
	inline bool operator!= (BitArray const & rval) const
	{
		return not operator==(rval);
	}

	/**
	 * Vergleich (kleiner)
	 *
	 * @param rval rechtes Argument
	 * @return true, falls rechtes Argument größer-gleich als *this
	 */
	inline bool operator< (BitArray const & rval) const;

	/**
	 * Vergleich (kleiner-gleich)
	 *
	 * @param rval rechtes Argument
	 * @return true, falls rechtes Argument größer als *this
	 */
	inline bool operator<= (BitArray const & rval) const;

	/**
	 * Vergleich (größer)
	 *
	 * @param rval rechtes Argument
	 * @return true, falls rechtes Argument kleiner als *this
	 */
	inline bool operator> (BitArray const & rval) const
	{
		return not operator<=(rval);
	}

	/**
	 * Vergleich (größer-gleich)
	 *
	 * @param rval rechtes Argument
	 * @return true, falls rechtes Argument kleiner-gleich als *this
	 */
	inline bool operator>= (BitArray const & rval) const
	{
		return not operator<(rval);
	}

	/**
	 * Addition.
	 *
	 * @param rval zu addierendes rechtes Argument
	 */
	BitArray & operator+= (BitArray const & rval);
	
	/**
	 * Subtraktion.
	 *
	 * @param rval zu subtrahierendes rechtes Argument
	 */
	BitArray & operator-= (BitArray const & rval);
	
	/**
	 * Bitweise Disjuktion.
	 *
	 * @param rval rechtes Argument
	 */
	BitArray & operator|= (BitArray const & rval);
	
	/**
	 * Bitweise Konjunktion.
	 *
	 * @param rval rechtes Argument
	 */
	BitArray & operator&= (BitArray const & rval);
	
	/**
	 * Bitweise Konjunktion.
	 *
	 * @param rval rechtes Argument
	 */
	BitArray & operator^= (BitArray const & rval);
	
	/**
	 * Bitweises Rechtsschieben.
	 *
	 * @param shr Anzahl zu schiebender Bits
	 */
	BitArray & operator>>= (size_t shr);

	/**
	 * Bitweises Linksschieben.
	 *
	 * @param shl Anzahl zu schiebender Bits
	 */
//	BitArray & operator<<= (size_t shl);

	/**
	 * Konvertiert das BitArray in einen vorzeichenlosen
	 * 64 Bit Integer.
	 *
	 * @return vorzeichenloser 64 Bit Integer
	 */
	inline uint64_t toUnsignedInt64() const;

	/**
	 * Größenänderung.
	 *
	 * @param blength neue Größe
	 * @param sign_extend Sign-Extension durchführen
	 * @return vergrößerte Kopie von *this
	 */
	inline void resize(size_t blength, bool sign_extend);

	/**
	 * Test auf non-zero.
	 *
	 * @return true, falls Bits gesetzt sind.
	 */
	inline bool isTrue() const { return anyOnes(); }

	/**
	 * Eine Hash-Funktion.
	 *
	 * @return Hash-Wert
	 */
	inline size_t hashValue() const;

}; // class BitArray


/**
 * Addition.
 *
 * @param lval erstes Bit-Array
 * @param rval zweites Bit-Array
 * @return lval+rval
 */
inline BitArray operator+ (BitArray const & lval, BitArray const & rval);

/**
 * Subtraktion (Zweierkomplement).
 *
 * @param lval erstes Bit-Array
 * @param rval zweites Bit-Array
 * @return lval+rval
 */
inline BitArray operator- (BitArray const & lval, BitArray const & rval);

/**
 * Unäres Minus (Zweierkomplement).
 *
 * @param lval ein Bit-Array
 * @return -lval
 */
inline BitArray operator- (BitArray const & lval);

/**
 * Multiplikation (Zweierkomplement).
 *
 * @param lval erstes Bit-Array
 * @param rval zweites Bit-Array
 * @return Produkt
 */
inline BitArray operator* (BitArray const & lval, BitArray const & rval);

/**
 * Bitweise Disjunktion (OR).
 *
 * @param lval erstes Bit-Array
 * @param rval zweites Bit-Array
 * @return Bitweise Disjuktion (OR) von lval und rval
 */
inline BitArray operator| (BitArray const & lval, BitArray const & rval);

/**
 * Bitweise Konjunktion (AND).
 *
 * @param lval erstes Bit-Array
 * @param rval zweites Bit-Array
 * @return Bitweise Konjunktion (AND) von lval und rval
 */
inline BitArray operator& (BitArray const & lval, BitArray const & rval);

/**
 * Bitweise Antivalenz (XOR).
 *
 * @param lval erstes Bit-Array
 * @param rval zweites Bit-Array
 * @return Bitweise Antivalenz (XOR) von lval und rval
 */
inline BitArray operator^ (BitArray const & lval, BitArray const & rval);

/**
 * Bitweise Negation (NOT).
 *
 * @param lval ein Bit-Array
 * @return Bitweise Negation (NOT) von lval
 */
inline BitArray operator~ (BitArray const & lval);

/**
 * Bitweises Schieben nach links.
 *
 * @param lval ein Bit-Array
 * @param shl ein Integer
 * @return ein um shl Bits verschobenes Bit-Array
 */
inline BitArray operator<< (BitArray const & lval, size_t shl);

/**
 * Bitweises Schieben nach rechts.
 *
 * @param lval ein Bit-Array
 * @param shr ein Integer
 * @return ein um shr Bits verschobenes Bit-Array
 */
inline BitArray operator>> (BitArray const & lval, size_t shr);

/**
 * Eine Hash-Funktion.
 *
 * @param ba const-Referenz auf BitArray-Objekt
 * @return Hash-Wert
 */
inline size_t BitArray_hashf(BitArray const & ba) { return ba.hashValue(); }

// Inline-Implementierungen
#include "BitArray_impl.h"

#endif

