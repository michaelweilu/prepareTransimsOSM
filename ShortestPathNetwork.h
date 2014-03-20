#ifndef _SHORTEST_PATH_NETWORK_H_
#define _SHORTEST_PATH_NETWORK_H_

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

extern int TheMethod;

class ShortestPathNode : public Node
{ 
public:
	int64_t			nodeID;
    bool            isExternal;

public:
	ShortestPathNode() {}
	
	virtual ~ShortestPathNode() {}

	ShortestPathNode(ShortestPathNode * aHighwayNode) { *this = *aHighwayNode; }

	//ShortestPathNode(int aPos, double aX, double aY, double aZ = 0.0) :
	//	Node(aPos, 0, aX, aY, aZ)
	//{
	//}

	ShortestPathNode(int64_t aNodeID, double aX, double aY, double aZ = 0.0) 
	{
		nodeID = aNodeID;
		strcpy(id, Object::build_node_id(nodeID, (char *) "HH"));
		x = aX;
		y = aY;
		z = aZ;
	}

	ShortestPathNode(double aX, double aY, double aZ = 0.0) :
		Node (0, (int64_t) 0, aX, aY, aZ)
	{
	}

	virtual int read(GISFeature * aFeature)
    {
        GIS_ERROR("LOGIC");
        exit(1);
        return 0;
	}

	friend ostream& operator << (ostream& out, ShortestPathNode& n) {
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
};

template<class N>
class _ShortestPathNodes : public Nodes<N>
{
public:
	_ShortestPathNodes(const char * aNodeFileName)
		: Nodes<N> (aNodeFileName)
	{
	}

	_ShortestPathNodes() {};

	void free()
	{
		//this->data->free();
	}

};

class ShortestPathLink : public Link
{
public:
	int64_t			linkID;
    double          distance;
    double          speed;

public:
	ShortestPathLink() {}

	ShortestPathLink(int64_t aLinkID, int64_t aFrom, int64_t aTo, double aSpeed, double aDistance, ShapeVertexs * aShape, bool aReverseFlag)
	{
		linkID = aLinkID;
		strcpy(id, Object::build_link_id(linkID, (char *) "HH"));
		fromNode = aFrom;
		strcpy(from, Object::build_node_id(fromNode, (char *) "HH"));
		toNode = aTo;
		strcpy(to, Object::build_node_id(toNode, (char *) "HH"));

		speed           = aSpeed;
        distance        = aDistance;
        shape           = new ShapeVertexs();
        for (unsigned i = 0; i < aShape->size(); ++i)
        {
            shape->push_back(ShapeVertex(aShape->at(i)[0], aShape->at(i)[1]));
        }
        if (aReverseFlag)
        {
            std::reverse (shape->begin(), shape->end());  
        }
	}

	ShortestPathLink(ShortestPathLink * aHighwayLink) { *this = *aHighwayLink; }

	virtual int read(GISFeature * aFeature)
    {
        GIS_ERROR("logic");
        exit(1);
	}

	double get_distance() { return distance; }
	double get_imp()
    {
        double theImp;

        if (TheMethod == 1)
        {
            theImp = distance;
        }
        else
        {
            if (speed == 0)
                speed = 15;
            theImp = distance / speed;
//        GIS_ERROR("linkID = %lld distance = %lf speed = %lf imp = %lf", linkID, distance, speed, theImp);
        }
        return theImp;
    }
	void set_imp(double aImp) { imp = aImp; }

	friend ostream& operator << (ostream& out, ShortestPathLink& l)
    {
		int64_t		numOfPoints;
		if (l.shape == NULL)
        {
			numOfPoints = 0;
		}
		else
        {
			numOfPoints = (int64_t) l.shape->size();
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

	virtual ~ShortestPathLink()
    {
		//vector<ShapeVertex*>::iterator it;
		//for (it = shape->begin(); it != shape->end(); ++it) {
		//	delete *it;
		//}
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
//		feature->SetField("FCC",  fcc.c_str());
//		feature->SetField("SPEED", System::Convert::toString(speed).c_str());
//		feature->SetField("MILES", System::Convert::toString(miles).c_str());;
//		feature->SetField("LANES", lanes);
//		feature->SetField("DIRECT", twoWayFlag);
		aShapeFile->create_feature(feature);
		//	delete feature;
	}

};

template<class L>
class _ShortestPathLinks : public Links<L>
{
public:
    int origSize;

public:
	_ShortestPathLinks()
		: Links<L> ()
	{
	}

	_ShortestPathLinks(const char * aLinkFileName)
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

template<class N, class L>
class _ShortestPathNetwork : public Network<N, L>
{
public:
	_ShortestPathNetwork()
	{
		this->nodes = new Nodes<N>;
		this->links = new Links<L>;
	}

	_ShortestPathNetwork(const char * aNetworkFileName)
		: Network<N, L> (aNetworkFileName)
	{
	}

	_ShortestPathNetwork(const char * aNodeFileName, const char * aLinkFileName)
		: Network<N, L> (aNodeFileName, aLinkFileName)
	{
	}

	void add_two_way_links()
	{
//		DEBUG_FUNCTION_BEGIN("Network::add_two_way_links");

		this->links->add_two_way_links();

//		DEBUG_FUNCTION_END("Network::add_two_way_links");
	}

};

typedef _ShortestPathNodes<ShortestPathNode>					ShortestPathNodes;
typedef _ShortestPathLinks<ShortestPathLink>					ShortestPathLinks;
typedef _ShortestPathNetwork<ShortestPathNode, ShortestPathLink>	ShortestPathNetwork;

#endif
