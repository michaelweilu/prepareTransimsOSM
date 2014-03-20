
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

#include "DebugTools/DebugTools.h"
#include "GISSystem/GISSystem.h"
#include "Network/gis.h"
using namespace GIS;

#include <proj_api.h>

#include "LandScanUSAPop.h"
#include "ShapeFileHelper.h"
#include "StandardPolygon.h"
#include "PrepareTransims.h"

extern int TheMethod;

projUV
TransimsOutput::projection(double aX, double aY)
{
    projUV              p;

    p.u = aX * DEG_TO_RAD;
    p.v = aY * DEG_TO_RAD;
    p = pj_fwd(p, pj);

    return p;
}

int
TransimsOutput::rename_node()
{
    return (populations->size() + numOfExternalNodes);
}

bool
TransimsOutput::is_boundary_exit_node(int64_t aNodeID)
{
    bool isBoundaryExitNode = false;

    for (unsigned j = 0; j < links->size(); ++j)
    {
        if (((links->at(j)->fromNode == aNodeID) ||
             (links->at(j)->toNode == aNodeID)) && 
             (links->at(j)->oneway == 0)) 
        {
            isBoundaryExitNode = true;
            break;
        }
        
        if ((links->at(j)->toNode == aNodeID && links->at(j)->oneway == 1) || (links->at(j)->fromNode == aNodeID && links->at(j)->oneway == 1))
        {
            isBoundaryExitNode = true;
            break;
        }
    }

    return isBoundaryExitNode;
}

void
TransimsOutput::build_network_shape_file_nodes_and_links()
{
    DEBUG_FUNCTION_BEGIN;

    nodes = new OSMHighwayNodes();

    map<std::string, int>::iterator it;
	for (it  = nodeMap.begin();
         it != nodeMap.end();
         ++it)
	{
        std::string theIndex = it->first;
        vector<std::string>& theItems = GIS::System::String::split(theIndex, ':');
        if (theItems.size() < 2)
        {
            GIS_ERROR("format error %s", theIndex.c_str());
            exit(1);
        }
        double theLon = GIS::System::Convert::toDouble(theItems[0]);
        double theLat = GIS::System::Convert::toDouble(theItems[1]);
        delete &theItems;

        OSMHighwayNode * theNode = new OSMHighwayNode();

		strcpy(theNode->id, Object::build_node_id(it->second, (char *) "HH"));

        int theNodeID = it->second;

        bool isExternal = false;
        if (theNodeID < 0)
        {
            isExternal = true;
        }

        theNodeID += populations->size() + numOfExternalNodes;

		strcpy(theNode->id, Object::build_node_id(theNodeID, (char *) "HH"));
        theNode->nodeID     = theNodeID;
        theNode->x          = theLon;
        theNode->y          = theLat;
        theNode->isExternal = isExternal;

        nodes->data->push_back(theNode);
    }

    nodes->sort();

    std::string theGISDataHome = getenv ("GISDATAHOME");
    std::string theOSMNodeFileName = theGISDataHome + "/ORTS/pdata/Beijing_CN/bjNodes";
    nodes->dump_to_shape_file(theOSMNodeFileName);
    std::string theOSMLinkFileName = theGISDataHome + "/ORTS/pdata/Beijing_CN/bjLinks";
    links->dump_to_shape_file(theOSMLinkFileName);

    for (int i = 0; i < links->size(); ++i)
    {
        links->at(i)->fromNode += rename_node();
        links->at(i)->toNode += rename_node();
    }

    DEBUG_INFO("%d shape nodes built", nodes->size());

    DEBUG_FUNCTION_END;
}

void
TransimsOutput::build_network_super_node_and_links()
{
    DEBUG_FUNCTION_BEGIN;

GIS_ERROR("not implement yet");
exit(1);
}

