
#ifndef _PREPARE_TRANSIMS_H_
#define _PREPARE_TRANSIMS_H_

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
#include "Network/ShortestPath.h"
using namespace GIS;

#include <proj_api.h>
#include "OSMHighway.h"
#include "ShortestPathNetwork.h"
#include "LandScanUSAPop.h"
#include "StandardPolygon.h"

#define DEGREE_TO_METER     111221.76384
#define METERS_PER_MILE     1609.344
#define SUPER_NODE_ALT      -1000

class ExternalNode
{
public:
    int64_t         nodeID;
    double          x;
    double          y;

public:
    ExternalNode(int64_t aNodeID, double aX, double aY)
        : nodeID(aNodeID), x(aX), y(aY)
    {
    }
};

class TransimsOutput
{
public:
    OSMHighwayLinks         *   links;
    OSMHighwayNodes         *   nodes;
    LandScanUSAPops         *   populations;
    StandardPolygons        *   polygons;

    std::string                 outputDir;
    projPJ                      pj;
    vector<ExternalNode>        externalNodeVector;
    map<std::string, int>       nodeMap;
    map<int64_t, int>           nodePosMap;
    double                      xMin;
    double                      yMin;
    double                      xMax;
    double                      yMax;
    double                      xCenter;
    double                      yCenter;
    int                         numOfExternalNodes;
    int64_t                     maxLinkID;

    ShortestPathNetwork                                 * network;
    ShortestPath<ShortestPathNode, ShortestPathLink>    * shortestPath;

public:
    projUV projection(double aX, double aY);
    bool point_on_polygon(double aX1, double aY1, double (*aPolygon)[DIM2], int aPolygonSize);

    bool is_boundary_exit_node(int64_t aNodeID);
    int  rename_node();
    int  nearest_street_node (double aX, double aY);
    int  nearest_external_node (int aPos);
    double travel_time (int aPos);

    void find_center();
    void find_boundary_nodes();

    void build_network_nodes();
    void build_network_shape_file_nodes_and_links();
    void build_network_external_nodes_and_links();
    void build_network_super_node_and_links();
//    void build_network_external_links();

    void output_network_input_node_files();
    void output_network_input_link_files();
    void output_network_trip_table_text_file (std::string& aType);
    void output_network_input_zone_text_file();
    void output_network_input_shape_text_file();

    void set_up_quickest_travel_time ();
    void set_up_shortest_distance ();
};


#endif
