#ifndef _LANDSAN_USA_POP_H_
#define _LANDSAN_USA_POP_H_

#include "Network/gis.h"
#include "GISSystem/GISSystem.h"

class LandScanUSAPop : public Node
{
public:
	LandScanUSAPop() {}
	~LandScanUSAPop()
	{
		GIS_FUNCTION_BEGIN("LandScanUSAPop::~LandScanUSAPop");
		GIS_FUNCTION_END("LandScanUSAPop::~LandScanUSAPop");
	}	
	LandScanUSAPop(LandScanUSAPop * aNode) { *this = *aNode; }

	LandScanUSAPop(int aPos, double aX, double aY, double aZ = 0.0) :
		Node(aPos, (int64_t) 0, aX, aY, aZ)
	{
	}

	LandScanUSAPop(double aX, double aY, double aZ = 0.0) :
		Node (0, (int64_t) 0, aX, aY, aZ)
	{
	}

	virtual int read(OGRFeature * aFeature) {
		OGRFeature *feature = aFeature;
		OGRGeometry *geometry = feature->GetGeometryRef();
		if (geometry != NULL) {
			if ( wkbFlatten(geometry->getGeometryType()) == wkbPoint ) {
		     OGRPoint *point = (OGRPoint *) geometry;
				x = point->getX();
				y = point->getY();
				z = 0;
			}
		}
		nodeID =  feature->GetFieldAsInteger(0);
		population = feature->GetFieldAsInteger(2);
        if (population == 0)
            return -1;
		return 0;
	}

	friend ostream& operator << (ostream& out, LandScanUSAPop& n) {
		out 
			<< "id ("
			<< n.nodeID
			<< ") "
			<< " population(" 
			<< n.population
			<< ")"
			<< " geoID(" 
			<< n.geoID
			<< ")"
			<< endl;

		return out;
	}

public:
	long			nodeID;
	int				population;
	double			lon;
	double			lat;
	long			id;
	long			geoID;
};

typedef Nodes<LandScanUSAPop> LandScanUSAPops;

#endif
