#ifdef WINDOWS
#include "PrefabManager.h"
#include "Shape.h"
#include "Wire.h"
#include "Text.h"
#include "Controller_Button.h"
#include "Controller_Switch.h"
#include "Controller_Bulb.h"
#include "Controller_SimulatorLink.h"
#include "Controller_BL0942.h"
#include "Controller_Pot.h"
#include "Controller_WS2812.h"
#include "Controller_DHT11.h"
#include "Junction.h"

class CShape *PrefabManager::generateVDD() {
	CShape *o = new CShape();
	o->setName("VDD");
	o->addJunction(0, 0, "VDD");
	o->addLine(0, -20, 0, 0);
	o->addCircle(0, -30, 10);
	return o;
}
class CShape *PrefabManager::generateGND() {
	CShape *o = new CShape();
	o->setName("GND");
	o->addJunction(0, -20, "GND");
	o->addLine(0, -20, 0, 0);
	o->addLine(-10, 0, 10, 0);
	o->addLine(-8, 2, 8, 2);
	o->addLine(-6, 4, 6, 4);
	o->addLine(-4, 6, 4, 6);
	return o;
}
class CShape *PrefabManager::generateButton() {
	CShape *o = new CShape();
	o->setName("Button");
	CJunction *a = o->addJunction(-40, -10, "pad_a");
	CJunction *b = o->addJunction(40, -10, "pad_b");
	a->setName("pad_a");
	b->setName("pad_b");
	o->addLine(40, -10, 20, -10);
	o->addLine(-40, -10, -20, -10);
#if 0
	CShape *mover = new CShape();
	mover->addLine(20, 10, -20, 10);
	mover->addLine(0, 20, 0, 10);
	o->addShape(mover);
#else
	CShape *mover = o->addLine(20, 10, -20, 10);
	//	mover->addLine(-20, 0, -20, 10);

#endif
	mover->setName("button_mover");
	CControllerButton *btn = new CControllerButton(a, b);
	btn->setMover(mover);
	o->setController(btn);
	o->translateEachChild(0, 10);
	return o;
}
class CShape *PrefabManager::generateSwitch() {
	CShape *o = new CShape();
	o->setName("Switch");
	CJunction *a = o->addJunction(-40, -10, "pad_a");
	CJunction *b = o->addJunction(40, -10, "pad_b");
	a->setName("pad_a");
	b->setName("pad_b");
	o->addLine(40, -10, 20, -10);
	o->addLine(-40, -10, -20, -10);

	CShape *mover = o->addLine(20, -10, -20, 10);

	mover->setName("switch_mover");
	CControllerSwitch *btn = new CControllerSwitch(a, b);
	btn->setMover(mover);
	o->setController(btn);
	o->translateEachChild(0, 10);
	return o;
}
class CShape *PrefabManager::generateBL0942() {

	CShape *o = new CShape();
	o->setName("BL0942");
	float w = 40.0f;
	float h = 40.0f;
	o->addText(-w - 5.0f, -h - 5.0f, "BL0942");
	o->addText(-w + 5.0f, -h + 15.0f, "U:");
	o->addText(-w + 5.0f, -h + 35.0f, "I:");
	o->addText(-w + 5.0f, -h + 55.0f, "P:");
	o->addText(-w + 5.0f, -h + 75.0f, "F:");
	CText *tx_voltage = o->addText(-w + 25, -h + 15, "230V", true, false)->asText();
	tx_voltage->setName("text_voltage");
	CText *tx_current = o->addText(-w + 25, -h + 35, "0.24A", true, false)->asText();
	tx_current->setName("text_current");
	CText *tx_power = o->addText(-w + 25, -h + 55, "60W", true, false)->asText();
	tx_power->setName("text_power");
	CText *tx_freq = o->addText(-w + 25, -h + 75, "60Hz", true, false)->asText();
	tx_freq->setName("text_freq");
	o->addRect(-w, -h, w * 2, 80);
	CJunction *rx = o->addJunction(-w-20, -20, "RX");
	o->addLine(-w-20, -20, -w, -20);
	rx->setName("RX");
	rx->addText(-5, -5, "RX");
	CJunction *tx = o->addJunction(-w - 20, 20, "TX");
	o->addLine(-w - 20, 20, -w, 20);
	tx->setName("TX");
	tx->addText(-5, -5, "TX");

