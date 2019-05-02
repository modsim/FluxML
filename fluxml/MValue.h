#ifndef MVALUE_H
#define MVALUE_H

extern "C"
{
#include <stdint.h>
}
#include "Combinations.h"
#include "cstringtools.h"

namespace flux {
namespace xml {

/**
 * Abstrakte Basisklasse zur Abbildung von Messwerten.
 *
 * @author Michael Weitzel <info@13cflux.net>
 */
class MValue
{
protected:
	/** Bezeichner (XML) des Messwerts */
	char * id_;
	/** Timestamp des Messwerts */
	double timestamp_;
	/** Standardabweichung */
	double stddev_;
	/** unverrauschter Messwert */
	double orig_mvalue_;
	/** möglicherweise verrauschter Messwert */
	double mvalue_;

public:
	inline MValue(
		char const * id,
		double ts,
		double sd,
		double mv
		) : id_(strdup_alloc(id)),
		    timestamp_(ts),
		    stddev_(sd),
		    orig_mvalue_(mv),
		    mvalue_(mv) { }

	inline MValue(
		MValue const & copy
		) : id_(strdup_alloc(copy.id_)),
		    timestamp_(copy.timestamp_),
		    stddev_(copy.stddev_),
		    orig_mvalue_(copy.orig_mvalue_),
		    mvalue_(copy.mvalue_) { }
	
	// ein "Virtueller Constructor"
	// in den abgeleiteten Klassen wird diese Methode mittels
	// "Covariant Return Types" überschrieben.
	virtual MValue * clone() const = 0;

	inline virtual ~MValue() { delete[] id_; }

public:
	inline char const * getGroupId() const { return id_; }

	inline double getTimeStamp() const { return timestamp_; }

	inline double getStdDev() const { return stddev_; }

	inline double get() const { return mvalue_; }

	inline void set(double mvalue) { mvalue_ = mvalue; }

	inline uint32_t computeCheckSum(uint32_t crc, int crc_scope) const
	{
		if (crc_scope & CRC_CFG_MEAS_DATA)
		{
			crc = update_crc32(&timestamp_,sizeof(timestamp_),crc);
			crc = update_crc32(&stddev_,sizeof(stddev_),crc);
			crc = update_crc32(&orig_mvalue_,sizeof(orig_mvalue_),crc);
		}
		return crc;
	}

	inline bool operator<(MValue const & Rval) const
	{
		return timestamp_ < Rval.timestamp_;
	}

};

class MValueMS : public MValue
{
protected:
	int weight_;

public:
	inline MValueMS(
		char const * id,
		double ts,
		double sd,
		double mv,
		int w
		) : MValue(id,ts,sd,mv),
		    weight_(w) { }

	inline MValueMS(
		MValueMS const & copy
		) : MValue(copy),
		    weight_(copy.weight_) { }
	
	inline virtual MValueMS * clone() const
	{
		return new MValueMS(*this);
	}

public:
	inline int getWeight() const { return weight_; }

};

class MValueMIMS : public MValue
{
protected:
	std::vector<int> weights_;

public:
	inline MValueMIMS(
		char const * id,
		double ts,
		double sd,
		double mv,
		std::vector<int> weights
		) : MValue(id,ts,sd,mv)
                {
                    weights_.clear();
                    for(std::vector<int>::const_iterator w=weights.begin();
                            w!= weights.end(); w++)
                       weights_.push_back(*w);
                }

	inline MValueMIMS(
		MValueMIMS const & copy
		) : MValue(copy),
		    weights_(copy.weights_) 
                {
                    weights_.clear();
                    for(std::vector<int>::const_iterator w=copy.weights_.begin();
                            w!= copy.weights_.end(); w++)
                       weights_.push_back(*w);
                }
	
	inline virtual MValueMIMS * clone() const
	{
		return new MValueMIMS(*this);
	}

public:
	inline std::vector<int> const & getWeight() const { return weights_; }

};

class MValueMSMS : public MValue
{
protected:
	int weight1_;
	int weight2_;

public:
	inline MValueMSMS(
		char const * id,
		double ts,
		double sd,
		double mv,
		int w1,
		int w2
		) : MValue(id,ts,sd,mv),
		    weight1_(w1),
		    weight2_(w2) { }

	inline MValueMSMS(
		MValueMSMS const & copy
		) : MValue(copy),
		    weight1_(copy.weight1_),
		    weight2_(copy.weight2_) { }

	inline virtual MValueMSMS * clone() const
	{
		return new MValueMSMS(*this);
	}

public:
	inline int getWeight1() const { return weight1_; }

	inline int getWeight2() const { return weight2_; }

};

class MValueNMR : public MValue
{
protected:
	int pos_;

public:
	inline MValueNMR(
		char const * id,
		double ts,
		double sd,
		double mv,
		int pos
		) : MValue(id,ts,sd,mv),
		    pos_(pos) { }

	inline MValueNMR(
		MValueNMR const & copy
		) : MValue(copy),
		    pos_(copy.pos_) { }

	inline virtual MValueNMR * clone() const
	{
		return new MValueNMR(*this);
	}

public:
	inline int getPos() const { return pos_; }

};

typedef MValueNMR MValue1HNMR;

class MValue13CNMR : public MValueNMR
{
public:
	enum NMR13CType { S=1, DL=2, DR=3, DD=4, T=5 };

protected:
	NMR13CType type_;

public:
	inline MValue13CNMR(
		char const * id,
		double ts,
		double sd,
		double mv,
		int pos,
		int type
		) : MValueNMR(id,ts,sd,mv,pos)
	{
		fASSERT(type >= 1 && type <= 5);
		type_ = NMR13CType(type);
	}

	inline MValue13CNMR(
		MValue13CNMR const & copy
		) : MValueNMR(copy),
		    type_(copy.type_) { }

	inline virtual MValue13CNMR * clone() const
	{
		return new MValue13CNMR(*this);
	}

public:
	inline NMR13CType getType() const { return type_; }

};

class MValueGeneric : public MValue
{
protected:
	int row_;

public:
	inline MValueGeneric(
		char const * id,
		double ts,
		double sd,
		double mv,
		size_t row
		) : MValue(id,ts,sd,mv),
		    row_(row) { }

	inline MValueGeneric(
		MValueGeneric const & copy
		) : MValue(copy),
		    row_(copy.row_) { }

	inline virtual MValueGeneric * clone() const
	{
		return new MValueGeneric(*this);
	}

public:
	inline int getRow() const { return row_; }

};

class MValueSimple : public MValue
{
public:
	inline MValueSimple(
		char const * id,
		double ts,
		double sd,
		double mv
		) : MValue(id,ts,sd,mv) { }

	inline MValueSimple(
		MValueSimple const & copy
		) : MValue(copy) { }

	inline virtual MValueSimple * clone() const
	{
		return new MValueSimple(*this); 
	}

};

typedef MValueSimple MValueFlux;
typedef MValueSimple MValuePool;
typedef MValueSimple MValueCumomer;

} // namespace flux::xml
} // namespace flux

#endif

