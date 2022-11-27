
#include "sim_local.h"
#include "Shape.h"

class CObject : public CShape {

public:
	CObject() {

	}
	CObject(CShape *s) {
		addShape(s);
	}
	virtual CShape *cloneShape();
	class CObject *cloneObject();
};
