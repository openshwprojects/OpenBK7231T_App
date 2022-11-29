
#include "sim_local.h"
#include "Shape.h"

class CObject : public CShape {
	CString name;
public:
	CObject() {

	}
	CObject(CShape *s) {
		addShape(s);
	}
	virtual CShape *cloneShape();
	class CObject *cloneObject();
	void setName(const char *s) {
		name = s;
	}
	const char *getName() const {
		return name.c_str();
	}
};
