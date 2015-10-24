/*
 * FactoryManager.cpp
 *
 *  Created on: Dec 1, 2014
 *      Author: rlcevg
 */

#include "module/FactoryManager.h"
#include "module/EconomyManager.h"
#include "module/BuilderManager.h"
#include "setup/SetupManager.h"
#include "terrain/TerrainManager.h"
#include "task/NullTask.h"
#include "task/IdleTask.h"
#include "task/static/RepairTask.h"
#include "task/static/ReclaimTask.h"
#include "CircuitAI.h"
#include "util/Scheduler.h"
#include "util/utils.h"

#include "AIFloat3.h"
#include "OOAICallback.h"
#include "Command.h"
#include "Feature.h"

namespace circuit {

using namespace springai;

CFactoryManager::CFactoryManager(CCircuitAI* circuit) :
		IUnitModule(circuit),
		factoryPower(.0f),
		assistDef(nullptr)
{
	CScheduler* scheduler = circuit->GetScheduler().get();
	scheduler->RunTaskEvery(std::make_shared<CGameTask>(&CFactoryManager::Watchdog, this),
							FRAMES_PER_SEC * 60,
							circuit->GetSkirmishAIId() * WATCHDOG_COUNT + 1);
	const int interval = 4;
	const int offset = circuit->GetSkirmishAIId() % interval;
	scheduler->RunTaskEvery(std::make_shared<CGameTask>(&CFactoryManager::UpdateIdle, this), interval, offset + 0);
	scheduler->RunTaskEvery(std::make_shared<CGameTask>(&CFactoryManager::UpdateAssist, this), interval, offset + 2);

	/*
	 * factory handlers
	 */
	auto factoryCreatedHandler = [this](CCircuitUnit* unit, CCircuitUnit* builder) {
		if (unit->GetTask() == nullptr) {
			unit->SetManager(this);
			nullTask->AssignTo(unit);
		}
	};
	auto factoryFinishedHandler = [this](CCircuitUnit* unit) {
		if (unit->GetTask() == nullptr) {
			unit->SetManager(this);
			idleTask->AssignTo(unit);
		} else {
			nullTask->RemoveAssignee(unit);
		}

		factoryPower += unit->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();

		// check nanos around
		std::set<CCircuitUnit*> nanos;
		float radius = assistDef->GetBuildDistance();
		const AIFloat3& pos = unit->GetUnit()->GetPos();
		auto units = std::move(this->circuit->GetCallback()->GetFriendlyUnitsIn(pos, radius));
		int nanoId = assistDef->GetId();
		int teamId = this->circuit->GetTeamId();
		for (auto nano : units) {
			if (nano == nullptr) {
				continue;
			}
			UnitDef* ndef = nano->GetDef();
			if (ndef->GetUnitDefId() == nanoId && nano->GetTeam() == teamId) {
				CCircuitUnit* ass = this->circuit->GetTeamUnit(nano->GetUnitId());
				nanos.insert(ass);

				std::set<CCircuitUnit*>& facs = assists[ass];
				if (facs.empty()) {
					factoryPower += ass->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
				}
				facs.insert(unit);
			}
			delete ndef;
		}
		utils::free_clear(units);
		factories.emplace_back(unit, nanos, 3, true);

		this->circuit->GetSetupManager()->SetBasePos(pos);
	};
	auto factoryIdleHandler = [this](CCircuitUnit* unit) {
		unit->GetTask()->OnUnitIdle(unit);
	};
	auto factoryDestroyedHandler = [this](CCircuitUnit* unit, CEnemyUnit* attacker) {
		IUnitTask* task = unit->GetTask();
		task->OnUnitDestroyed(unit, attacker);  // can change task
		unit->GetTask()->RemoveAssignee(unit);  // Remove unit from IdleTask

		if (task == nullTask) {  // alternative: unit->GetUnit()->IsBeingBuilt()
			return;
		}
		factoryPower -= unit->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
		for (auto it = factories.begin(); it != factories.end(); ++it) {
			if (it->unit != unit) {
				continue;
			}
			for (CCircuitUnit* ass : it->nanos) {
				std::set<CCircuitUnit*>& facs = assists[ass];
				facs.erase(unit);
				if (facs.empty()) {
					factoryPower -= ass->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
				}
			}
			factories.erase(it);
			break;
		}

		// check if any factory with builders left
		bool valid = false;
		for (SFactory& fac : factories) {
			if (fac.hasBuilder) {
				valid = true;
				break;
			}
		}
		if (!valid) {
			CCircuitDef* facDef = this->circuit->GetCircuitDef("factorycloak");
			this->circuit->GetBuilderManager()->EnqueueTask(IBuilderTask::Priority::HIGH, facDef, -RgtVector,
															IBuilderTask::BuildType::FACTORY);
		}
	};

	/*
	 * armnanotc handlers
	 */
	auto assistCreatedHandler = [this](CCircuitUnit* unit, CCircuitUnit* builder) {
		if (unit->GetTask() == nullptr) {
			unit->SetManager(this);
			nullTask->AssignTo(unit);
		}
	};
	auto assistFinishedHandler = [this](CCircuitUnit* unit) {
		if (unit->GetTask() == nullptr) {
			unit->SetManager(this);
			idleTask->AssignTo(unit);
		} else {
			nullTask->RemoveAssignee(unit);
		}

		Unit* u = unit->GetUnit();
		const AIFloat3& assPos = u->GetPos();

		std::vector<float> params;
		params.push_back(0.0f);
		u->ExecuteCustomCommand(CMD_PRIORITY, params);

		// check factory nano belongs to
		float radius = unit->GetCircuitDef()->GetBuildDistance();
		float qradius = radius * radius;
		std::set<CCircuitUnit*>& facs = assists[unit];
		for (SFactory& fac : factories) {
			if (assPos.SqDistance2D(fac.unit->GetUnit()->GetPos()) >= qradius) {
				continue;
			}
			fac.nanos.insert(unit);
			facs.insert(fac.unit);
		}
		if (!facs.empty()) {
			factoryPower += unit->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();

			bool isInHaven = false;
			for (const AIFloat3& hav : havens) {
				if (assPos.SqDistance2D(hav) < qradius) {
					isInHaven = true;
					break;
				}
			}
			if (!isInHaven) {
				havens.push_back(assPos);
				// TODO: Send HavenFinished message?
			}
		}
	};
	auto assistIdleHandler = [this](CCircuitUnit* unit) {
		unit->GetTask()->OnUnitIdle(unit);
	};
	auto assistDestroyedHandler = [this](CCircuitUnit* unit, CEnemyUnit* attacker) {
		IUnitTask* task = unit->GetTask();
		task->OnUnitDestroyed(unit, attacker);  // can change task
		unit->GetTask()->RemoveAssignee(unit);  // Remove unit from IdleTask

		if (task == nullTask) {  // alternative: unit->GetUnit()->IsBeingBuilt()
			return;
		}
		const AIFloat3& assPos = unit->GetUnit()->GetPos();
		float radius = unit->GetCircuitDef()->GetBuildDistance();
		float qradius = radius * radius;
		for (SFactory& fac : factories) {
			if ((fac.nanos.erase(unit) == 0) || !fac.nanos.empty()) {
				continue;
			}
			auto it = havens.begin();
			while (it != havens.end()) {
				if (it->SqDistance2D(assPos) < qradius) {
					it = havens.erase(it);
					// TODO: Send HavenDestroyed message?
				} else {
					++it;
				}
			}
		}
		if (!assists[unit].empty()) {
			factoryPower -= unit->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
		}
		assists.erase(unit);
	};

	const CCircuitAI::CircuitDefs& defs = circuit->GetCircuitDefs();
	for (auto& kv : defs) {
		CCircuitDef* cdef = kv.second;
		if (!cdef->IsMobile() && cdef->GetUnitDef()->IsBuilder()) {
			CCircuitDef::Id unitDefId = kv.first;
			if  (!kv.second->GetBuildOptions().empty()) {
				createdHandler[unitDefId] = factoryCreatedHandler;
				finishedHandler[unitDefId] = factoryFinishedHandler;
				idleHandler[unitDefId] = factoryIdleHandler;
				destroyedHandler[unitDefId] = factoryDestroyedHandler;
			} else {
				createdHandler[unitDefId] = assistCreatedHandler;
				finishedHandler[unitDefId] = assistFinishedHandler;
				idleHandler[unitDefId] = assistIdleHandler;
				destroyedHandler[unitDefId] = assistDestroyedHandler;
				assistDef = cdef;
			}
		}
	}

	#define NUM 8
	struct SFactoryPreDef {
		SFactoryPreDef(const char* n, const char* b, const std::array<const char*, NUM>& is,
					   const std::array<float, NUM>& p0, const std::array<float, NUM>& p1,
					   const std::function<bool (CEconomyManager* mgr)>& crit)
			: name(n)
			, builder(b)
			, items(is)
			, prob0(p0)
			, prob1(p1)
			, criteria(crit)
		{}
		const char* name;
		const char* builder;
		const std::array<const char*, NUM> items;
		const std::array<float, NUM> prob0;
		const std::array<float, NUM> prob1;
		const std::function<bool (CEconomyManager* mgr)> criteria;  // prob0 criterion
	};
	const SFactoryPreDef facs[] = {
			SFactoryPreDef("factorycloak", "armrectr",
					{"armpw", "armrock", "armwar", "armzeus", "armsnipe", "armjeth", "spherepole", "armham"},
					{.60, .15, .10, .06, .01, .01, .02, .05},
					{.09, .05, .05, .30, .10, .10, .30, .01},
					[](CEconomyManager* mgr) {
						float metalIncome = mgr->GetAvgMetalIncome() * mgr->GetEcoFactor();
						return (metalIncome < 40) || mgr->IsEnergyEmpty();
					}),
			SFactoryPreDef("factorygunship", "armca",
					//blastwing,   gnat,     banshee,  rapier,           brawler,    blackdawn,   krow,     trident
					{"blastwing", "bladew", "armkam", "gunshipsupport", "armbrawl", "blackdawn", "corcrw", "gunshipaa"},
					{.10, .15, .30, .36, .05, .02, .00, .02},
					{.01, .01, .01, .10, .60, .15, .05, .07},
					[](CEconomyManager* mgr) {
						float metalIncome = mgr->GetAvgMetalIncome() * mgr->GetEcoFactor();
						return metalIncome < 40;
					})
	};
	for (const SFactoryPreDef& facPreDef : facs) {
		SFactoryDef facDef;
		facDef.builderDef = circuit->GetCircuitDef(facPreDef.builder);
		facDef.buildDefs.reserve(NUM);
		facDef.prob0.reserve(NUM);
		facDef.prob1.reserve(NUM);
		for (int i = 0; i < NUM; ++i) {
			facDef.buildDefs.push_back(circuit->GetCircuitDef(facPreDef.items[i]));
			facDef.prob0.push_back(facPreDef.prob0[i]);
			facDef.prob1.push_back(facPreDef.prob1[i]);
		}
		facDef.criteria = facPreDef.criteria;
		factoryDefs[circuit->GetCircuitDef(facPreDef.name)->GetId()] = facDef;
	}
	#undef NUM

	// FIXME: EXPERIMENTAL
	/*
	 * striderhub handlers
	 */
	CCircuitDef::Id defId = circuit->GetCircuitDef("striderhub")->GetId();
	finishedHandler[defId] = [this, defId](CCircuitUnit* unit) {
		unit->SetManager(this);

		factoryPower += unit->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
		Unit* u = unit->GetUnit();
		CRecruitTask* task = new CRecruitTask(this, IUnitTask::Priority::HIGH, nullptr, ZeroVector, CRecruitTask::BuildType::FIREPOWER, unit->GetCircuitDef()->GetBuildDistance());
		unit->SetTask(task);

		// check nanos around
		std::set<CCircuitUnit*> nanos;
		float radius = assistDef->GetBuildDistance();
		auto units = std::move(this->circuit->GetCallback()->GetFriendlyUnitsIn(u->GetPos(), radius));
		int nanoId = assistDef->GetId();
		int teamId = this->circuit->GetTeamId();
		for (auto nano : units) {
			if (nano == nullptr) {
				continue;
			}
			UnitDef* ndef = nano->GetDef();
			if (ndef->GetUnitDefId() == nanoId && nano->GetTeam() == teamId) {
				CCircuitUnit* ass = this->circuit->GetTeamUnit(nano->GetUnitId());
				nanos.insert(ass);

				std::set<CCircuitUnit*>& facs = assists[ass];
				if (facs.empty()) {
					factoryPower += ass->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
				}
				facs.insert(unit);
			}
			delete ndef;
		}
		utils::free_clear(units);
		factories.emplace_back(unit, nanos, 9, false);

		std::vector<float> params;
		params.push_back(2.0f);
		u->ExecuteCustomCommand(CMD_PRIORITY, params);

//		u->SetRepeat(true);
		idleHandler[defId](unit);
	};
	idleHandler[defId] = [this](CCircuitUnit* unit) {
		float metalIncome = this->circuit->GetEconomyManager()->GetAvgMetalIncome();
		const char* names[] = {"armcomdgun", "scorpion", "dante", "armraven", "funnelweb", "armbanth", "armorco"};
		const std::array<float, 7> prob0 = {.01, .30, .39, .10, .10, .10, .00};
		const std::array<float, 7> prob1 = {.10, .30, .25, .07, .10, .15, .03};
		const std::array<float, 7>& prob = (metalIncome < 100) ? prob0 : prob1;
		int choice = 0;
		float dice = rand() / (float)RAND_MAX;
		float total;
		for (int i = 0; i < prob.size(); ++i) {
			total += prob[i];
			if (dice < total) {
				choice = i;
				break;
			}
		}
		CCircuitDef* striderDef = this->circuit->GetCircuitDef(names[choice]);
		AIFloat3 pos = unit->GetUnit()->GetPos();
		pos = this->circuit->GetTerrainManager()->FindBuildSite(striderDef, pos, this->circuit->GetCircuitDef("striderhub")->GetBuildDistance(), -1);
		if (pos != -RgtVector) {
			unit->GetUnit()->Build(striderDef->GetUnitDef(), pos, -1, 0, this->circuit->GetLastFrame() + FRAMES_PER_SEC * 10);
		}
	};
	destroyedHandler[defId] = [this](CCircuitUnit* unit, CEnemyUnit* attacker) {
		if (unit->GetUnit()->IsBeingBuilt()) {
			return;
		}
		factoryPower -= unit->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
		for (auto it = factories.begin(); it != factories.end(); ++it) {
			if (it->unit != unit) {
				continue;
			}
			for (CCircuitUnit* ass : it->nanos) {
				std::set<CCircuitUnit*>& facs = assists[ass];
				facs.erase(unit);
				if (facs.empty()) {
					factoryPower -= ass->GetCircuitDef()->GetUnitDef()->GetBuildSpeed();
				}
			}
			factories.erase(it);
			break;
		}
		delete unit->GetTask();
	};
	// FIXME: EXPERIMENTAL
}

CFactoryManager::~CFactoryManager()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
	utils::free_clear(factoryTasks);
	utils::free_clear(deleteTasks);
	utils::free_clear(assistTasks);
	utils::free_clear(deleteAssists);
}

