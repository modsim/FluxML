#include <cstdio>
#include "XMLFramework.h"
#include "DOMReader.h"
#include "DOMReaderImpl.h"
#include "MMDocument.h"
#include "charptr_map.h"
#include "readfile.h"
#include "fexpand.h"
#include "Stat.h"
#include "config.h"

using namespace flux::xml;

void dumpMSgroup(MGroupMS const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	int const * weights = G.getWeights();
	int w;
	printf("=== MS ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Spec: %s\n", G.getSpec());
	printf("Metabolit: %s\n", G.getMetaboliteName());
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");
	
	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
		for (w=0; weights[w] != -1; w++)
		{
			MValue const * mv = G.getMValue(*si,weights[w]);
			fASSERT(mv != 0);
			MValueMS const & mvms = static_cast< MValueMS const & >(*mv);
			fASSERT(weights[w] == mvms.getWeight());
			printf("\tweight=%i value=%g stddev=%g\n", weights[w], mv->get(), mv->getStdDev());
		}
	}
}

void dumpMSMSgroup(MGroupMSMS const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	int const * weights1 = G.getWeights1();
	int const * weights2 = G.getWeights2();

	int w;
	printf("=== MS-MS ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Spec: %s\n", G.getSpec());
	printf("Metabolit: %s\n", G.getMetaboliteName());
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");

	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
		for (w=0; weights1[w] != -1; w++)
		{
			MValue const * mv = G.getMValue(*si,weights1[w],weights2[w]);
			fASSERT(mv != 0);
			MValueMSMS const & mvmsms = static_cast< MValueMSMS const & >(*mv);
			fASSERT(weights1[w] == mvmsms.getWeight1() && weights2[w] == mvmsms.getWeight2());
			printf("\tweight=(%i,%i) value=%g stddev=%g\n", weights1[w], weights2[w], mv->get(), mv->getStdDev());
		}
	}
}

void dump1HNMRgroup(MGroup1HNMR const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	int const * posns = G.getPositions();
	int p;
	printf("=== 1H-NMR ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Spec: %s\n", G.getSpec());
	printf("Metabolit: %s\n", G.getMetaboliteName());
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");
	
	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
		for (p=0; posns[p] != -1; p++)
		{
			MValue const * mv = G.getMValue(*si,posns[p]);
			fASSERT(mv != 0);
			MValue1HNMR const & mv1hnmr = static_cast< MValue1HNMR const & >(*mv);
			fASSERT(posns[p] == mv1hnmr.getPos());
			printf("\tpos=%i value=%g stddev=%g\n", posns[p], mv->get(), mv->getStdDev());
		}
	}
}

void dump13CNMRgroup(MGroup13CNMR const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	int const * posns = G.getPositions();
	MGroup13CNMR::Type const * types = G.getNMRTypes();

	int p;
	printf("=== 13C-NMR ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Spec: %s\n", G.getSpec());
	printf("Metabolit: %s\n", G.getMetaboliteName());
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");

	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
		for (p=0; posns[p] != -1; p++)
		{
			MValue const * mv = G.getMValue(*si,posns[p],types[p]);
			fASSERT(mv != 0);		
			MValue13CNMR const & mv13cnmr = static_cast< MValue13CNMR const & >(*mv);
			fASSERT(posns[p] == mv13cnmr.getPos() && types[p] == mv13cnmr.getType());
			printf("\t(pos,type)=(%i,t%i) value=%g stddev=%g\n", posns[p], types[p], mv->get(), mv->getStdDev());
		}
	}
}

void dumpCumomerGroup(MGroupCumomer const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	printf("=== Generic Cumomer ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Spec: %s\n", G.getSpec());
	printf("Metabolit: %s\n", G.getMetaboliteName());
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");
	
	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s ", *si, *si==-1?" (=Inf)":"");
		MValue const * mv = G.getMValue(*si);
		fASSERT(mv != 0);
		printf("\tvalue=%g stddev=%g\n", mv->get(), mv->getStdDev());
	}
}

void dumpGenericGroup(MGroupGeneric const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();

	printf("=== generic ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	for (size_t r=0; r<G.getNumRows(); r++)
		printf("Spec.: {%s}\n", G.getExpression(r)->toString().c_str());
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");

	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		for (size_t r=0; r<G.getNumRows(); r++)
		{
			printf("[ROW %i]\n", int(r)+1);
			charptr_array gspecs = G.getVarNames(r);
			charptr_array::const_iterator gspec_i;
			printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
			MValue const * mv = G.getMValue(*si,r);
			fASSERT(mv != 0);
			printf("\tvalue=%g stddev=%g\n", mv->get(), mv->getStdDev());
			printf("{{{\n");
			for (gspec_i=gspecs.begin(); gspec_i!=gspecs.end(); gspec_i++)
			{
				MGroup * subG = G.getSubGroup(*gspec_i,r);
				fASSERT( subG != 0 );
				fASSERT( subG->getDim() == 1 );
				switch (subG->getType())
				{
					case MGroup::mg_MS:
						dumpMSgroup(static_cast< MGroupMS & >(*subG));
						break;
					case MGroup::mg_MSMS:
						dumpMSMSgroup(static_cast< MGroupMSMS & >(*subG));
						break;
					case MGroup::mg_1HNMR:
						dump1HNMRgroup(static_cast< MGroup1HNMR &>(*subG));
						break;
					case MGroup::mg_13CNMR:
						dump13CNMRgroup(static_cast< MGroup13CNMR &>(*subG));
						break;
					case MGroup::mg_CUMOMER:
						dumpCumomerGroup(static_cast< MGroupCumomer &>(*subG));
						break;
					default:
						fASSERT_NONREACHABLE();
				}
			}
			printf("}}}\n");
		}
	}
}

