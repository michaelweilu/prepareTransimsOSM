#include <float.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <list>
#include <stdio.h>
#include <map>
#include <fstream>
#include <vector>
#include <set>
using namespace std;

#define DEBUG_GLOBAL
#include "DebugTools/DebugTools.h"
#include "GISSystem/GISSystem.h"
#include "Network/gis.h"
using namespace GIS;

#include <proj_api.h>
#include "OSMHighway.h"
#include "LandScanUSAPop.h"
#include "StandardPolygon.h"
#include "PrepareTransims.h"

int TheMethod;

char *ProjArgs[] = { (char *) "proj=utm", (char *) "zone=33N", (char *) "ellps=WGS84" };

string get_shape_file_name(string aName, string aType = "", string aPath = "")
{
	if (aType == "")
		return aPath + "/" + aName + "/" + aName;
	else if (aPath == "")
		return aName + aType;
	else
		return aPath + "/" + aName + aType + "/" + aName + aType;
}

void
usage(int argc, char * argv[])
{
    if (argc < 4)
    {
	    GIS_ERROR("usage: uGenerateStudyAreaTextFile <project> <streets> <pop> <output>");
		exit(1);
	}
}

int main(int argc, char ** argv) 
{
	DEBUG_HEADER;

	usage(argc, argv);

	GIS_START_TIME

	OGRRegisterAll();

    TransimsOutput  theTransimsOutput;

    if (!(theTransimsOutput.pj = pj_init(3, ProjArgs)))
    {
        cout << "ERROR: can not initialize the projection library" << endl;
        exit(1);
    }

    string  theProject              = argv[1];
    string  thePolygonFileName      = argv[2];
    string  theStreetFileName       = argv[3];
    string  thePopulationFileName   = argv[4];
    string  theOutputDir            = argv[5];
    string  theType                 = argv[6];

    TheMethod = GIS::System::Convert::toInt (theType);
    DEBUG_INFO("ploygon = %s",  thePolygonFileName.c_str());
    DEBUG_INFO("streets = %s",  theStreetFileName.c_str());
    DEBUG_INFO("pop = %s",      thePopulationFileName.c_str());
    DEBUG_INFO("output = %s",   theOutputDir.c_str());
//    DEBUG_INFO("type = %s",   theType == "1" ? "Super Node" : "External Nodes");
    DEBUG_INFO("type = %s",   theType == "1" ? "shortest distance" : "highway bias ");

    string thePolygonFileFullName = theProject + "/" + thePolygonFileName;

    StandardPolygons * thePolygons = new StandardPolygons(thePolygonFileFullName.c_str());

    thePolygons->read();

    string theStreetFileFullName = theProject + "/" + theStreetFileName;

    OSMHighwayLinks * theLinks = new OSMHighwayLinks(
            theStreetFileFullName.c_str());
    theLinks->read();

    std::string thePopulationFileFullName = theProject + "/" + thePopulationFileName;
    LandScanUSAPops * thePops = new LandScanUSAPops(thePopulationFileFullName.c_str());
    thePops->read();

    for (unsigned i = 0; i < thePops->size(); ++i)
    {
        thePops->at(i)->nodeID = i + 1;
    }

    theTransimsOutput.outputDir         = theOutputDir;
    theTransimsOutput.links             = theLinks;
    theTransimsOutput.polygons          = thePolygons;
    theTransimsOutput.populations       = thePops;

    theTransimsOutput.find_center();

    theTransimsOutput.build_network_nodes();
    theTransimsOutput.find_boundary_nodes();
    theTransimsOutput.maxLinkID = theLinks->size();
    theTransimsOutput.build_network_external_nodes_and_links();

    theTransimsOutput.build_network_shape_file_nodes_and_links();

    OSMHighwayNetwork * theNetwork = new OSMHighwayNetwork();

    theNetwork->nodes = theTransimsOutput.nodes;
    theNetwork->links = theTransimsOutput.links;
    theNetwork->check_topology();

    theTransimsOutput.output_network_input_zone_text_file();
    theTransimsOutput.output_network_input_node_files();
    theTransimsOutput.output_network_input_link_files();
    theTransimsOutput.output_network_input_shape_text_file();
    theTransimsOutput.output_network_trip_table_text_file(theType);

	GIS_END_TIME

	exit (0);
}
