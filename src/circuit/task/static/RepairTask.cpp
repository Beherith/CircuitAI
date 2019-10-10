/*
 * RepairTask.cpp
 *
 *  Created on: Mar 30, 2015
 *      Author: rlcevg
 */

#include "task/static/RepairTask.h"
#include "task/TaskManager.h"
#include "module/BuilderManager.h"
#include "module/EconomyManager.h"
#include "module/FactoryManager.h"
#include "CircuitAI.h"
#include "util/utils.h"

#include "OOAICallback.h"
#include "AISCommands.h"
#include "Feature.h"

namespace circuit {

using namespace springai;

CSRepairTask::CSRepairTask(ITaskManager* mgr, Priority priority, CAllyUnit* target, int timeout)
		: IRepairTask(mgr, priority, Type::FACTORY, target, timeout)
{
}

CSRepairTask::~CSRepairTask()
{
}

void CSRepairTask::Update()
{
	CCircuitAI* circuit = manager->GetCircuit();
	CEconomyManager* economyManager = circuit->GetEconomyManager();
	if (economyManager->GetAvgMetalIncome() < savedIncome * 0.6f) {
		manager->AbortTask(this);
	} else if ((++updCount % 4 == 0) && !units.empty()) {
		CAllyUnit* repTarget = circuit->GetFriendlyUnit(targetId);
		if (repTarget == nullptr) {
			manager->AbortTask(this);
			return;
		}
		IBuilderTask* task = nullptr;
		if (repTarget->GetUnit()->IsBeingBuilt()) {
			CFactoryManager* factoryManager = circuit->GetFactoryManager();
			if (economyManager->IsMetalEmpty() && !factoryManager->IsHighPriority(repTarget)) {
				// Check for damaged units
				CBuilderManager* builderManager = circuit->GetBuilderManager();
				circuit->UpdateFriendlyUnits();
				float radius = (*units.begin())->GetCircuitDef()->GetBuildDistance();
				auto us = std::move(circuit->GetCallback()->GetFriendlyUnitsIn(position, radius * 0.9f));
				for (Unit* u : us) {
					CAllyUnit* candUnit = circuit->GetFriendlyUnit(u);
					if ((candUnit == nullptr) || builderManager->IsReclaimed(candUnit)) {
						continue;
					}
					if (!u->IsBeingBuilt() && (u->GetHealth() < u->GetMaxHealth())) {
						task = factoryManager->EnqueueRepair(IBuilderTask::Priority::NORMAL, candUnit);
						break;
					}
				}
				utils::free_clear(us);
				if (task == nullptr) {
					// Reclaim task
					auto features = std::move(circuit->GetCallback()->GetFeaturesIn(position, radius));
					if (!features.empty()) {
						utils::free_clear(features);
						task = factoryManager->EnqueueReclaim(IBuilderTask::Priority::NORMAL, position, radius);
					}
				}
			}
		} else if (economyManager->IsMetalFull()) {
			// Check for units under construction
			CFactoryManager* factoryManager = circuit->GetFactoryManager();
			CBuilderManager* builderManager = circuit->GetBuilderManager();
			float maxCost = MAX_BUILD_SEC * economyManager->GetAvgMetalIncome() * economyManager->GetEcoFactor();
			circuit->UpdateFriendlyUnits();
			float radius = (*units.begin())->GetCircuitDef()->GetBuildDistance();
			auto us = std::move(circuit->GetCallback()->GetFriendlyUnitsIn(position, radius * 0.9f));
			for (Unit* u : us) {
				CAllyUnit* candUnit = circuit->GetFriendlyUnit(u);
				if ((candUnit == nullptr) || builderManager->IsReclaimed(candUnit)) {
					continue;
				}
				bool isHighPrio = factoryManager->IsHighPriority(candUnit);
				if (u->IsBeingBuilt() && ((candUnit->GetCircuitDef()->GetBuildTime() < maxCost) || isHighPrio)) {
					IBuilderTask::Priority priority = isHighPrio ? IBuilderTask::Priority::HIGH : IBuilderTask::Priority::NORMAL;
					task = factoryManager->EnqueueRepair(priority, candUnit);
					break;
				}
			}
			utils::free_clear(us);
		}
		if (task != nullptr) {
			decltype(units) tmpUnits = units;
			for (CCircuitUnit* unit : tmpUnits) {
				manager->AssignTask(unit, task);
			}
			manager->AbortTask(this);
		}
	}
}

void CSRepairTask::Finish()
{
	CCircuitAI* circuit = manager->GetCircuit();
	for (CCircuitUnit* unit : units) {
		Unit* u = unit->GetUnit();
		TRY_UNIT(circuit, unit,
			u->ExecuteCustomCommand(CMD_PRIORITY, {0});
			u->PatrolTo(position, UNIT_COMMAND_OPTION_SHIFT_KEY);
		)
	}

	IRepairTask::Finish();
}

void CSRepairTask::OnUnitIdle(CCircuitUnit* unit)
{
	manager->DoneTask(this);
}

void CSRepairTask::OnUnitDamaged(CCircuitUnit* unit, CEnemyUnit* attacker)
{
	// TODO: Terraform attacker into dust
}

} // namespace circuit
