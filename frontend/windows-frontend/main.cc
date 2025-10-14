#include "logging.h"


int main(int argc, char** argv)
{
    LOG_TRACE("Open game rom:%s", "mario.nes", "", 1234);
    LOG_TRACE("rom size:%d", 123);
    LOG_DEBUG("debug:%d", 123);
    LOG_INFO("info:%d", 123);
    LOG_WARN("warn:%d", 123);
    LOG_ERROR("error:%d", 123);
    LOG_FATAL("fatal:%d", 123);
    return 0;
}