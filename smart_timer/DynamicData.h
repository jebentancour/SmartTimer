
#ifndef _DYNAMICDATA_h
#define _DYNAMICDATA_h

#include "Config.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

void send_networks_scan_xml();

void send_configuration_values_xml();

void save_configuration();

void req_reset();

#endif