void
TransimsOutput::build_network_external_nodes_and_links()
{
    DEBUG_FUNCTION_BEGIN;

    int theCount = 0;
    for (unsigned i = 0; i < externalNodeVector.size(); ++i)
    {
        int64_t theNodeID               = externalNodeVector[i].nodeID;

        bool    isBoundaryExitNode      = is_boundary_exit_node(theNodeID);

        if (isBoundaryExitNode)
        {
            double theX = xCenter + 1.01 * (externalNodeVector[i].x - xCenter);
            double theY = yCenter + 1.01 * (externalNodeVector[i].y - yCenter);

            char theBuffer[2048];
            sprintf(theBuffer, "%020.12lf:%020.12lf", theX, theY);
            string theID = theBuffer;
            nodeMap[theID] = --theCount;

            OSMHighwayLink        * theLink   = new OSMHighwayLink;
            theLink->linkID                 = maxLinkID++;
            theLink->fromNode               = externalNodeVector[i].nodeID;
            theLink->toNode                 = theCount;
            theLink->fcc                    = "A00";
            theLink->distance               = 0;
            theLink->oneway                 = 1;
            theLink->shape                  = new ShapeVertexs;
        
            theLink->shape->push_back(ShapeVertex(0, externalNodeVector[i].x, externalNodeVector[i].y, 0.0));
            theLink->shape->push_back(ShapeVertex(1, theX, theY, 0.0));
            links->data->push_back(theLink);
        }
    }

    numOfExternalNodes = -theCount;

    DEBUG_INFO("total %d external nodes build", numOfExternalNodes); 
    DEBUG_INFO ("links = %d", links->size());

    DEBUG_FUNCTION_END;
}

void
TransimsOutput::output_network_input_node_files()
{
    DEBUG_FUNCTION_BEGIN;

	ofstream oFile;
	string theFileName = outputDir + "/Input_Node.txt";
    DEBUG_INFO("write to %s", theFileName.c_str());
	oFile.open(theFileName.c_str());
	if (oFile.fail())
	{
		cerr << "open " << theFileName << " failed" << endl;
		exit (1);
	}

	GIS_INFO("The number of nodes %d ", nodeMap.size());

	oFile.precision(18);
	oFile << "NODE\tX_COORD\tY_COORD\tZ_COORD\tNOTES" << endl;

    double theAltitude = 0.0;

    if (!nodes)
    {
        GIS_ERROR("LOGIC nodes is empty");
        exit(1);
    }

    for (unsigned i = 0; i < nodes->size(); ++i)
    {
		projUV	p = projection(nodes->at(i)->x, nodes->at(i)->y);
		oFile
			<< nodes->at(i)->nodeID + 1 << "\t"
			<< p.u << "\t"
			<< p.v << "\t"
			<< theAltitude << "\t"
			<< " "
			<< endl;
	}

	oFile.close();

    DEBUG_FUNCTION_END;
}