int CFactoryManager::UnitCreated(CCircuitUnit* unit, CCircuitUnit* builder)
{
	auto search = createdHandler.find(unit->GetCircuitDef()->GetId());
	if (search != createdHandler.end()) {
		search->second(unit, builder);
	}

	if (builder == nullptr) {
		return 0; //signaling: OK
	}

	IUnitTask* task = builder->GetTask();
	if (task->GetType() != IUnitTask::Type::FACTORY) {
		return 0; //signaling: OK
	}

	if (unit->GetUnit()->IsBeingBuilt()) {
		CRecruitTask* taskR = static_cast<CRecruitTask*>(task);
		if (taskR->GetTarget() == nullptr) {
			taskR->SetTarget(unit);
			unfinishedUnits[unit] = taskR;
		}
	}

	return 0; //signaling: OK
}

int CFactoryManager::UnitFinished(CCircuitUnit* unit)
{
	auto iter = unfinishedUnits.find(unit);
	if (iter != unfinishedUnits.end()) {
		DoneTask(iter->second);
	}

	auto search = finishedHandler.find(unit->GetCircuitDef()->GetId());
	if (search != finishedHandler.end()) {
		search->second(unit);
	}

	return 0; //signaling: OK
}

int CFactoryManager::UnitIdle(CCircuitUnit* unit)
{
	auto search = idleHandler.find(unit->GetCircuitDef()->GetId());
	if (search != idleHandler.end()) {
		search->second(unit);
	}

	return 0; //signaling: OK
}