	CJunction *vdd = o->addJunction(w + 20, -20, "VDD");
	o->addLine(w + 20, -20, w, -20);
	vdd->setName("VDD");
	vdd->addText(-5, -5, "VDD");
	CJunction *gnd = o->addJunction(w + 20, 20, "GND");
	o->addLine(w + 20, 20, w, 20);
	gnd->setName("GND");
	gnd->addText(-5, -5, "GND");
	CControllerBL0942 *cntr = new CControllerBL0942(rx, tx, tx_voltage, tx_current, tx_power, tx_freq);
	o->setController(cntr);
	return o;
}
class CShape *PrefabManager::generateDHT11() {

	CShape *o = new CShape();
	o->setName("DHT11");
	int w = 40;
	int h = 40;
	o->addText(-w - 5, -h - 5, "DHT11");
	o->addText(-w + 5, -h + 15, "T:");
	o->addText(-w + 5, -h + 35, "H:");
	CText *tx_temperatura = o->addText(-w + 25, -h + 15, "19.9C", true, false)->asText();
	tx_temperatura->setName("tex_temperature");
	CText *tx_humidity = o->addText(-w + 25, -h + 35, "85%", true, false)->asText();
	tx_humidity->setName("text_humidity");
	o->addRect(-w, -h, w * 2, 80);
	CJunction *dat = o->addJunction(w + 20, 0, "DAT");
	o->addLine(w + 20, 0, w, 0);
	dat->setName("DAT");
	dat->addText(-5, -5, "DAT");

	CJunction *vdd = o->addJunction(w + 20, 20, "VDD");
	o->addLine(w + 20, 20, w, 20);
	vdd->setName("VDD");
	vdd->addText(-5, -5, "VDD");
	CJunction *gnd = o->addJunction(w + 20, -20, "GND");
	o->addLine(w + 20, -20, w, -20);
	gnd->setName("GND");
	gnd->addText(-5, -5, "GND");
	CControllerDHT11 *cntr = new CControllerDHT11(dat, tx_temperatura, tx_humidity);
	o->setController(cntr);
	return o;
}
class CShape *PrefabManager::generateTest() {