void 
TransimsOutput::output_network_input_link_files()
{
    DEBUG_FUNCTION_BEGIN;

	ofstream oFile;
    std::string theFileName = outputDir + "/Input_Link.txt";
    DEBUG_INFO("write to %s", theFileName.c_str());
	oFile.open(theFileName.c_str());
	if (oFile.fail())
	{
		cerr << "open " << theFileName << " failed" << endl;
		exit (1);
	}


	oFile.precision(18);
	oFile << "LINK\tSTREET\tANODE\tBNODE\tLENGTH\tTYPE\tLANES_AB\tFSPD_AB\tCAP_AB\tLANES_BA\tFSPD_BA\tCAP_BA\tUSE\tNOTES" << endl;

	for (unsigned int i = 0; i < links->size(); ++i)
	{
        if (!links->at(i)->shape)
            continue;

        double theLength = 0.0;
        for (unsigned int j = 1; j < links->at(i)->shape->size(); ++j)
        {
            double theX1 = links->at(i)->shape->at(j - 1)[0];
            double theY1 = links->at(i)->shape->at(j - 1)[1];
            double theX2 = links->at(i)->shape->at(j)[0];
            double theY2 = links->at(i)->shape->at(j)[1];

            double theMile = GIS::System::Geometry::great_circle_distance( theX1, theY1, theX2, theY2);
            double theMeter = theMile * METER_PER_MILE;
            theLength += theMeter;
        }

        string  theType;

		int     theABCapacity = 0;
        int     theABNumOfLanes = 0;
        double  theABSpeed = 0.0;

        int     theBACapacity = 0;
        int     theBANumOfLanes = 0;
        double  theBASpeed = 0.0;
	
        if (links->at(i)->fcc < "A01")
        {
	    	theType         = "EXTERNAL";
            theLength       = 7.5;
	        theABSpeed      = 100;
	    	theABCapacity   = 10000;
	    	theABNumOfLanes = 100;
	    	theBASpeed      = 0;
	    	theBACapacity   = 0;
	    	theBANumOfLanes = 0;
        }
        else if (links->at(i)->fcc < "A20")
        {
            if (links->at(i)->oneway == 0)
            {
		        theType         = "FREEWAY";
		        theABSpeed      = 37.5;
		        theABCapacity   = 8000;
		        theABNumOfLanes = 4;
		        theBASpeed      = 37.5;
		        theBACapacity   = 8000;
		        theBANumOfLanes = 4;
            }
            else if (links->at(i)->oneway == 1)
            {
		        theType         = "FREEWAY";
		        theABSpeed      = 37.5;
		        theABCapacity   = 8000;
		        theABNumOfLanes = 4;
		        theBASpeed      = 37.5;
		        theBACapacity   = 8000;
		        theBANumOfLanes = 4;
            }
            else
            {
                GIS_ERROR("LOGIC:");
                exit(1);
            }
        }
	    else if (links->at(i)->fcc < "A30")
	    {
            if (links->at(i)->oneway == 0)
            {
		        theType         = "EXPRESSWAY";
		        theABSpeed      = 30;
		        theABCapacity   = 4200;
		        theABNumOfLanes = 3;
		        theBASpeed      = 30;
		        theBACapacity   = 4200;
		        theBANumOfLanes = 3;
            }
            else if (links->at(i)->oneway == 1)
            {
		        theType         = "EXPRESSWAY";
		        theABSpeed      = 30;
		        theABCapacity   = 4200;
		        theABNumOfLanes = 3;
		        theBASpeed      = 30;
		        theBACapacity   = 4200;
		        theBANumOfLanes = 3;
            }
            else
            {
                GIS_ERROR("LOGIC: oneway = %d", links->at(i)->oneway);
                exit(1);
            }
	    }
	    else if (links->at(i)->fcc < "A40")
	    {
            if (links->at(i)->oneway == 0)
            {
	    	    theType         = "PRINCIPAL";
	    	    theABSpeed      = 22.5;
	    	    theABCapacity   = 1600;
	    	    theABNumOfLanes = 2;
	    	    theBASpeed      = 22.5;
	    	    theBACapacity   = 1600;
	    	    theBANumOfLanes = 2;
            }
            else if (links->at(i)->oneway == 1)
            {
	    	    theType         = "PRINCIPAL";
	    	    theABSpeed      = 22.5;
	    	    theABCapacity   = 1600;
	    	    theABNumOfLanes = 2;
	    	    theBASpeed      = 22.5;
	    	    theBACapacity   = 1600;
	    	    theBANumOfLanes = 2;
            }
            else
            {
                GIS_ERROR("LOGIC:");
                exit(1);
            }
	    }
	    else if (links->at(i)->fcc < "A50")
	    {
            if (links->at(i)->oneway == 0)
            {
	    	    theType         = "MAJOR";
	    	    theABSpeed      = 22.5;
	    	    theABCapacity   = 1200;
	    	    theABNumOfLanes = 2;
	    	    theBASpeed      = 22.5;
	    	    theBACapacity   = 1200;
	    	    theBANumOfLanes = 2;
            }
            else if (links->at(i)->oneway == 1)
            {
	    	    theType         = "MAJOR";
	    	    theABSpeed      = 22.5;
	    	    theABCapacity   = 1200;
	    	    theABNumOfLanes = 2;
	    	    theBASpeed      = 22.5;
	    	    theBACapacity   = 1200;
	    	    theBANumOfLanes = 2;
            }
            else
            {
                GIS_ERROR("LOGIC:");
                exit(1);
            }
	    }
        else if (links->at(i)->fcc < "A60")  
	    {
            if (links->at(i)->oneway == 0)
            {
	    	    theType         = "MINOR";
	    	    theABSpeed      = 15;
	    	    theABCapacity   = 800;
	    	    theABNumOfLanes = 1;
	    	    theBASpeed      = 15;
	    	    theBACapacity   = 800;
	    	    theBANumOfLanes = 1;
            }
            else if (links->at(i)->oneway == 1)
            {
	    	    theType         = "MINOR";
	    	    theABSpeed      = 15;
	    	    theABCapacity   = 800;
	    	    theABNumOfLanes = 1;
	    	    theBASpeed      = 15;
	    	    theBACapacity   = 800;
	    	    theBANumOfLanes = 1;
            }
            else
            {
                GIS_ERROR("LOGIC: oneway = %d", links->at(i)->oneway);
                exit(1);
            }
	    }
	    else if (links->at(i)->fcc < "A70")
	    {
            if (links->at(i)->oneway == 0)
            {
	    	    theType         = "LOCAL";
	    	    theABSpeed      = 7.5;
	    	    theABCapacity   = 800;
		        theABNumOfLanes = 1;
	    	    theBASpeed      = 7.5;
		        theBACapacity   = 800;
		        theBANumOfLanes = 1;
            }
            else if (links->at(i)->oneway == 1)
            {
	    	    theType         = "LOCAL";
	    	    theABSpeed      = 7.5;
	    	    theABCapacity   = 800;
		        theABNumOfLanes = 1;
	    	    theBASpeed      = 7.5;
		        theBACapacity   = 800;
		        theBANumOfLanes = 1;
            }
            else
            {
                GIS_ERROR("LOGIC:");
                exit(1);
            }
	    }

	    oFile
	        << links->at(i)->linkID + 1 << "\t"
            << links->at(i)->name << "\t"
            << links->at(i)->fromNode + 1 << "\t"
            << links->at(i)->toNode + 1 << "\t"
            << theLength << "\t"
            << theType << "\t"
            << theABNumOfLanes << "\t"
            << theABSpeed << "\t"
            << theABCapacity << "\t"
            << theBANumOfLanes << "\t"
            << theBASpeed << "\t"
            << theBACapacity << "\t"
            << "ANY" << "\t"
            << "NOTES"
            << endl;
    }
	oFile.close();

    DEBUG_FUNCTION_END;
}