int CFactoryManager::UnitDestroyed(CCircuitUnit* unit, CEnemyUnit* attacker)
{
	if (unit->GetUnit()->IsBeingBuilt()) {
		auto iter = unfinishedUnits.find(unit);
		if (iter != unfinishedUnits.end()) {
			AbortTask(iter->second);
		}
	}

	auto search = destroyedHandler.find(unit->GetCircuitDef()->GetId());
	if (search != destroyedHandler.end()) {
		search->second(unit, attacker);
	}

	return 0; //signaling: OK
}

CRecruitTask* CFactoryManager::EnqueueTask(CRecruitTask::Priority priority,
										   CCircuitDef* buildDef,
										   const AIFloat3& position,
										   CRecruitTask::BuildType type,
										   float radius)
{
	CRecruitTask* task = new CRecruitTask(this, priority, buildDef, position, type, radius);
	factoryTasks.insert(task);
	return task;
}

IBuilderTask* CFactoryManager::EnqueueReclaim(IBuilderTask::Priority priority,
											  const springai::AIFloat3& position,
											  float radius,
											  int timeout)
{
	IBuilderTask* task = new CSReclaimTask(this, priority, position, .0f, timeout, radius);
	assistTasks.insert(task);
	return task;
}

IBuilderTask* CFactoryManager::EnqueueRepair(IBuilderTask::Priority priority,
											 CCircuitUnit* target)
{
	IBuilderTask* task = new CSRepairTask(this, priority, target);
	assistTasks.insert(task);
	return task;
}

