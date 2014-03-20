#ifndef _OSM_HIGHWAY_H_
#define _OSM_HIGHWAY_H_

#include <map>
#include <vector>
using namespace std;

#include "Network/gis.h"

#include "GISSystem/GISSystem.h"
using namespace GIS;

#ifdef USE_GDAL
typedef OGRFeature			GISFeature;
typedef OGRGeometry			GISGeometry;
typedef OGRPoint			GISPoint;
typedef	OGRLineString		GISLineString;
#else
#include "GISPoint.h"
#include "GISFeature.h"
#endif


//Define the Node class
class OSMHighwayNode : public Node
{ 
public:
	int64_t			nodeID;
	int64_t			population;
	string			blockID;
	vector<int>	*	residents;
	string			type;
    bool            isBoundaryExit;
    bool            isExternal;

public:
	OSMHighwayNode() {}
	
	virtual ~OSMHighwayNode() {}

	OSMHighwayNode(OSMHighwayNode * aHighwayNode) { *this = *aHighwayNode; }


	OSMHighwayNode(int64_t aNodeID, double aX, double aY, double aZ = 0.0) 
	{
		nodeID = aNodeID;
		x = aX;
		y = aY;
		z = aZ;
        isExternal      = false;
        isBoundaryExit  = false;
	}

	OSMHighwayNode(double aX, double aY, double aZ = 0.0) :
		Node (0, (int64_t) 0, aX, aY, aZ)
	{
        isExternal      = false;
        isBoundaryExit  = false;
	}

	virtual int read(GISFeature * aFeature)
    {
		GISFeature *feature = aFeature;
		GISGeometry *geometry = feature->GetGeometryRef();
		if (geometry != NULL)
        {
			if ( wkbFlatten(geometry->getGeometryType()) == wkbPoint)
			{
				GISPoint *point = (GISPoint *) geometry;
				x = point->getX();
				y = point->getY();
				z = 0;
			}
		}

		string theNodeID = feature->GetFieldAsString(0);

		nodeID = System::Convert::toLongLong((char *) theNodeID.c_str());
		strcpy(id, Object::build_node_id(nodeID, (char *) "H"));
		population = 0;
		type = feature->GetFieldAsString(3);
        isExternal      = false;
        isBoundaryExit  = false;
		return 0;
	}

	friend ostream& operator << (ostream& out, OSMHighwayNode& n) {
		out 
			<< "\""
			<< n.id
			<< "\""
			<< " owners: ";
		if (n.owners != NULL) {
			set<string>::iterator it;
			for (it = n.owners->begin(); it != n.owners->end(); ++it) {
				out << " " << *it;
			}
		}
		out
			<< endl;
		return out;
	}

	void write_shape_file_record(GISShapeFile * aShapeFile)
	{
		GISPoint * thePoint = new GISPoint(x, y, 0);
		GISFeature *  feature = aShapeFile->new_feature();
		feature->SetGeometry( thePoint );
		feature->SetField("NODEID", System::Convert::toString(nodeID).c_str());
		feature->SetField("LON", System::Convert::toString(x).c_str());
		feature->SetField("LAT", System::Convert::toString(y).c_str());
		feature->SetField("TYPE", type.c_str());				
		aShapeFile->create_feature(feature);
#ifdef LINUX
		delete feature;
#endif
	}

	void write_shape_file_record(
		GISShapeFile * aShapeFile, 
		int aID, 
		double aX, 
		double aY, 
		string aType)
	{
		GISPoint * thePoint = new GISPoint(aX, aY, 0);
		GISFeature *  feature = aShapeFile->new_feature();
		feature->SetGeometry( thePoint );
		feature->SetField("NODEID", aID);
		feature->SetField("LON", System::Convert::toString(aX).c_str());
		feature->SetField("LAT", System::Convert::toString(aY).c_str());
		feature->SetField("TYPE", aType.c_str());				
		aShapeFile->create_feature(feature);
#ifdef LINUX
		delete feature;
#endif
	}

};

//Define the Nodes array
template<class N>
class _OSMHighwayNodes : public Nodes<N>
{
public:
	_OSMHighwayNodes(const char * aNodeFileName)
		: Nodes<N> (aNodeFileName)
	{
	}

	_OSMHighwayNodes() {};

	static GISShapeFile * write_shape_file_header(string aFileName)
	{
		GIS_FUNCTION_BEGIN("write_shape_file_header");
		GISShapeFile * sf = new GISShapeFile(aFileName.c_str());
		DBFields *  dbfs = new DBFields();
		dbfs->push_back(new DBField("NODEID", DBSTRING, 12));
		dbfs->push_back(new DBField("LON", DBSTRING, 20));
		dbfs->push_back(new DBField("LAT", DBSTRING, 20));
		dbfs->push_back(new DBField("TYPE", DBSTRING, 12));
		sf->set_dbfields(dbfs);
		sf->create(GIS_POINT);
		GIS_FUNCTION_END("write_shape_file_header");
		return sf;
	}

