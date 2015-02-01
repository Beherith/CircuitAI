/*
 * BunkerTask.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: rlcevg
 */

#include "task/builder/BunkerTask.h"
#include "util/utils.h"

namespace circuit {

using namespace springai;

CBBunkerTask::CBBunkerTask(ITaskManager* mgr, Priority priority,
						   UnitDef* buildDef, const AIFloat3& position,
						   float cost, int timeout) :
		IBuilderTask(mgr, priority, buildDef, position, BuildType::BUNKER, cost, timeout)
{
}

CBBunkerTask::~CBBunkerTask()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
}

} // namespace circuit
