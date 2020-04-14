#include <iostream>

#include <fluxml/FluxML.h>
#include <fluxml/MGroup.h>
#include <fluxml/MMDocument.h>
#include <fluxml/XMLFramework.h>

using namespace std;
using namespace flux;

std::string make_section_head(const std::string& section_name)
{
	std::string line(80, '=');
	std::string fill((line.length() - section_name.length()) / 2, ' ');
	std::string header = line + "\n" + fill + section_name + "\n" + line;
	return header;
}

int main(int argc, char* argv[])
{

	/** check number of specified arguments **/
	if (argc != 2)
	{
		cout << endl << "Usage: readFluxML filename" << endl << endl;
		return 1;
	}
	
	/** make sure error Messages go somewhere **/
  PUBLISHLOG(stderr_log);

	const char* filename = argv[1];
	xml::FluxMLDocument * fml = 0;
  try
  {
    fml = new xml::FluxMLDocument(filename);
  }
  catch (xml::XMLException& e)
  {
    cerr << "Problem with opening File " << filename << ":" << endl << e.toString() << endl;
    exit(1);
  }
	cout << make_section_head("metabolites") << endl;
	/** print network metabolites **/
	charptr_map<data::Pool*> * poolMap = fml->getPoolMap();
	charptr_map<data::Pool*>::const_iterator pi;
	for (pi = poolMap->begin(); pi != poolMap->end(); ++pi)
	{
		data::Pool* pool = pi->value;
		cout << "pool: " << pool->getName() << "\t atoms: "
				<< pool->getNumAtoms() << endl;
	}

	/** print network reactions **/
	cout << endl << make_section_head("reactions") << endl;
	const std::list<data::IsoReaction*>* reactionList = fml->getReactions();
	std::list<data::IsoReaction*>::const_iterator ri;
	for (ri = reactionList->begin(); ri != reactionList->end(); ++ri)
	{
		data::IsoReaction* reaction = *ri;
		cout << "reaction: " << reaction->getName() << "\t atoms: "
				<< reaction->getNumAtoms() << endl;

		/** print reaction educts **/
		const std::list<data::IsoReaction::Isotopomer*> educts =
				reaction->getEducts();
		std::list<data::IsoReaction::Isotopomer*>::const_iterator ei;
		for (ei = educts.begin(); ei != educts.end(); ++ei)
			cout << "educt-name: " << (*ei)->name << "\t educt-cfg: "
					<< (*ei)->atom_cfg << endl;

		/** print reaction products **/
		const std::list<data::IsoReaction::Isotopomer*> products =
				reaction->getProducts();
		for (ei = products.begin(); ei != products.end(); ++ei)
			cout << "educt-name: " << (*ei)->name << "\t educt-cfg: "
					<< (*ei)->atom_cfg << endl;
	}

	cout << endl << make_section_head("configuration default") << endl;
	/** select "default" configuration**/
	data::Configuration * cfg = fml->getConfiguration("default");
	if (cfg == 0)
	{
		fWARNING("aborting: \"default\" configuration not found.");
		delete fml;
		xml::framework::terminate();
		return EXIT_FAILURE;
	}

	cout << endl << make_section_head("constraints") << endl;
	/** print model constraints **/
	charptr_array cons;
	charptr_array::const_iterator consi;

	/** netflux constraints **/
	cons = cfg->getConstraintFluxesNet();
	for (consi = cons.begin(); consi != cons.end(); ++consi)
		cout << "net: " << *consi << endl;

	/** xchflux constraints **/
	cons = cfg->getConstraintFluxesXch();
	for (consi = cons.begin(); consi != cons.end(); ++consi)
		cout << "xch: " << *consi << endl;

	/** poolsize constraints **/
	cons = cfg->getConstraintPools();
	for (consi = cons.begin(); consi != cons.end(); ++consi)
		cout << "pool: " << *consi << endl;

	cout << endl << make_section_head("input tracers") << endl;
	/** print tracer specification **/
	list<data::InputPool*> const & IPL = cfg->getInputPools();
	list<data::InputPool*>::const_iterator ip;
	cout << "tracers: " << endl;
	for (ip = IPL.begin(); ip != IPL.end(); ++ip)
	{
		data::InputPool & I = *(*ip);
		cout << "pool: " << I.getName() << "\t cost: " << I.getCost() << endl;
	}

	cout << endl << make_section_head("measurement data") << endl;
	/** print experimental data **/
	xml::MMDocument * mmdoc = 0;
	mmdoc = cfg->getMMDocument();

	charptr_array mgroup_names = mmdoc->getGroupNames();
	charptr_array::const_iterator mgi;

	for (mgi = mgroup_names.begin(); mgi != mgroup_names.end(); mgi++)
	{
		xml::MGroup * mgroups = mmdoc->getGroupByName(*mgi);
		cout << "group-id: " << mgroups->getGroupId() << "\t group-spec: "
				<< mgroups->getSpec() << endl;
	}

	/** free allocated memory **/
	delete fml;
	xml::framework::terminate();
	return EXIT_SUCCESS;
}
