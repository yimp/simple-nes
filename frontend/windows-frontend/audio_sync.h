#ifndef __AUDIO_SYNC_H__
#define __AUDIO_SYNC_H__
#include "simple_retro.h"

namespace audio_sync
{
typedef void* context_t;
context_t start(SimpleRetro*);
void destroy(context_t);

} // namespace audio_sync

#endif