void CFactoryManager::DequeueTask(IUnitTask* task, bool done)
{
	// TODO: Convert CRecruitTask into IBuilderTask
	auto itf = factoryTasks.find(static_cast<CRecruitTask*>(task));
	if (itf != factoryTasks.end()) {
		unfinishedUnits.erase(static_cast<CRecruitTask*>(task)->GetTarget());
		factoryTasks.erase(itf);
		task->Close(done);
		deleteTasks.insert(static_cast<CRecruitTask*>(task));
		return;
	}
	auto ita = assistTasks.find(static_cast<IBuilderTask*>(task));
	if (ita != assistTasks.end()) {
		assistTasks.erase(ita);
		task->Close(done);
		deleteAssists.insert(static_cast<IBuilderTask*>(task));
	}
}

void CFactoryManager::AssignTask(CCircuitUnit* unit)
{
	if (unit->GetCircuitDef() == assistDef) {  // FIXME: Check Id instead pointers?
		IBuilderTask* task = CreateAssistTask(unit);
		if (task != nullptr) {  // if nullptr then continue to Wait (or Idle)
			task->AssignTo(unit);
		}

	} else {

		IUnitTask* task = nullptr;
		decltype(factoryTasks)::iterator iter = factoryTasks.begin();
		for (; iter != factoryTasks.end(); ++iter) {
			if ((*iter)->CanAssignTo(unit)) {
				task = static_cast<CRecruitTask*>(*iter);
				break;
			}
		}

		if (task == nullptr) {
			task = CreateFactoryTask(unit);

//			iter = factoryTasks.begin();
		}

		task->AssignTo(unit);
//		if (task->IsFull()) {
//			factoryTasks.splice(factoryTasks.end(), factoryTasks, iter);  // move task to back
//		}
	}
}

