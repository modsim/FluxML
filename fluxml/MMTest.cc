#include "fhash_map.h"
#include "BitArray.h"
#include "Error.h"
#include "MGroup.h"
#include "ExprTree.h"

using namespace flux::xml;
using namespace flux::la;
using namespace flux::cumu;

struct isotopomer
{
	char const * bits;
	double fraction;
};

fhash_map< BitArray,CumulativeScalar< double >,BitArray_hashf >
	collect(isotopomer * isos)
{
	fhash_map< BitArray,CumulativeScalar< double >,BitArray_hashf > x;
	
	size_t i = 0;
	while (isos[i].bits)
	{
		BitArray * idx = BitArray::parseBin(isos[i].bits);
		if (idx == 0)
		{
			fERROR("error parsing index '%s'", isos[i].bits);
			exit(1);
		}

		x[*idx] = isos[i].fraction;
		delete idx;
		++i;
	}
	return x;
}

int main()
{
	PUBLISHLOG(stderr_log);

	char spec[] = "gonzo#M0,1,2";

	isotopomer isos[] = {
		{ "xxx", 1   }, // 0-Cumomer muss immmer drin sein
		{ "1xx", 0.1 },
		{ "x1x", 0.2 },
		{ "11x", 0.3 },
		{ "xx1", 0.4 },
		{ "1x1", 0.5 },
		{ "x11", 0.6 },
		{ "111", 0.7 },
		{ 0, 0. }
		};

	fhash_map< BitArray,CumulativeScalar< double >,BitArray_hashf > x
		= collect(isos);

	fhash_map< BitArray,CumulativeScalar< double >,BitArray_hashf >::iterator k;
	for (k=x.begin(); k!=x.end(); ++k)
		printf("%s => %.15f\n", k->key.toString('x','1'), double(k->value));
	
	try
	{
		int state = 0;
		MetaboliteMGroup * G = MetaboliteMGroup::parseSpec(spec,&state);
		//MGroupGeneric * G = MGroupGeneric::parseSpec(spec);
		printf("state = %i G=%p\n", state, G);
		if (G)
		{
			G->setNumAtoms(3);
			printf("metabolit=%s\n", G->getMetaboliteName());
			printf("dim=%i\n", int(G->getDim()));

			double gs;
			GVector< double > y_sim = G->evaluate(
					-1,	// stationär
					x,	// Cumomere
					true,	// automatische Skalierung
					gs	// berechneter Group-Scale-Faktor
					);

			for (size_t i=0; i<y_sim.dim(); ++i)
				printf("%s = %.15f\n", G->getSpec(i), y_sim(i));

			delete G;
		}
	}
	catch (XMLException & e)
	{
		printf("exc: %s\n", (char const*)e);
	}
}