void
TransimsOutput::set_up_shortest_distance ()
{
    set_up_quickest_travel_time ();
}

void
TransimsOutput::set_up_quickest_travel_time ()
{
    network = new ShortestPathNetwork();

    network->nodes = new ShortestPathNodes();

    for (unsigned i = 0;  i < nodes->size(); ++i)
    {
        ShortestPathNode * theNode = new ShortestPathNode (nodes->at(i)->nodeID, nodes->at(i)->x, nodes->at(i)->y);

        network->nodes->data->push_back(theNode);
    }

    network->links = new ShortestPathLinks();

#define TWO_WAY 
    set<int> theNodeSet;
    int theLinkSize = links->size();

    int64_t theMaxLinkID = -1;
    GIS_ERROR("origin link size = %d", links->size());
    for (unsigned i = 0;  i < links->size(); ++i)
    {
#ifdef TWO_WAY
        ShortestPathLink * theLink = new ShortestPathLink (links->at(i)->linkID, links->at(i)->fromNode, links->at(i)->toNode, links->at(i)->fromSpeedLimit, links->at(i)->distance, links->at(i)->shape, false);
        network->links->data->push_back(theLink);

        theLink = new ShortestPathLink (links->at(i)->linkID + theLinkSize, links->at(i)->toNode, links->at(i)->fromNode, links->at(i)->toSpeedLimit, links->at(i)->distance, links->at(i)->shape, true);
        network->links->data->push_back(theLink);
        theMaxLinkID = MAX(theMaxLinkID, links->at(i)->linkID + theLinkSize);
//        GIS_ERROR("The max link ID %d", theMaxLinkID);
#else
        theMaxLinkID = MAX(theMaxLinkID, links->at(i)->linkID);

        if (links->at(i)->oneway == 1 || links->at(i)->oneway == 0)
        {
            ShortestPathLink * theLink = new ShortestPathLink (links->at(i)->linkID, links->at(i)->fromNode, links->at(i)->toNode, links->at(i)->fromSpeedLimit, links->at(i)->distance, links->at(i)->shape, false);

            network->links->data->push_back(theLink);
        }
        else
        {
            GIS_ERROR("LOGIC");
            exit(1);
        }
#endif
    }

    int64_t theSuperNodeID = 0;

    network->nodes->data->push_back(new ShortestPathNode(theSuperNodeID, -77.20, 38.76));

    double theSuperNodeX = -77.20;
    double theSuperNodeY = 38.76;
    for (unsigned i = 0; i < nodes->size(); ++i)
    {
        if (nodes->at(i)->isExternal)
        {
            ShapeVertexs * theShape = new ShapeVertexs();
            theShape->push_back(ShapeVertex(nodes->at(i)->x, nodes->at(i)->y));
            theShape->push_back(ShapeVertex(theSuperNodeX, theSuperNodeY));
            network->links->data->push_back (new ShortestPathLink (++theMaxLinkID, nodes->at(i)->nodeID, theSuperNodeID, 1000.0, 0, theShape, false));
#ifdef TWO_WAY
            network->links->data->push_back (new ShortestPathLink (++theMaxLinkID, theSuperNodeID, nodes->at(i)->nodeID, 1000.0, 0, theShape, true));
            delete theShape;
#else
            delete theShape;
#endif
        }
    }

    std::string theGISDataHome = getenv ("GISDATAHOME");

    std::string theSuperLinkFileName = theGISDataHome + "/ORTS/pdata/Beijing_CN/superLinks";
    ShortestPathLinks * theSPLinks = (ShortestPathLinks *) network->links;
    theSPLinks->dump_to_shape_file (theSuperLinkFileName);
    network->build_forward_star();
#ifdef TWO_WAY
    DEBUG_INFO ("use two way for all links");
    shortestPath = new ShortestPath<ShortestPathNode, ShortestPathLink>(network->fs);
#else
    network->build_backward_star();
    shortestPath = new ShortestPath<ShortestPathNode, ShortestPathLink>(network->bs);
#endif
    shortestPath->run(theSuperNodeID);
    shortestPath->dump_shortest_path_tree();
}
#define D 2