void CFactoryManager::AbortTask(IUnitTask* task)
{
	DequeueTask(task, false);
}

void CFactoryManager::DoneTask(IUnitTask* task)
{
	DequeueTask(task, true);
}

void CFactoryManager::FallbackTask(CCircuitUnit* unit)
{
}

CCircuitUnit* CFactoryManager::NeedUpgrade()
{
	// TODO: Wrap into predicate
	if (factories.empty()) {
		return nullptr;
	}
	int facSize = factories.size();
	for (auto it = factories.rbegin(); it != factories.rend(); ++it) {
		SFactory& fac = *it;
		if (fac.nanos.size() < facSize * fac.weight) {
			return fac.unit;
		}
	}
	return nullptr;
}

CCircuitUnit* CFactoryManager::GetRandomFactory(CCircuitDef* buildDef)
{
	std::list<CCircuitUnit*> facs;
	for (SFactory& fac : factories) {
		if (fac.unit->GetCircuitDef()->CanBuild(buildDef)) {
			facs.push_back(fac.unit);
		}
	}
	if (facs.empty()) {
		return nullptr;
	}
	auto it = facs.begin();
	std::advance(it, rand() % facs.size());
	return *it;
}

AIFloat3 CFactoryManager::GetClosestHaven(CCircuitUnit* unit) const
{
	if (havens.empty()) {
		return -RgtVector;
	}
	float metric = std::numeric_limits<float>::max();
	const AIFloat3& position = unit->GetUnit()->GetPos();
	CTerrainManager* terrainManager = circuit->GetTerrainManager();
	auto it = havens.begin(), havIt = havens.end();
	for (; it != havens.end(); ++it) {
		if (!terrainManager->CanMoveToPos(unit->GetArea(), *it)) {
			continue;
		}
		float qdist = it->SqDistance2D(position);
		if (qdist < metric) {
			havIt = it;
			metric = qdist;
		}
	}
	return (havIt != havens.end()) ? *havIt : AIFloat3(-RgtVector);
}

