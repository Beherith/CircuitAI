/*
 * Module.cpp
 *
 *  Created on: Sep 5, 2014
 *      Author: rlcevg
 */

#include "Module.h"

namespace circuit {

IModule::IModule(CCircuitAI* circuit) :
		circuit(circuit)
{
}

IModule::~IModule()
{
}

int IModule::UnitCreated(CCircuitUnit* unit, CCircuitUnit* builder)
{
	return 0; //signaling: OK
}

int IModule::UnitFinished(CCircuitUnit* unit)
{
	return 0; //signaling: OK
}

int IModule::UnitIdle(CCircuitUnit* unit)
{
	return 0; //signaling: OK
}

int IModule::UnitDestroyed(CCircuitUnit* unit, CCircuitUnit* attacker)
{
	return 0; //signaling: OK
}

int IModule::UnitGiven(CCircuitUnit* unit, int oldTeamId, int newTeamId)
{
	return 0; //signaling: OK
}

int IModule::UnitCaptured(CCircuitUnit* unit, int oldTeamId, int newTeamId)
{
	return 0; //signaling: OK
}

} // namespace circuit
