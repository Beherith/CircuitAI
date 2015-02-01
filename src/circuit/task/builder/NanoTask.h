/*
 * NanoTask.h
 *
 *  Created on: Jan 30, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_TASK_BUILDER_NANOTASK_H_
#define SRC_CIRCUIT_TASK_BUILDER_NANOTASK_H_

#include "task/builder/BuilderTask.h"

namespace circuit {

class CBNanoTask: public IBuilderTask
{
public:
	CBNanoTask(ITaskManager* mgr, Priority priority,
			   springai::UnitDef* buildDef, const springai::AIFloat3& position,
			   float cost, int timeout);
	virtual ~CBNanoTask();

	virtual void Execute(CCircuitUnit* unit);
};

} // namespace circuit

#endif // SRC_CIRCUIT_TASK_BUILDER_NANOTASK_H_
