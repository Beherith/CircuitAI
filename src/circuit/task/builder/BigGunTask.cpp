/*
 * BigGunTask.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: rlcevg
 */

#include "task/builder/BigGunTask.h"
#include "util/utils.h"

namespace circuit {

using namespace springai;

CBBigGunTask::CBBigGunTask(ITaskManager* mgr, Priority priority,
						   UnitDef* buildDef, const AIFloat3& position,
						   float cost, int timeout) :
		IBuilderTask(mgr, priority, buildDef, position, BuildType::BIG_GUN, cost, timeout)
{
}

CBBigGunTask::~CBBigGunTask()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
}

} // namespace circuit
