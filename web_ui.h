#pragma once
#include <WebServer.h>
#include "config.h"

void webui_setup(WebServer& server);
void webui_loop(WebServer& server);