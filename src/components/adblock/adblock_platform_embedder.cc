#include "components/adblock/adblock_platform_embedder.h"

namespace adblock {

AdblockPlatformEmbedder::AdblockPlatformEmbedder(
    scoped_refptr<base::SequencedTaskRunner> task_runner_for_deletion)
    : RefcountedKeyedService(task_runner_for_deletion) {}

}  // namespace adblock