int
TransimsOutput::nearest_street_node (double aX, double aY)
{
    vector<double>  theQuery(D);
    theQuery[0] = aX;
    theQuery[1] = aY;

    KDTreeResultVector  theResults;
    nodes->tree->n_nearest(theQuery, 1, theResults);

    int     thePos          = theResults[0].index;

    return thePos;
}

double
TransimsOutput::travel_time (int aPos)
{
    double  theTravelTime   = shortestPath->report_impedance (aPos);
    return theTravelTime;
}

int
TransimsOutput::nearest_external_node (int aPos)
{
    int theDestination = shortestPath->report_last_second_node (aPos);
     
    return theDestination;
}

void
TransimsOutput::output_network_trip_table_text_file(std::string& aType)
{
    DEBUG_FUNCTION_BEGIN;

    nodes->build_index();

    if (aType == "1")
    {
        set_up_shortest_distance ();
    }
    else
    {
        set_up_quickest_travel_time ();
    }

	ofstream ofp;
	string theFileName = outputDir + "/Auto_Trips_" + aType + ".txt";
    DEBUG_INFO("write to %s", theFileName.c_str());
	ofp.open(theFileName.c_str());
	if (ofp.fail())
	{
		cerr << "open " << theFileName << " failed" << endl;
		exit (1);
	}

	ofp
		<< "ORG\tDES\tTRIPS" << endl;

    ShapeFileHelperLinks * theExitLinks = new ShapeFileHelperLinks ();

    std::map<int64_t, int> theNodePosMap;
    for (unsigned int i = 0; i < nodes->size(); ++i)
    {
        theNodePosMap[nodes->at(i)->nodeID] = i;
    }

    map<int64_t, int> theSPNodePosMap;
    for (unsigned int i = 0; i < network->nodes->size(); ++i)
    {
        theSPNodePosMap[network->nodes->at(i)->nodeID] = i;
    }

    for (unsigned int i = 0; i < populations->size(); ++i)
    {
/*
if ((i % 100))
    continue;
*/
        int theOrigID = populations->at(i)->nodeID;

        int theOrigPos = nearest_street_node (populations->at(i)->x, populations->at(i)->y);
        int theSPOrigPos = theSPNodePosMap[nodes->at(theOrigPos)->nodeID];
        double theTravelTime   = travel_time (theSPOrigPos);
        int theSPDestPos     = nearest_external_node (theSPOrigPos);
        int theDestPos      = theNodePosMap[network->nodes->at(theSPDestPos)->nodeID];


        int theDestID = -1;
        if (theDestPos > -1)
        {
            theExitLinks->data->push_back (new ShapeFileHelperLink (i, populations->at(i)->x, populations->at(i)->y, nodes->at(theOrigPos)->x, nodes->at(theOrigPos)->y, nodes->at(theDestPos)->x, nodes->at(theDestPos)->y));
            theDestID = nodes->at(theDestPos)->nodeID + 1;
        }
        else
        {
            theExitLinks->data->push_back (new ShapeFileHelperLink (i, nodes->at(theOrigPos)->x, nodes->at(theOrigPos)->y, nodes->at(theDestPos)->x, nodes->at(theDestPos)->y, nodes->at(theDestPos)->x, nodes->at(theDestPos)->y));
        }
        int theTrips = (int) MAX(1.0, ((double) populations->at(i)->population));  
        if (theTravelTime > 1000000)
        {
            GIS_ERROR("nodeID = %d", network->nodes->at(theSPOrigPos)->nodeID);
            continue;
//            exit(1);
        }
       if (theTrips > 0)
        {
            ofp
                << theOrigID  << "\t"
                << theDestID << "\t"
                << theTrips
                << endl;
        }

    }

    std::string theGISDataHome = getenv ("GISDATAHOME");
    std::string theExitFileName = theGISDataHome + "/ORTS/pdata/Beijing_CN/exitLinks_" + GIS::System::Convert::toString(TheMethod);
    theExitLinks->dump_to_shape_file(theExitFileName);

    DEBUG_FUNCTION_END;
}