	void free()
    {
	}

    void dump_to_shape_file(string& aFileName)
    {
	    GISShapeFile * theShapeFile = write_shape_file_header(aFileName);
        for (unsigned i = 0; i < this->data->size(); ++i)
        {
            this->data->at(i)->write_shape_file_record(theShapeFile);
        }
        theShapeFile->close();
        delete theShapeFile;
    }
};

//Define the link class
class OSMHighwayLink : public Link {
public:
    long long   origID;
	long long   linkID;
    std::string type;
    double      fromSpeedLimit;
    double      toSpeedLimit;
    int         fromLanes;
    int         toLanes;
    int         oneway;
    std::string bridge;
    std::string fcc;
    double      distance;
    std::string roadClass;
    std::string name;

public:
	OSMHighwayLink() {}

	OSMHighwayLink(OSMHighwayLink * aHighwayLink) { *this = *aHighwayLink; }

	virtual int read(GISFeature * aFeature)
    {
		GISFeature *feature = aFeature;
		GISGeometry *geometry = feature->GetGeometryRef();
		if (geometry != NULL)
        {
			if ( wkbFlatten(geometry->getGeometryType()) == wkbLineString ) 
            {
				GISLineString *lineString = (GISLineString *) geometry;
				shape = new ShapeVertexs();
				for (long i = 0; i < lineString->getNumPoints(); ++i) {
					shape->push_back(ShapeVertex(lineString->getX(i), lineString->getY(i)));
				}
			}
            else
            {
                return -1;
            }
		}
        
        distance        = 0;
        for (unsigned i = 1; i < shape->size(); ++i)
        {
            double theX1 = shape->at(i - 1)[0];
            double theY1 = shape->at(i - 1)[1];
            double theX2 = shape->at(i)[0];
            double theY2 = shape->at(i)[1];
            double theDistance = GIS::System::Geometry::great_circle_distance (theX1, theY1, theX2, theY2);
            distance += theDistance;
        }

        distance *= METER_PER_MILE;

		string theLinkID = feature->GetFieldAsString(0);
		linkID = System::Convert::toLongLong((char *) theLinkID.c_str());
		strcpy(id, Object::build_link_id(linkID, (char *) "H"));

        origID = linkID;


        //Read file from OSM Beijing
        name            = feature->GetFieldAsString(2);
        oneway          = feature->GetFieldAsInteger(6);
        type            = feature->GetFieldAsString(8);
        roadClass       = feature->GetFieldAsString(9);
        
        fromNode = toNode = -1;
        fcc = "";
        if (type == "motorway" || type == "motorway_link")
        {
            fcc = "A10";
            fromSpeedLimit  = 37.5;
            toSpeedLimit    = 37.5;
        }
        else if (type == "trunk" || type == "trunk_link")
        {
            fcc = "A20";
            fromSpeedLimit  = 30;
            toSpeedLimit    = 30;
        }
        else if (type == "primary" || type == "primary_link")
        {
            fcc = "A30";
            fromSpeedLimit  = 22.5;
            toSpeedLimit    = 22.5;
        }
        else if (type == "secondary" || type == "secondary_link")
        {
            fcc = "A40";
            fromSpeedLimit  = 22.5;
            toSpeedLimit    = 22.5;
        }
        
        else if (type == "tertiary" || type == "tertiary_link")
        {
            fcc = "A50";
            fromSpeedLimit  = 15;
            toSpeedLimit    = 15;
        }
        else if (type == "unclassified" || type == "residential" || type == "service")
        {
            fcc = "A60";
            fromSpeedLimit  = 7.5;
            toSpeedLimit    = 7.5;
        }
        else
        {
            GIS_ERROR("type = %s", type.c_str());
            exit(1);
        }

		owners = NULL;
		return 0;
	}

	double get_distance() { return distance; }
	double get_imp() { return imp; }
	void set_imp(double aImp) { imp = aImp; }

	friend ostream& operator << (ostream& out, OSMHighwayLink& l) {
		long		numOfPoints;
		if (l.shape == NULL) {
			numOfPoints = 0;
		}
		else {
			numOfPoints = (long) l.shape->size();
		}
		out 
			<< "\""
			<< l.id
			<< "\""
			<< " from = " 
			<< "\""
			<< l.fromNode
			<< "\""
		   	<< " to = " 
			<< "\""
			<< l.toNode
			<< "\""
			<< " number of points "
			<< numOfPoints
			<< " imp "
			<< l.imp
			<< endl;
		return out;
	}

	virtual ~OSMHighwayLink() {
		shape->clear();
		delete shape;
	}