	CShape *o = new CShape();
	o->setName("Test");
	o->addLine(50, 10, -50, 10);
	o->addLine(50, -10, -50, -10);
	o->addLine(50, -10, 50, 10);
	o->addLine(-50, 10, -50, -10);
	return o;
}
class CShape *PrefabManager::generateWB3S() {
	CShape *o = new CShape();
	o->setName("WB3S");
	CControllerSimulatorLink *link = new CControllerSimulatorLink();
	o->setController(link);
	o->addText(-40, -45, "WB3S");
	o->addText(-40, -25, "$simPowerState");
	o->addRect(-50, -20, 100, 180);
	struct PinDef_s {
		const char *name;
		int gpio;
	};
	PinDef_s wb3sPinsRight[] = {
		{ "TXD1", 11 },
		{ "RXD1", 10 },
		{ "PWM2", 8 },
		{ "PWM3", 9 },
		{ "RXD2", 1 },
		{ "TXD2", 0 },
		{ "PWM1", 7 },
		{ "GND", GPIO_GND }
	};
	PinDef_s wb3sPinsLeft[] = {
		{ "CEN", GPIO_CEN },
		{ "ADC3", 23 },
		{ "EN", GPIO_EN },
		{ "P14", 14 },
		{ "PWM5", 26 },
		{ "PWM4", 24 },
		{ "PWM0", 6 },
		{ "VCC", GPIO_VDD }
	};
	for (int i = 0; i < 8; i++) {
		int y = i * 20;
		o->addLine(50, y, 80, y);
		o->addLine(-50, y, -80, y);
		CJunction *a = o->addJunction(-80, y, "", wb3sPinsLeft[i].gpio);
		a->setName(wb3sPinsLeft[i].name);
		a->addText(-5, -5, wb3sPinsLeft[i].name);
		CJunction *b = o->addJunction(80, y, "", wb3sPinsRight[i].gpio);
		b->setName(wb3sPinsRight[i].name);
		b->addText(-25, -5, wb3sPinsRight[i].name);
		link->addRelatedJunction(a);
		link->addRelatedJunction(b);
	}
	return o;
}
class CShape *PrefabManager::generateLED_CW() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->setName("LED_CW");
	o->addText(-40, -25, "CW");
	CShape *filler = o->addCircle(0, 0, bulb_radius);
	CShape *body = o->addCircle(0, 0, bulb_radius);
	filler->setFill(true);
	CJunction *gnd = o->addJunction(-bulb_radius, 0.0f);
	gnd->setName("GND");
	gnd->addText(-5, -5, "");
	CJunction *cool = o->addJunction(bulb_radius, 20.0f);
	cool->setName("C");
	cool->addText(-5, -5, "C");
	CJunction *warm = o->addJunction(bulb_radius, -20.0f);
	warm->setName("W");
	warm->addText(-5, -5, "W");

	CControllerBulb *bulb = new CControllerBulb(cool, warm, gnd);
	filler->setName("internal_bulb_filler");
	bulb->addShape(filler);
	float mul = sqrt(2) * 0.5f;
	o->addLine(-bulb_radius * mul, -bulb_radius * mul, bulb_radius * mul, bulb_radius * mul);
	o->addLine(-bulb_radius * mul, bulb_radius * mul, bulb_radius * mul, -bulb_radius * mul);
	return o;
}
class CShape *PrefabManager::generateLED_RGB() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "RGB");
	o->setName("LED_RGB");
	//CShape *filler = o->addCircle(0, 0, bulb_radius);
	CShape *body = o->addCircle(0, 0, bulb_radius);
	//filler->setFill(true);
	CJunction *a = o->addJunction(-bulb_radius, 0);
	a->setName("GND");
	a->addText(-5, -5, "");
	CJunction *b1 = o->addJunction(bulb_radius, 20);
	b1->setName("R");
	b1->addText(-5, -5, "R");
	CJunction *b2 = o->addJunction(bulb_radius, 0);
	b2->setName("G");
	b2->addText(-5, -5, "G");
	CJunction *b3 = o->addJunction(bulb_radius, -20);
	b3->setName("B");
	b3->addText(-5, -5, "B");

	//filler->setName("internal_bulb_filler");
	//bulb->setShape(filler);
	float mul = sqrt(2) * 0.5f;
	o->addLine(-bulb_radius * mul, -bulb_radius * mul, bulb_radius * mul, bulb_radius * mul);
	o->addLine(-bulb_radius * mul, bulb_radius * mul, bulb_radius * mul, -bulb_radius * mul);
	return o;
}

class CShape *PrefabManager::generateBulb() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "Bulb");
	o->setName("Bulb");
	CShape *filler = o->addCircle(0, 0, bulb_radius);
	CShape *body = o->addCircle(0, 0, bulb_radius);
	filler->setFill(true);
	CJunction *a = o->addJunction(-bulb_radius, 0);
	a->setName("A");
	a->addText(-5, -5, "");
	CJunction *b = o->addJunction(bulb_radius, 0);
	b->setName("B");
	b->addText(-25, -5, "");
	CControllerBulb *bulb = new CControllerBulb(a, b);
	o->setController(bulb);
	filler->setName("internal_bulb_filler");
	bulb->setShape(filler);
	float mul = sqrt(2) * 0.5f;
	o->addLine(-bulb_radius * mul, -bulb_radius * mul, bulb_radius * mul, bulb_radius * mul);
	o->addLine(-bulb_radius * mul, bulb_radius * mul, bulb_radius * mul, -bulb_radius * mul);
	return o;
}

