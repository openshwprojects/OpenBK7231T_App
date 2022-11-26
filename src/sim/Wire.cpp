#ifdef WINDOWS
#include "Wire.h"
#include "Junction.h"

CWire::CWire(const Coord &a, const Coord &b) {
	addPoint(a);
	addPoint(b);
}
CWire::~CWire() {
	for (int i = 0; i < junctions.size(); i++) {
		delete junctions[i];
	}
	for (int i = 0; i < edges.size(); i++) {
		delete edges[i];
	}
}
class CShape* CWire::findSubPart(const Coord &p) {
	for (int i = 0; i < junctions.size(); i++) {
		if (p.isWithinDist(junctions[i]->getPosition(), 15))
		{
			return junctions[i];
		}
	}
	for (int i = 0; i < edges.size(); i++) {
		if (p.isWithinDist(edges[i]->getPositionA(), edges[i]->getPositionB(), 5))
		{
			return edges[i];
		}
	}
	return 0;
}

void CWire::addPoint(const Coord &p) {
	CJunction *prev = 0;
	if (junctions.size()) {
		prev = junctions[junctions.size() - 1];
	}
	CJunction *j = new CJunction(p.getX(), p.getY(), "");
	j->setParent(this);
	junctions.push_back(j);
	if (prev != 0) {
		CEdge *e = new CEdge(prev, j);
		e->setParent(this);
		edges.push_back(e);
	}
}



void CWire::drawWire() {
	glColor3f(0, 1, 0);
	glLineWidth(2.0f);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < junctions.size(); i++) {
		CJunction *j = junctions[i];
		glVertex2f(j->getX(), j->getY());
	}
	glEnd();
}

#endif
