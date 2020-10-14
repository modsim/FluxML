#include <cstdio>
#include <string>
#include <list>

#include "Error.h"
#include "NLgetopt.h"

// Kern-Bibliothek
#include "FluxML.h"

// XML / DOM
#include "XMLFramework.h"
#include "XMLException.h"
#include "DOMReader.h"
#include "DOMReaderImpl.h"
#include "DOMWriter.h"
#include "DOMWriterImpl.h"
#include "UnicodeTools.h"

using namespace flux;

struct FluxMLlintParams
{
	char const * in;
	char const * out;
	bool list_cfgs;
	charptr_array logpublishers;

	FluxMLlintParams()
		: in(0), out(0), list_cfgs(false)
	{
		// Logging auf stderr bis der User Genaueres
		// spezifiziert:
		logpublishers.add("fd:2");
	}
};

void putHelp()
{
/** <INST> **/       
        // Temporärer Log-Manager:
	LogManager LM(logNOTICE);
	LM.addPublisher(stderr_log);
        
	fWARNINGf(LM,
                "******************************************************************************\n"
                "******************************************************************************\n"
                "**  FluxML 'lint' Tool,    (c) 2010 Michael Weitzel (stationary MFA)        **\n"
                "**                             2015 Salah Azzouzi (non-stationary MFA)      **\n"
                "******************************************************************************");
        fprintf(stderr,
                "Syntax: fmllint ");
        fNOTICEf(LM, "<parameters>\n");
        fNOTICEf(LM, " # Standard Parameters:");          
	fprintf(stderr,
		"  -i/--in <URI>             fluxML input file (stdin if omitted)\n"
		"  -o/--out <file>           fluxML output file (optional,\"-\" for stdout)\n"
		"  -L/--list                 list fluxML configuration names and exit\n"
		"  -l/--log A,B,...          logging destinations (specify as first param.!)\n"
		"  -v/--verbose <number>     verbosity 0..10 (default: 5)\n\n"
		);

}

static void parseCommandLine(
	int argc,
	char ** argv,
	FluxMLlintParams & cfg
	)
{
	static option long_options[] =
	{
		{"in",		1, 0, 'i'},
		{"out",		1, 0, 'o'},
		{"list",	0, 0, 'L'},
		{"log",		1, 0, 'l'},
		{"verbose",	1, 0, 'v'},
		{"help",	0, 0, 'h'},
		{0,0,0,0}
	};

	// Temporärer Log-Manager:
	LogManager LM(logWARNING);
	LM.addPublisher(stderr_log);

	optind = 1;
	for (;;)
	{
		char * endptr = 0;
		long int loglevel;
		int oidx = 0;
		int c = getopt_long_newlib(argc,argv,"i:o:Ll:v:h", long_options, &oidx);
		if (c == -1)
			break;
		
		switch (c)
		{
		case 0:
			fASSERT_NONREACHABLE();
			break;
		case 'i':
			cfg.in = optarg;
			break;
		case 'o':
			cfg.out = optarg;
			break;
		case 'L':
			cfg.list_cfgs = true;
			break;
		case 'l':
			cfg.logpublishers = charptr_array::split(optarg,",");
			break;
		case 'v':
			errno = 0;
			loglevel = strtol(optarg,&endptr,10);
			if (endptr == optarg or errno != 0 or loglevel<0 or loglevel>10)
			{
				fERRORf(LM,"invalid log level");
				exit(EXIT_FAILURE);
			}
			SETLOGLEVEL((LogLevel)loglevel);
			break;
		case 'h':
			putHelp();
			exit(EXIT_SUCCESS);
		case '?':
			fERRORf(LM,"unknown command line switch.");
			exit(EXIT_FAILURE);
		}
	}

	PUBLISHLOG(cfg.logpublishers.get());

	if (cfg.in != 0 and cfg.out != 0 and strcmp(cfg.in,cfg.out) == 0)
	{
		fERROR("refusing to overwrite input file (\"%s\" for -i, -o)",
			cfg.in);
		exit(EXIT_FAILURE);
	}

	if (optind < argc)
	{
		fWARNING("ignoring extra arguments:");
		while (optind < argc)
			fWARNING("%s", argv[optind++]);
	}
}

int safe_main(int,char**);

int main(int argc, char **argv)
{
	try
	{
		return safe_main(argc,argv);
	}
	catch (AssertionError & E)
	{
		fERROR("assertion failed please report this [%s]",
			E.toString());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int safe_main(int argc, char **argv)
{
	xml::DOMReader * reader = 0;
	xml::FluxMLDocument * fml = 0;
	FluxMLlintParams params;

	SETLOGLEVEL(logINFO); // Default: LogLevel auf logINFO

	parseCommandLine(argc, argv, params);

	try
	{
		xml::framework::initialize();
	}
	catch(xml::XMLException & e)
	{
		fERROR("exception: %s", (char const*)e);
		return EXIT_FAILURE;
	}
	
	try
	{
		reader = new xml::DOMReaderImpl;
		reader->mapEntity("https://www.13cflux.net/fluxml",
				FLUX_XML_DIR "/fluxml.xsd");
		reader->mapEntity("https://www.w3.org/Math/XMLSchema/mathml2/mathml2.xsd",
				FLUX_XML_DIR "/mathml2/mathml2.xsd");
		reader->setResolveXInclude(true);

		if (params.in == 0)
			reader->parseFromStdIn();
		else
			reader->parseFromURI(params.in);

		fml = new xml::FluxMLDocument(reader->getDOMDocument());
	}
	catch (xml::XMLException & e)
	{
		fERROR("FluxML parsing error: %s", (char const*)e);
		delete reader;
		delete fml;
		xml::framework::terminate();
		return EXIT_FAILURE;
	}

	if (params.list_cfgs)
	{
		charptr_array cfgs = fml->getConfigurationNames();
		charptr_array::const_iterator ci;
		for (ci=cfgs.begin(); ci!=cfgs.end(); ++ci)
		{
			if (strcmp(*ci,"__root__") == 0)
				continue;
			printf("%s\n", *ci);
		}
		delete reader;
		delete fml;
		xml::framework::terminate();
		return EXIT_SUCCESS;
	}

	if (params.out)
	{
		FILE * outF = stdout;
		if (strcmp(params.out,"-") != 0)
		{
			errno = 0;
			if ((outF = fopen(params.out,"w")) == 0)
			{
				fERROR("opening of output file \"%s\" failed: %s",
					params.out, strerror(errno));
				delete reader;
				delete fml;
				xml::framework::terminate();
				return EXIT_FAILURE;
			}
		}

		xml::DOMWriter * writer = new xml::DOMWriterImpl(
			reader->getDOMImplementation(),
			reader->getDOMDocument()
			);
		writer->setPrettyPrint(true);
		writer->writeToStream(outF);
		delete writer;

		if (outF != stdout)
			fclose(outF);
	}
	delete reader;
	delete fml;
	xml::framework::terminate();
	return EXIT_SUCCESS;
}

