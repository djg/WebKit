//
//  TestWGSLAPI.cpp
//  TestWGSL
//
//  Created by Dan Glastonbury on 28/9/2022.
//

#include "config.h"

#include "TestWGSLAPI.h"
#include "WGSL.h"
#include <wtf/Assertions.h>

namespace TestWGSLAPI {

void logCompilationError(WGSL::CompilationMessage& error)
{
    WTFLogAlways("%u:%u length:%u %s", error.lineNumber(), error.lineOffset(), error.length(), error.message().utf8().data());
    GTEST_FAIL();
}

} // namespace TestWGSLAPI
