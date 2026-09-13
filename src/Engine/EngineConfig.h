// Copyright (c) 2025, WH, All rights reserved.
// global (engine-wide) configuration (constants etc.)
#pragma once

#include "config.h"

#define CASSERT_STR_ENDSWITH(str__, termchar__)                                  \
    static_assert(str__[(sizeof(str__) / sizeof((str__)[0]) - 2)] == termchar__, \
                  #str__ " (" str__ ") must end with " #termchar__)

// build-time default for where the bundled assets live (relative to the working directory, which setcwdexe()
// points at the executable); the actual directories are resolved at startup, see Paths.h
#ifndef MCENGINE_DATA_DIR

#ifndef MCENGINE_DATA_ROOT
#define MCENGINE_DATA_ROOT "."
#endif

#define MCENGINE_DATA_DIR MCENGINE_DATA_ROOT "/"

#endif

#ifndef APP_DATA_DIR
#define APP_DATA_DIR MCENGINE_DATA_DIR
#endif

CASSERT_STR_ENDSWITH(MCENGINE_DATA_DIR, '/');
CASSERT_STR_ENDSWITH(APP_DATA_DIR, '/');
