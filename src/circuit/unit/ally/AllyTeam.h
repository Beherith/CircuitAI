/*
 * AllyTeam.h
 *
 *  Created on: Apr 7, 2015
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_SETUP_ALLYTEAM_H_
#define SRC_CIRCUIT_SETUP_ALLYTEAM_H_

#include "unit/enemy/EnemyManager.h"

#include <memory>
#include <map>
#include <unordered_set>

namespace springai {
	class AIFloat3;
	class Unit;
}

namespace circuit {

class CCircuitAI;
class CAllyUnit;
class CMapManager;
class CMetalManager;
class CEnergyGrid;
class CDefenceMatrix;
class CPathFinder;
class CFactoryData;
struct STerrainMapArea;

class CAllyTeam {
public:
	using Id = int;
	using AllyUnits = std::map<ICoreUnit::Id, CAllyUnit*>;
	using TeamIds = std::unordered_set<Id>;
	union SBox {
		SBox(): edge{0.f, 0.f, 0.f, 0.f} {}
		struct {
			float bottom;
			float left;
			float right;
			float top;
		};
		float edge[4];

		bool ContainsPoint(const springai::AIFloat3& point) const;
	};
	struct SClusterTeam {
		SClusterTeam(int tid, unsigned cnt = 0) : teamId(tid), count(cnt) {}
		int teamId;  // cluster leader
		unsigned count;  // number of commanders
	};
	// FIXME: DEBUG
//	struct SAreaTeam {
//		SAreaTeam(int tid) : teamId(tid) {}
//		int teamId;  // area leader
//	};
	// FIXME: DEBUG

public:
	CAllyTeam(const TeamIds& tids, const SBox& sb);
	virtual ~CAllyTeam();

	int GetSize() const { return teamIds.size(); }
	int GetAliveSize() const { return GetSize() - resignSize; }
	const TeamIds& GetTeamIds() const { return teamIds; }
	const SBox& GetStartBox() const { return startBox; }

	void Init(CCircuitAI* circuit, float decloakRadius);
	void Release();

	void UpdateFriendlyUnits(CCircuitAI* circuit);
	CAllyUnit* GetFriendlyUnit(ICoreUnit::Id unitId) const;
	const AllyUnits& GetFriendlyUnits() const { return friendlyUnits; }

	const std::vector<ICoreUnit::Id>& GetEnemyGarbage() const { return enemyManager->GetGarbage(); }
	bool EnemyInLOS(CEnemyUnit* data, CCircuitAI* ai);
	std::pair<CEnemyUnit*, bool> RegisterEnemyUnit(ICoreUnit::Id unitId, bool isInLOS, CCircuitAI* ai);
	CEnemyUnit* RegisterEnemyUnit(springai::Unit* e, CCircuitAI* ai);
	void UnregisterEnemyUnit(CEnemyUnit* data, CCircuitAI* ai);

	void EnemyEnterLOS(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyLeaveLOS(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyEnterRadar(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyLeaveRadar(CEnemyUnit* enemy, CCircuitAI* ai);
	void EnemyDestroyed(CEnemyUnit* enemy, CCircuitAI* ai);

	void Update(CCircuitAI* ai);
	void EnqueueUpdate();

	std::shared_ptr<CMapManager>& GetMapManager() { return mapManager; }
	std::shared_ptr<CEnemyManager>& GetEnemyManager() { return enemyManager; }
	std::shared_ptr<CMetalManager>& GetMetalManager() { return metalManager; }
	std::shared_ptr<CEnergyGrid>& GetEnergyGrid() { return energyGrid; }
	std::shared_ptr<CDefenceMatrix>& GetDefenceMatrix() { return defence; }
	std::shared_ptr<CPathFinder>& GetPathfinder() { return pathfinder; }
	std::shared_ptr<CFactoryData>& GetFactoryData() { return factoryData; }

	void OccupyCluster(int clusterId, int teamId);
	SClusterTeam GetClusterTeam(int clusterId);
	// FIXME: DEBUG
//	void OccupyArea(STerrainMapArea* area, int teamId);
//	SAreaTeam GetAreaTeam(STerrainMapArea* area);
	// FIXME: DEBUG

private:
	void DelegateAuthority(CCircuitAI* curOwner);

	CCircuitAI* circuit;  // authority
	TeamIds teamIds;
	SBox startBox;

	int initCount;
	int resignSize;
	int lastUpdate;
	AllyUnits friendlyUnits;  // owner

	std::map<int, SClusterTeam> occupants;  // Cluster owner on start. clusterId: SClusterTeam
	// FIXME: DEBUG
//	std::map<STerrainMapArea*, SAreaTeam> habitants;  // Area habitants on start.
	// FIXME: DEBUG

	std::shared_ptr<CMapManager> mapManager;
	std::shared_ptr<CMetalManager> metalManager;
	std::shared_ptr<CEnergyGrid> energyGrid;
	std::shared_ptr<CDefenceMatrix> defence;
	std::shared_ptr<CPathFinder> pathfinder;
	std::shared_ptr<CFactoryData> factoryData;

	std::shared_ptr<CEnemyManager> enemyManager;

	int uEnemyMark;
};

} // namespace circuit

#endif // SRC_CIRCUIT_SETUP_ALLYTEAM_H_