void
TransimsOutput::output_network_input_zone_text_file()
{
	DEBUG_FUNCTION_BEGIN;

	ofstream oFile;
	string theFileName = outputDir + "/Input_Zone.txt";
    DEBUG_INFO("write to %s", theFileName.c_str());
	oFile.open(theFileName.c_str());
	if (oFile.fail())
	{
		cerr << "open " << theFileName << " failed" << endl;
		exit (1);
	}

	oFile.precision(18);

	oFile << "ZONE\tX_COORD\tY_COORD\tAREATYPE" << endl;
	cout << "The number of zones " << populations->size() << endl;

	for (unsigned int i = 0; i < populations->size(); ++i)
	{
		projUV	p = projection(populations->at(i)->x, populations->at(i)->y);
        int theTrips = (int) MAX(1, ((double) populations->at(i)->population));
        if (theTrips > 0)
        {
		    oFile
		    	<< populations->at(i)->nodeID << "\t"
		    	<< p.u << "\t"
		    	<< p.v << "\t"
			    << 1
			    << endl;
        }
	}

	for (unsigned int i = 0; i < nodes->size(); ++i)
	{
        if (nodes->at(i)->isExternal)
        {
		    projUV	p = projection(nodes->at(i)->x, nodes->at(i)->y);
	        oFile
		        << nodes->at(i)->nodeID + 1  << "\t"
		        << p.u << "\t" 
		        << p.v << "\t"
		        << 2
		        << endl;
        }
    }

	oFile.close();

	DEBUG_FUNCTION_END;
}

