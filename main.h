#include <Windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "tinyxml2.h"

using std::wctomb;
using std::mbtowc;
using std::string;
using namespace tinyxml2;

unsigned int curHold = 1;
XMLElement * prevHold = NULL;