CRecruitTask* CFactoryManager::UpdateBuildPower(CCircuitUnit* unit)
{
	if (!CanEnqueueTask()) {
		return nullptr;
	}

	CEconomyManager* economyManager = circuit->GetEconomyManager();
	float metalIncome = std::min(economyManager->GetAvgMetalIncome(), economyManager->GetAvgEnergyIncome());
	CBuilderManager* builderManager = circuit->GetBuilderManager();
	if ((builderManager->GetBuilderPower() >= metalIncome * 2.0f) || (rand() >= RAND_MAX / 2)) {
		return nullptr;
	}

	auto it = factoryDefs.find(unit->GetCircuitDef()->GetId());
	if ((it == factoryDefs.end()) || !it->second.builderDef->IsAvailable()) {
		return nullptr;
	}

	CCircuitDef* buildDef = it->second.builderDef;
	CCircuitUnit* factory = GetRandomFactory(buildDef);
	if (factory != nullptr) {
		const AIFloat3& buildPos = factory->GetUnit()->GetPos();
		CTerrainManager* terrainManager = circuit->GetTerrainManager();
		float radius = std::max(terrainManager->GetTerrainWidth(), terrainManager->GetTerrainHeight()) / 4;
		return EnqueueTask(CRecruitTask::Priority::NORMAL, buildDef, buildPos, CRecruitTask::BuildType::BUILDPOWER, radius);
	}

	return nullptr;
}

CRecruitTask* CFactoryManager::UpdateFirePower(CCircuitUnit* unit)
{
	if (!CanEnqueueTask()) {
		return nullptr;
	}

	auto it = factoryDefs.find(unit->GetCircuitDef()->GetId());
	if (it == factoryDefs.end()) {
		return nullptr;
	}
	const SFactoryDef& facDef = it->second;
	const std::vector<float>& prob = facDef.GetProb(circuit->GetEconomyManager());

	int choice = 0;
	float dice = rand() / (float)RAND_MAX;
	float total;
	for (int i = 0; i < prob.size(); ++i) {
		total += prob[i];
		if (dice < total) {
			choice = i;
			break;
		}
	}

	CCircuitDef* buildDef = facDef.buildDefs[choice];
	if (buildDef->IsAvailable()) {
		const AIFloat3& buildPos = unit->GetUnit()->GetPos();
		UnitDef* def = unit->GetCircuitDef()->GetUnitDef();
		float radius = std::max(def->GetXSize(), def->GetZSize()) * SQUARE_SIZE * 4;
		return EnqueueTask(CRecruitTask::Priority::LOW, buildDef, buildPos, CRecruitTask::BuildType::DEFAULT, radius);
	}

	return nullptr;
}