void
TransimsOutput::output_network_input_shape_text_file()
{
	DEBUG_FUNCTION_BEGIN;

	ofstream oFile;
	string theFileName = outputDir + "/Input_Shape.txt";
    DEBUG_INFO("write to %s", theFileName.c_str());
	oFile.open(theFileName.c_str());
	if (oFile.fail())
	{
		cerr << "open " << theFileName << " failed" << endl;
		exit (1);
	}

	oFile.precision(18);

	oFile << "LINK\tPOINTS\tNOTES" << endl;
	oFile << "X_COORD\tY_COORD" << endl;
    for (unsigned int i = 0; i < links->size(); ++i)
    {
        if (!links->at(i)->shape)
            continue;

        oFile
            << links->at(i)->linkID + 1 << "\t"
            << links->at(i)->shape->size() << "\t "
            << endl;

        for (unsigned int j = 0; j < links->at(i)->shape->size(); ++j)
        {
		    projUV	p = projection(links->at(i)->shape->at(j)[0], links->at(i)->shape->at(j)[1]);

		    oFile
                << p.u << "\t"
                << p.v 
                << endl;
        }
	}

    oFile.close();

    DEBUG_FUNCTION_END;
}

bool
TransimsOutput::point_on_polygon(double aX1, double aY1, double (*aPolygon)[DIM2], int          aPolygonSize)
{
    double thePoint[2];
    double theEpsl = 0.000001 * DEGREE_TO_METER;

    thePoint[0] = aX1;
    thePoint[1] = aY1;

    bool theReturn =
        GIS::System::Geometry::Search2D::point_on_polygon (
            (double *) thePoint, aPolygon, aPolygonSize, 0.000001);

    return theReturn;
}

void
TransimsOutput::build_network_nodes()
{
    DEBUG_FUNCTION_BEGIN;

    int theEmpty = 0;
    int theCount = 0;

    for (unsigned i = 0; i < links->size(); ++i)
    {
        if (!links->at(i)->shape)
        {
            theEmpty++;
            GIS_ERROR("empty shapes");
            continue;
        }
        char theBuffer[2048];
        sprintf(theBuffer, "%020.12lf:%020.12lf", 
            links->at(i)->shape->at(0)[0], links->at(i)->shape->at(0)[1]);
        std::string theID = theBuffer;
        if (nodeMap.find(theID) != nodeMap.end())
        {
            int theNodeID = nodeMap[theID];
            links->at(i)->fromNode = theNodeID;
        }
        else
        {
            nodeMap[theID] = theCount;
            links->at(i)->fromNode = theCount;
            theCount++;
        }
        int n = links->at(i)->shape->size() - 1;
        sprintf(theBuffer, "%020.12lf:%020.12lf", 
            links->at(i)->shape->at(n)[0], links->at(i)->shape->at(n)[1]);
        theID = theBuffer;
        if (nodeMap.find(theID) != nodeMap.end())
        {
            int theNodeID = nodeMap[theID];
            links->at(i)->toNode = theNodeID;
        }
        else
        {
            nodeMap[theID] = theCount;
            links->at(i)->toNode = theCount;
            theCount++;
        }
    }

    DEBUG_INFO("total %d nodes build", nodeMap.size());

#ifdef LIU
    for (unsigned i = 0; i < links->size(); ++i)
    {
        if (links->at(i)->fromNode == links->at(i)->toNode)
        {
            GIS_ERROR("circle = %d", i);
        }
    }

    HelpFileHelperNetwork * theNetwork = new ShapeFileHelperNetwork();
    theOSMNetwork->links = links;
    theOSMNetwork->nodes = nodes;
    theOSMNetwork->build_forward_star();
    int * theDegrees        = new int[nodes->size()];
    int * theDeletedLinks   = new int[links->size()];

    for (unsigned i = 0; i < links->size(); ++i)
    {
        if (links->at(i)->fromNode >= nodes->size() ||
            links->at(i)->toNode >= nodes->size())
        {
            GIS_ERROR("LOGIC");
            exit(1);
        }
        ++theDegrees[links->at(i)->fromNode];
        ++theDegrees[links->at(i)->toNode];
    }

    for (unsigned i = 0; i < nodes->size(); ++i)
    {
        if (theDegrees[i] > 2)
        {
            for (int j = theOSMNetwork->fs->aNodes[i];
                     j < theOSMNetwork->fs->aNodes[i + 1];
                     ++j)
            {
                GIS_ERROR("from = %d to = %d", nodes->at(i)->nodeID, nodes->at(j)->nodeID);
            }
            exit(1);
        }
    }

    delete theDegrees;

#endif
    DEBUG_FUNCTION_END;
}