void dumpFluxGroup(MGroupFlux const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	printf("=== Flux ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Fluesse: %s\n", G.getFluxes().concat(", "));
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");
	
	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
		MValue const * mv = G.getMValue(*si);
		fASSERT(mv != 0);
		printf("\tvalue=%g stddev=%g\n", mv->get(), mv->getStdDev());
	}
}

void dumpPoolGroup(MGroupPool const & G)
{
	std::set< double >::const_iterator si;
	std::set< double > const & ts_set = G.getTimeStampSet();
	printf("=== Pool ===\n");
	printf("Group-Id: %s\n", G.getGroupId());
	printf("Pools: %s\n", G.getPools().concat(", "));
	printf("Group-Scale: %s\n", G.getScaleAuto() ? "auto" : "Skalieren mit 1");
	
	for (si=ts_set.begin(); si!=ts_set.end(); si++)
	{
		printf("timestamp=%g%s\n", *si, *si==-1?" (=Inf)":"");
		MValue const * mv = G.getMValue(*si);
		fASSERT(mv != 0);
		printf("\tvalue=%g stddev=%g\n", mv->get(), mv->getStdDev());
	}
}

int main(int argc, char **argv)
{
	DOMReader * DOMp = 0;
	MMDocument * mmdoc = 0;

	try
	{
		framework::initialize();

		DOMp = new DOMReaderImpl;

		// Den Verweis auf die DTD/XSD auf das Installationsverzeichnis
		// umbiegen
		DOMp->mapEntity("mm.dtd", FLUX_XML_DIR "/mm.dtd");
		DOMp->mapEntity("http://www.13cflux.net/xml-schema/mm.xsd", FLUX_XML_DIR "/mm.xsd");
		DOMp->mapEntity("http://www.13cflux.net/xml-schema/fluxml.xsd", FLUX_XML_DIR "/fluxml.xsd");
		DOMp->mapEntity("http://www.w3.org/Math/XMLSchema/mathml2/mathml2.xsd",
			FLUX_XML_DIR "/mathml2/mathml2.xsd");

		DOMp->parseFromFile("test/beispiel.mm"/*"test/beispiel_stat.mm"*/);
		mmdoc = new MMDocument(DOMp->getDOMDocument());
	}
	catch (XMLException & e)
	{
		printf("Exception (%i,%i): %s\n",
			e.getXMLLine(),
			e.getXMLColumn(),
			(char const*)e
		);
		if (mmdoc) delete mmdoc;
		if (DOMp) delete DOMp;
		return EXIT_FAILURE;
	}

	charptr_array gnames = mmdoc->getGroupNames();
	charptr_array::const_iterator k;
	for (k=gnames.begin(); k!=gnames.end(); k++)
	{
		MGroup * G = mmdoc->getGroupByName(*k);
		fASSERT( G != 0 );

		switch (G->getType())
		{
		case MGroup::mg_MS:
			dumpMSgroup(static_cast< MGroupMS & >(*G));
			break;
		case MGroup::mg_MSMS:
			dumpMSMSgroup(static_cast< MGroupMSMS & >(*G));
			break;
		case MGroup::mg_1HNMR:
			dump1HNMRgroup(static_cast< MGroup1HNMR &>(*G));
			break;
		case MGroup::mg_13CNMR:
			dump13CNMRgroup(static_cast< MGroup13CNMR &>(*G));
			break;
		case MGroup::mg_GENERIC:
			dumpGenericGroup(static_cast< MGroupGeneric &>(*G));
			break;
		case MGroup::mg_FLUX:
			dumpFluxGroup(static_cast< MGroupFlux & >(*G));
			break;
		case MGroup::mg_POOL:
			dumpPoolGroup(static_cast< MGroupPool & >(*G));
			break;
		default:
			fASSERT_NONREACHABLE();
		}
	}

	printf("alle messzeitpunkte:\n");
	int i,size;
	double const * ts = mmdoc->getTimeStamps(size);
	for (i=0; i<size; i++)
		printf("%2i %f\n", i+1, ts[i]);

	delete mmdoc;
	delete DOMp;
	framework::terminate();
	return EXIT_SUCCESS;
}

