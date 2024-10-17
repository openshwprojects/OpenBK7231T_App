#ifndef __TEXT_H__
#define __TEXT_H__

#include "sim_local.h"
#include "Shape.h"

class CText : public CShape {
	bool bDeepText;
	bool bAllowNewLine;
	std::string txt;
	bool bTextEditMode;
	int cursorPos;
	void rotateDegreesAround_internal(float f, const class Coord &p);
	virtual void recalcBoundsSelf();
public:
	CText() {
		cursorPos = 0;
		bTextEditMode = false;
		bDeepText = false;
		bAllowNewLine = true;
	}
	CText(const Coord &np, const char *s) {
		this->setPosition(np);
		this->txt = s;
		cursorPos = strlen(s);
		bTextEditMode = false;
		bDeepText = false;
		bAllowNewLine = true;
	}
	CText(float _x, float _y, const char *s) {
		this->setPosition(_x, _y);
		this->txt = s;
		cursorPos = strlen(s);
		bTextEditMode = false;
		bDeepText = false;
		bAllowNewLine = true;
	}
	void setAllowNewLine(bool b) {
		bAllowNewLine = b;
	}
	bool isBeingEdited() const {
		return bTextEditMode;
	}
	void setTextEditMode(bool b) {
		bTextEditMode = b;
	}
	void setText(const char *s) {
		this->txt = s;
		cursorPos = strlen(s);
	}
	void setTextf(const char *s, ...);
	const char *getText() const {
		return this->txt.c_str();
	}
	void setDeepText(bool b) {
		bDeepText = b;
	}
	float getFloat() const;
	virtual bool isDeepText() const {
		return bDeepText;
	}
	void appendText(const char *s);
	bool processKeyDown(int keyCode);
	virtual CShape *cloneShape();
	virtual float drawPrivateInformation2D(float x, float h);
	virtual const char *getClassName() const {
		return "CText";
	}
	virtual void drawShape();
};


#endif // __TEXT_H__