	void write_shape_file_record(GISShapeFile * aShapeFile)
	{
		GISLineString* lineString = new GISLineString();
		for (unsigned j = 0; j < shape->size(); ++j) 
		{
			lineString->setPoint(
				j, 
				shape->at(j)[0],
				shape->at(j)[1], 
				0.0);
		}
		GISFeature* feature = aShapeFile->new_feature();
		feature->SetGeometry( lineString );
		feature->SetField("LINKID", GIS::System::Convert::toString(linkID).c_str());
		feature->SetField("FROM", GIS::System::Convert::toString(fromNode).c_str());
		feature->SetField("TO", GIS::System::Convert::toString(toNode).c_str());
		feature->SetField("FCC",  fcc.c_str());
//		feature->SetField("SPEED", System::Convert::toString(speed).c_str());
//		feature->SetField("MILES", System::Convert::toString(miles).c_str());;
//		feature->SetField("LANES", lanes);
		feature->SetField("DIRECT", twoWayFlag);
		aShapeFile->create_feature(feature);
		//	delete feature;
	}

	void write_shape_file_record(
		GISShapeFile * aShapeFile, 
		OSMHighwayLink * aLink)
	{
GIS_ERROR("not implemented yet");
exit(1);
#ifdef LIU
		GISLineString* lineString = new GISLineString();
		for (unsigned j = 0; j < aLink->shape->size(); ++j) 
		{
			lineString->setPoint(
				j, 
				aLink->shape->at(j)[0],
				aLink->shape->at(j)[1], 
				0.0);
		}
	
		GISFeature* feature = aShapeFile->new_feature();
		feature->SetGeometryDirectly( lineString );
		feature->SetField("LINKID", GIS::System::Convert::toString(aLink->linkID).c_str());
		feature->SetField("FROM", GIS::System::Convert::toString(aLink->fromNode).c_str());
		feature->SetField("TO", GIS::System::Convert::toString(aLink->toNode).c_str());
		feature->SetField("FCC",  aLink->fcc.c_str());
		feature->SetField("SPEED", aLink->speed);
		feature->SetField("MILES", aLink->miles);
		feature->SetField("LANES", aLink->lanes);
		feature->SetField("DIRECT", aLink->twoWayFlag);
		aShapeFile->create_feature(feature);
		//	delete feature;
#endif
	}

};

//Define the links array
template<class L>
class _OSMHighwayLinks : public Links<L>
{
public:
	_OSMHighwayLinks()
		: Links<L> ()
	{
	}

	_OSMHighwayLinks(const char * aLinkFileName)
		: Links<L> (aLinkFileName)
	{
	}

	static GISShapeFile * write_shape_file_header(string aFileName)
	{
		GIS_FUNCTION_BEGIN("write_shape_file_header");
		GISShapeFile * sf = new GISShapeFile(aFileName.c_str());
		DBFields * dbfs = new DBFields();
	
		dbfs->push_back(new DBField("LINKID", DBSTRING, 20));
		dbfs->push_back(new DBField("FROM", DBSTRING, 20));
		dbfs->push_back(new DBField("TO", DBSTRING, 20));
		dbfs->push_back(new DBField("FCC", DBSTRING, 8));
		dbfs->push_back(new DBField("SPEED", DBSTRING, 8));
		dbfs->push_back(new DBField("MILES", DBSTRING, 12));
		dbfs->push_back(new DBField("LANES", DBINT, 8));
		dbfs->push_back(new DBField("DIRECT", DBINT, 8));
	
		sf->set_dbfields(dbfs);
		sf->create(GIS_LINE);

		GIS_FUNCTION_END("write_shape_file_header");
		return sf;
	}

	void free()
	{
		//this->data->free();
	}

    void dump_to_shape_file(string& aFileName)
    {
	    GISShapeFile * theShapeFile = write_shape_file_header(aFileName);
        for (unsigned i = 0; i < this->data->size(); ++i)
        {
            this->data->at(i)->write_shape_file_record(theShapeFile);
        }
        theShapeFile->close();
        delete theShapeFile;
    }
};

//Define the network class
template<class N, class L>
class _OSMNetwork : public Network<N, L>
{
public:
	_OSMNetwork()
	{
		this->nodes = new Nodes<N>;
		this->links = new Links<L>;
	}

	_OSMNetwork(const char * aNetworkFileName)
		: Network<N, L> (aNetworkFileName)
	{
	}

	_OSMNetwork(const char * aNodeFileName, const char * aLinkFileName)
		: Network<N, L> (aNodeFileName, aLinkFileName)
	{
	}
};

typedef _OSMHighwayNodes<OSMHighwayNode>					OSMHighwayNodes;
typedef _OSMHighwayLinks<OSMHighwayLink>					OSMHighwayLinks;
typedef _OSMNetwork<OSMHighwayNode, OSMHighwayLink>         OSMHighwayNetwork;

#endif