class CShape *PrefabManager::generateStrip_SingleColor() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "Strip Single Color");
	o->setName("StripSingleColor");
	int w = 200;
	o->addRect(-w, -20, w * 2, 40);
	int leds = 8;
	int ofs = 10;
	float start = ofs - w;
	float len = 2*w - ofs * 2;
	float ledlen = len / (leds + 1);
	CJunction *a = o->addJunction(-w, 0);
	a->setName("A");
	a->addText(-5, -5, "");
	CJunction *b = o->addJunction(w, 0);
	b->setName("B");
	b->addText(-25, -5, "");
	CControllerBulb *bulb = new CControllerBulb(a, b);
	o->setController(bulb);
	for (int i = 0; i < leds; i++) {
		float x = start + (len / leds) * i;
		float y = -15;
		int filler_ofs = 1;
		CShape *shape_outer = o->addRect(x, y, ledlen, 30);
		CShape *shape_filler = o->addRect(x+filler_ofs, y + filler_ofs, ledlen - 2 * filler_ofs, 30 - 2 * filler_ofs);
		shape_filler->setFill(true);
		bulb->addShape(shape_filler);
		char str[16];
		sprintf(str, "LED_%i", i);
		shape_filler->setName(str);
	}
	return o;
}
class CShape *PrefabManager::generateStrip_CW() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "Strip CW");
	o->setName("StripCW");
	int w = 200;
	o->addRect(-w, -20, w * 2, 40);
	int leds = 8;
	int ofs = 10;
	float start = ofs - w;
	float len = 2 * w - ofs * 2;
	float ledlen = len / (leds + 1);
	CJunction *warm = o->addJunction(-w, 20);
	warm->setName("W");
	warm->addText(-5, -5, "W");
	CJunction *cool = o->addJunction(-w, -20);
	cool->setName("C");
	cool->addText(-5, -5, "C");
	CJunction *gnd = o->addJunction(w, 0);
	gnd->setName("GND");
	gnd->addText(-25, -5, "GND");
	CControllerBulb *bulb = new CControllerBulb(cool, warm, gnd);
	o->setController(bulb);
	for (int i = 0; i < leds; i++) {
		float x = start + (len / leds) * i;
		float y = -15;
		int filler_ofs = 1;
		CShape *shape_outer = o->addRect(x, y, ledlen, 30);
		CShape *shape_filler = o->addRect(x + filler_ofs, y + filler_ofs, ledlen - 2 * filler_ofs, 30 - 2 * filler_ofs);
		shape_filler->setFill(true);
		bulb->addShape(shape_filler);
		char str[16];
		sprintf(str, "LED_%i", i);
		shape_filler->setName(str);
	}
	return o;
}

class CShape *PrefabManager::generatePot() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "Pot");
	o->setName("Pot");
	int w = 200;
	o->addRect(-w, -20, w * 2, 40);
	CShape *handle = o->addRect(-10, -60, 20, 40);
	handle->setName("pot_handle_mover");
	int leds = 8;
	float ofs = 10.0f;
	float start = ofs - w;
	float len = 2 * w - ofs * 2;
	float ledlen = len / (leds + 1);
	CJunction *out = o->addJunction(-w+80, 40);
	out->setName("OUT");
	out->addText(-5, -5, "OUT");
	CJunction *vdd = o->addJunction(-w+20, 40);
	vdd->setName("VDD");
	vdd->addText(-5, -5, "VDD");
	CJunction *gnd = o->addJunction(w-20, 40);
	gnd->setName("GND");
	gnd->addText(-25, -5, "GND");
	o->addLine(w - 20, 40, w - 20, 20);
	o->addLine(-w + 20, 40, -w + 20, 20);
	o->addLine(-w + 80, 40, -w + 80, 20);
	CShape *text_value = o->addText(0, 0, "1.23V");
	text_value->setName("text_value");
	CControllerPot *pot = new CControllerPot(gnd, vdd, out);
	pot->setMover(handle);
	pot->setDisplay(text_value->asText());
	o->setController(pot);
	return o;
}

