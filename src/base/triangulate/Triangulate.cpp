#include "Triangulate.h"
#include "sweep/Sweep.h"
#include "sweep/SweepContext.h"

#include "common/Shapes.h"

using namespace std;

namespace avg {

std::vector<int> triangulatePolygon(const Vec2Vector& points, const std::vector<int>& holeIndexes)
{
	std::vector<Point*> polyline;
	unsigned int contourEnd;


	if (holeIndexes.size() > 0) {
		contourEnd = holeIndexes[0];
	} else {
		contourEnd = points.size();
	}

	for (unsigned int i = 0; i < contourEnd; i++) {
		polyline.push_back(new Point(points[i].x, points[i].y, i));
	}

	SweepContext* sweepContext = new SweepContext(polyline);
	Sweep* sweep = new Sweep;

	if (holeIndexes.size() > 0) {
		std::vector<Point*> holeLine;
		for (unsigned int i = 0; i < holeIndexes.size(); i++) {
			for (unsigned int j = holeIndexes[i]; j < points.size() && j < holeIndexes[i+1]; j++) {
				holeLine.push_back(new Point(points[j].x, points[j].y, j));
			}
			sweepContext->AddHole(holeLine);
			holeLine.clear();
		}
	}

	sweep->Triangulate(*sweepContext);

	std::vector<int> result;
	std::vector<avg::TriangulationTriangle*> triangles =  sweepContext->GetTriangles();
	for (unsigned int i = 0; i < triangles.size(); ++i) {
		result.push_back(triangles[i]->GetPoint(0)->m_index);
		result.push_back(triangles[i]->GetPoint(1)->m_index);
		result.push_back(triangles[i]->GetPoint(2)->m_index);
	}

	return result;
}

}