void
TransimsOutput::find_center()
{
    DEBUG_FUNCTION_BEGIN;

    xMin =  DBL_MAX;
    yMin =  DBL_MAX;
    xMax = -DBL_MAX;
    yMax = -DBL_MAX;

    for (unsigned i = 0; i < links->size(); ++i)
    {
        if (!links->at(i)->shape)
            continue;

        links->at(i)->linkID = i;
        for (unsigned j = 0; j < links->at(i)->shape->size(); ++j)
        {
            xMin = MIN(xMin, links->at(i)->shape->at(j)[0]);
            yMin = MIN(yMin, links->at(i)->shape->at(j)[1]);
            xMax = MAX(xMax, links->at(i)->shape->at(j)[0]);
            yMax = MAX(yMax, links->at(i)->shape->at(j)[1]);
        }
    }

    xCenter = (xMin + xMax) / 2.0;
    yCenter = (yMin + yMax) / 2.0;

    DEBUG_INFO("xMin = %lf yMin = %lf xMax = %lf yMax = %lf", xMin, yMin, xMax, yMax);
    DEBUG_INFO("xCenter = %lf yCenter = %lf", xCenter, yCenter);

    DEBUG_FUNCTION_END;
}

void
TransimsOutput::find_boundary_nodes()
{
    DEBUG_FUNCTION_BEGIN;

    double (*thePolygon) [2];

    int thePolygonSize = polygons->at(0)->polygons->at(0)->exterior->shape->size();

    thePolygon = new double[thePolygonSize][2];

    for (unsigned j = 0; j < polygons->at(0)->polygons->at(0)->exterior->shape->size(); ++j)
    {
        double theX = polygons->at(0)->polygons->at(0)->exterior->shape->at(j)[0];
        double theY = polygons->at(0)->polygons->at(0)->exterior->shape->at(j)[1];
       thePolygon[j][0] = theX;
       thePolygon[j][1] = theY;
    }

    int theCount = 0;
    map<std::string, int>::iterator it;
	for (it  = nodeMap.begin();
         it != nodeMap.end();
         ++it)
	{
        std::string theIndex = it->first;

        vector<std::string>& theItems = GIS::System::String::split(theIndex, ':');
        if (theItems.size() < 2)
        {
            GIS_ERROR("format error %s", theIndex.c_str());
            exit(1);
        }
        double theX = GIS::System::Convert::toDouble(theItems[0]);
        double theY = GIS::System::Convert::toDouble(theItems[1]);
        delete &theItems;

        if (point_on_polygon(theX, theY, thePolygon, thePolygonSize))
        {
            externalNodeVector.push_back(ExternalNode(it->second, theX, theY));
        }
    }

    DEBUG_INFO("number of external nodes = %d", externalNodeVector.size());

    DEBUG_FUNCTION_END;
}