class CShape *PrefabManager::generateStrip_RGBCW() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "Strip RGBCW");
	o->setName("StripRGBCW");
	int w = 200;
	o->addRect(-w, -20, w * 2, 40);
	int leds = 8;
	float ofs = 10.0f;
	float start = ofs - w;
	float len = 2 * w - ofs * 2;
	float ledlen = len / (leds + 1);
	CJunction *red = o->addJunction(-w, 20);
	red->setName("R");
	red->addText(-5, -5, "R");
	CJunction *green = o->addJunction(-w, 0);
	green->setName("G");
	green->addText(-5, -5, "G");
	CJunction *blue = o->addJunction(-w, -20);
	blue->setName("B");
	blue->addText(-5, -5, "B");
	CJunction *gnd = o->addJunction(w, 0);
	gnd->setName("GND");
	gnd->addText(-25, -5, "GND");
	CJunction *cool = o->addJunction(w, 20);
	cool->setName("C");
	cool->addText(-25, -5, "C");
	CJunction *warm = o->addJunction(w, -20);
	warm->setName("W");
	warm->addText(-25, -5, "W");
	CControllerBulb *bulb = new CControllerBulb(red, green, blue, cool, warm, gnd);
	o->setController(bulb);
	for (int i = 0; i < leds; i++) {
		float x = start + (len / leds) * i;
		float y = -15;
		int filler_ofs = 1;
		CShape *shape_outer = o->addRect(x, y, ledlen, 30);
		CShape *shape_filler = o->addRect(x + filler_ofs, y + filler_ofs, ledlen - 2 * filler_ofs, 30 - 2 * filler_ofs);
		shape_filler->setFill(true);
		bulb->addShape(shape_filler);
		char str[16];
		sprintf(str, "LED_%i", i);
		shape_filler->setName(str);
	}
	return o;
}
class CShape *PrefabManager::generateWS2812B() {

	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "WS2812");
	o->setName("WS2812");
	CShape *filler = o->addRect(-bulb_radius,-bulb_radius, bulb_radius*2, bulb_radius*2);
	CShape *body = o->addRect(-bulb_radius, -bulb_radius, bulb_radius * 2, bulb_radius * 2);
	filler->setFill(true);
	CJunction *a = o->addJunction(-bulb_radius, 0);
	a->setName("A");
	a->addText(-5, -5, "");
	CJunction *b = o->addJunction(bulb_radius, 0);
	b->setName("B");
	b->addText(-25, -5, "");
	CControllerWS2812 *bulb = new CControllerWS2812(a, b);
	o->setController(bulb);
	filler->setName("internal_bulb_filler");
	bulb->setShape(filler);
	return o;
}
class CShape *PrefabManager::generateStrip_RGB() {
	float bulb_radius = 20.0f;

	CShape *o = new CShape();
	o->addText(-40, -25, "Strip RGB");
	o->setName("StripRGB");
	int w = 200;
	o->addRect(-w, -20, w * 2, 40);
	int leds = 8;
	int ofs = 10;
	float start = ofs - w;
	float len = 2 * w - ofs * 2;
	float ledlen = len / (leds + 1);
	CJunction *red = o->addJunction(-w, 20);
	red->setName("R");
	red->addText(-5, -5, "R");
	CJunction *green = o->addJunction(-w, 0);
	green->setName("G");
	green->addText(-5, -5, "G");
	CJunction *blue = o->addJunction(-w, -20);
	blue->setName("B");
	blue->addText(-5, -5, "B");
	CJunction *gnd = o->addJunction(w, 0);
	gnd->setName("GND");
	gnd->addText(-25, -5, "GND");
	CControllerBulb *bulb = new CControllerBulb(red, green, blue, gnd);
	o->setController(bulb);
	for (int i = 0; i < leds; i++) {
		float x = start + (len / leds) * i;
		float y = -15;
		int filler_ofs = 1;
		CShape *shape_outer = o->addRect(x, y, ledlen, 30);
		CShape *shape_filler = o->addRect(x + filler_ofs, y + filler_ofs, ledlen - 2 * filler_ofs, 30 - 2 * filler_ofs);
		shape_filler->setFill(true);
		bulb->addShape(shape_filler);
		char str[16];
		sprintf(str, "LED_%i", i);
		shape_filler->setName(str);
	}
	return o;
}
void PrefabManager::addPrefab(CShape *o) {
	prefabs.add_unique(o);
}
CShape *PrefabManager::findPrefab(const char *name) {
	for (int i = 0; i < prefabs.size(); i++) {
		if (prefabs[i]->hasName(name))
			return prefabs[i];
	}
	return 0;
}
CShape *PrefabManager::instantiatePrefab(const char *name) {
	CShape *ret = findPrefab(name);
	if (ret == 0)
		return 0;
	return ret->cloneShape();
}
void PrefabManager::createDefaultPrefabs() {

	
	addPrefab(generateStrip_SingleColor());
	addPrefab(generateStrip_CW());
	addPrefab(generateStrip_RGB());
	addPrefab(generateStrip_RGBCW());
	addPrefab(generateLED_CW());
	addPrefab(generateLED_RGB());
	addPrefab(generateBulb());
	addPrefab(generateWB3S());
	addPrefab(generateButton());
	addPrefab(generateSwitch());
	addPrefab(generateTest());
	addPrefab(generateGND());
	addPrefab(generateVDD());
	addPrefab(generateBL0942());
	addPrefab(generatePot());
	addPrefab(generateWS2812B());
	addPrefab(generateDHT11());
}


#endif