IUnitTask* CFactoryManager::CreateFactoryTask(CCircuitUnit* unit)
{
	IUnitTask* task = UpdateBuildPower(unit);
	if (task != nullptr) {
		return task;
	}

	task = UpdateFirePower(unit);
	if (task != nullptr) {
		return task;
	}

	return nullTask;
}

IBuilderTask* CFactoryManager::CreateAssistTask(CCircuitUnit* unit)
{
	CEconomyManager* economyManager = circuit->GetEconomyManager();
	bool isMetalEmpty = economyManager->IsMetalEmpty();
	CCircuitUnit* repairTarget = nullptr;
	CCircuitUnit* buildTarget = nullptr;
	bool isBuildMobile = true;
	const AIFloat3& pos = unit->GetUnit()->GetPos();
	float radius = unit->GetCircuitDef()->GetBuildDistance();

	float maxCost = MAX_BUILD_SEC * economyManager->GetAvgMetalIncome() * economyManager->GetEcoFactor();
	CCircuitDef* terraDef = circuit->GetBuilderManager()->GetTerraDef();
	circuit->UpdateFriendlyUnits();
	// NOTE: OOAICallback::GetFriendlyUnitsIn depends on unit's radius
	auto units = std::move(circuit->GetCallback()->GetFriendlyUnitsIn(pos, radius * 0.9f));
	for (Unit* u : units) {
		CCircuitUnit* candUnit = circuit->GetFriendlyUnit(u);
		if (candUnit == nullptr) {
			continue;
		}
		if (u->IsBeingBuilt()) {
			CCircuitDef* cdef = candUnit->GetCircuitDef();
			if (isBuildMobile && (!isMetalEmpty || (*cdef == *terraDef) || (cdef->GetCost() < maxCost))) {
				isBuildMobile = candUnit->GetCircuitDef()->IsMobile();
				buildTarget = candUnit;
			}
		} else if ((repairTarget == nullptr) && (u->GetHealth() < u->GetMaxHealth())) {
			repairTarget = candUnit;
			if (isMetalEmpty) {
				break;
			}
		}
	}
	utils::free_clear(units);
	if (!isMetalEmpty && (buildTarget != nullptr)) {
		// Construction task
		return EnqueueRepair(IBuilderTask::Priority::NORMAL, buildTarget);
	}
	if (repairTarget != nullptr) {
		// Repair task
		return EnqueueRepair(IBuilderTask::Priority::NORMAL, repairTarget);
	}
	if (isMetalEmpty) {
		// Reclaim task
		auto features = std::move(circuit->GetCallback()->GetFeaturesIn(pos, radius));
		if (!features.empty()) {
			utils::free_clear(features);
			return EnqueueReclaim(IBuilderTask::Priority::NORMAL, pos, radius);
		}
	}

	return nullptr;
}

void CFactoryManager::Watchdog()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
	auto checkIdler = [this](CCircuitUnit* unit) {
		auto commands = std::move(unit->GetUnit()->GetCurrentCommands());
		if (commands.empty()) {
			UnitIdle(unit);
		}
		utils::free_clear(commands);
	};

	for (auto& fac : factories) {
		checkIdler(fac.unit);
	}

	for (auto& kv : assists) {
		checkIdler(kv.first);
	}
}

void CFactoryManager::UpdateIdle()
{
	idleTask->Update();
}

void CFactoryManager::UpdateAssist()
{
	utils::free_clear(deleteTasks);
	if (!deleteAssists.empty()) {
		for (auto task : deleteAssists) {
			updateAssists.erase(task);
			delete task;
		}
		deleteAssists.clear();
	}

	auto it = updateAssists.begin();
	unsigned int i = 0;
	while (it != updateAssists.end()) {
		(*it)->Update();

		it = updateAssists.erase(it);
		if (++i >= updateSlice) {
			break;
		}
	}

	if (updateAssists.empty()) {
		updateAssists = assistTasks;
		updateSlice = updateAssists.size() / TEAM_SLOWUPDATE_RATE;
	}
}

} // namespace circuit
