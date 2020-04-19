/*
 * Circuit.h
 *
 *  Created on: Aug 9, 2014
 *      Author: rlcevg
 */

#ifndef SRC_CIRCUIT_CIRCUIT_H_
#define SRC_CIRCUIT_CIRCUIT_H_

#include "unit/CircuitDef.h"
#include "unit/CircuitWDef.h"
#include "unit/ally/AllyTeam.h"
#include "util/Defines.h"

#include <memory>
#include <unordered_map>
#include <map>
#include <set>
#include <vector>

struct SSkirmishAICallback;

namespace circuit {

#define ERROR_UNKNOWN			200
#define ERROR_INIT				(ERROR_UNKNOWN + EVENT_INIT)
#define ERROR_RELEASE			(ERROR_UNKNOWN + EVENT_RELEASE)
#define ERROR_UPDATE			(ERROR_UNKNOWN + EVENT_UPDATE)
#define ERROR_UNIT_CREATED		(ERROR_UNKNOWN + EVENT_UNIT_CREATED)
#define ERROR_UNIT_FINISHED		(ERROR_UNKNOWN + EVENT_UNIT_FINISHED)
#define ERROR_UNIT_IDLE			(ERROR_UNKNOWN + EVENT_UNIT_IDLE)
#define ERROR_UNIT_MOVE_FAILED	(ERROR_UNKNOWN + EVENT_UNIT_MOVE_FAILED)
#define ERROR_UNIT_DAMAGED		(ERROR_UNKNOWN + EVENT_UNIT_DAMAGED)
#define ERROR_UNIT_DESTROYED	(ERROR_UNKNOWN + EVENT_UNIT_DESTROYED)
#define ERROR_UNIT_GIVEN		(ERROR_UNKNOWN + EVENT_UNIT_GIVEN)
#define ERROR_UNIT_CAPTURED		(ERROR_UNKNOWN + EVENT_UNIT_CAPTURED)
#define ERROR_ENEMY_ENTER_LOS	(ERROR_UNKNOWN + EVENT_ENEMY_ENTER_LOS)
#define ERROR_ENEMY_LEAVE_LOS	(ERROR_UNKNOWN + EVENT_ENEMY_LEAVE_LOS)
#define ERROR_ENEMY_ENTER_RADAR	(ERROR_UNKNOWN + EVENT_ENEMY_ENTER_RADAR)
#define ERROR_ENEMY_LEAVE_RADAR	(ERROR_UNKNOWN + EVENT_ENEMY_LEAVE_RADAR)
#define ERROR_ENEMY_DAMAGED		(ERROR_UNKNOWN + EVENT_ENEMY_DAMAGED)
#define ERROR_ENEMY_DESTROYED	(ERROR_UNKNOWN + EVENT_ENEMY_DESTROYED)
#define ERROR_LOAD				(ERROR_UNKNOWN + EVENT_LOAD)
#define ERROR_SAVE				(ERROR_UNKNOWN + EVENT_SAVE)
#define ERROR_ENEMY_CREATED		(ERROR_UNKNOWN + EVENT_ENEMY_CREATED)
#define LOG(fmt, ...)	GetLog()->DoLog(utils::string_format(std::string(fmt), ##__VA_ARGS__).c_str())

class CGameAttribute;
class CSetupManager;
class CEnemyManager;
class CMapManager;
class CThreatMap;
class CInfluenceMap;
class CPathFinder;
class CTerrainManager;
class CBuilderManager;
class CFactoryManager;
class CEconomyManager;
class CMilitaryManager;
class CScriptManager;
class CScheduler;
class IModule;
class CCircuitUnit;
class CEnemyInfo;
class COOAICallback;
class CEngine;
class CMap;
#ifdef DEBUG_VIS
class CDebugDrawer;
#endif

/*
 * Эти парни не созданы чувствовать!
 * Ледяная душа не боится жути!
 * Только под ногами их крутятся:
 * По оси земля, по полу полу-люди!
 */
constexpr char version[]{"1.1.0"};

class CException: public std::exception {
public:
	CException(const char* r) : std::exception(), reason(r) {}
	virtual const char* what() const throw() {
		return reason;
	}
	const char* reason;
};

class CCircuitAI {
public:
	CCircuitAI(springai::OOAICallback* callback);
	virtual ~CCircuitAI();

// ---- AI Event handler ---- BEGIN
public:
	int HandleEvent(int topic, const void* data);
	void NotifyGameEnd();
	void NotifyResign();
	void Resign(int newTeamId);
private:
	typedef int (CCircuitAI::*EventHandlerPtr)(int topic, const void* data);
	int HandleGameEvent(int topic, const void* data);
	int HandleEndEvent(int topic, const void* data);
	int HandleResignEvent(int topic, const void* data);
	EventHandlerPtr eventHandler;

	int ownerTeamId;
	springai::Economy* economy;
	springai::Resource* metalRes;
	springai::Resource* energyRes;
// ---- AI Event handler ---- END

private:
//	bool IsModValid();
	void CheatPreload();
	int Init(int skirmishAIId, const struct SSkirmishAICallback* sAICallback);
	int Release(int reason);
	int Update(int frame);
	int Message(int playerId, const char* message);
	int UnitCreated(CCircuitUnit* unit, CCircuitUnit* builder);
	int UnitFinished(CCircuitUnit* unit);
	int UnitIdle(CCircuitUnit* unit);
	int UnitMoveFailed(CCircuitUnit* unit);
	int UnitDamaged(CCircuitUnit* unit, CEnemyInfo* attacker/*, int weaponId*/);
	int UnitDestroyed(CCircuitUnit* unit, CEnemyInfo* attacker);
	int UnitGiven(ICoreUnit::Id unitId, int oldTeamId, int newTeamId);
	int UnitCaptured(ICoreUnit::Id unitId, int oldTeamId, int newTeamId);
	int EnemyEnterLOS(CEnemyInfo* enemy);
	int EnemyLeaveLOS(CEnemyInfo* enemy);
	int EnemyEnterRadar(CEnemyInfo* enemy);
	int EnemyLeaveRadar(CEnemyInfo* enemy);
	int EnemyDamaged(CEnemyInfo* enemy);
	int EnemyDestroyed(CEnemyInfo* enemy);
	int PlayerCommand(std::vector<CCircuitUnit*>& units);
//	int CommandFinished(CCircuitUnit* unit, int commandTopicId, springai::Command* cmd);
	int Load(std::istream& is);
	int Save(std::ostream& os);
	int LuaMessage(const char* inData);

// ---- Units ---- BEGIN
public:
	using Units = std::map<ICoreUnit::Id, CCircuitUnit*>;
private:
	CCircuitUnit* GetOrRegTeamUnit(ICoreUnit::Id unitId);
	CCircuitUnit* RegisterTeamUnit(ICoreUnit::Id unitId);
	CCircuitUnit* RegisterTeamUnit(ICoreUnit::Id unitId, springai::Unit* u);
	void UnregisterTeamUnit(CCircuitUnit* unit);
	void DeleteTeamUnit(CCircuitUnit* unit);
public:
	void Garbage(CCircuitUnit* unit, const char* reason);
	CCircuitUnit* GetTeamUnit(ICoreUnit::Id unitId) const;
	const Units& GetTeamUnits() const { return teamUnits; }

	void UpdateFriendlyUnits() { allyTeam->UpdateFriendlyUnits(this); }
	CAllyUnit* GetFriendlyUnit(springai::Unit* u) const;
	CAllyUnit* GetFriendlyUnit(ICoreUnit::Id unitId) const { return allyTeam->GetFriendlyUnit(unitId); }
	const CAllyTeam::AllyUnits& GetFriendlyUnits() const { return allyTeam->GetFriendlyUnits(); }

	using EnemyInfos = std::map<ICoreUnit::Id, CEnemyInfo*>;
private:
	std::pair<CEnemyInfo*, bool> RegisterEnemyInfo(ICoreUnit::Id unitId, bool isInLOS = false);
	CEnemyInfo* RegisterEnemyInfo(springai::Unit* e);
	void UnregisterEnemyInfo(CEnemyInfo* enemy);
public:
	CEnemyInfo* GetEnemyInfo(springai::Unit* u) const;
	CEnemyInfo* GetEnemyInfo(ICoreUnit::Id unitId) const;
	const EnemyInfos& GetEnemyInfos() const { return enemyInfos; }

	CAllyTeam* GetAllyTeam() const { return allyTeam; }

	void DisableControl(CCircuitUnit* unit);
	void DisableControl(const std::string data);
	void EnableControl(const std::string data);

	void AddActionUnit(CCircuitUnit* unit) { actionUnits.push_back(unit); }

private:
	void UpdateActions();

	Units teamUnits;  // owner
	EnemyInfos enemyInfos;  // owner
	CAllyTeam* allyTeam;

	std::vector<CCircuitUnit*> actionUnits;
	unsigned int actionIterator;

	std::set<CCircuitUnit*> garbage;
// ---- Units ---- END

// ---- AIOptions.lua ---- BEGIN
public:
	bool IsCheating() const { return isCheating; }
	bool IsAllyAware() const { return isAllyAware; }
	bool IsCommMerge() const { return isCommMerge; }
private:
	std::string InitOptions();
	bool isCheating;
	bool isAllyAware;
	bool isCommMerge;
// ---- AIOptions.lua ---- END

// ---- UnitDefs ---- BEGIN
public:
	using CircuitDefs = std::unordered_map<CCircuitDef::Id, CCircuitDef*>;
	using NamedDefs = std::map<const char*, CCircuitDef*, cmp_str>;

	const CircuitDefs& GetCircuitDefs() const { return defsById; }
	CCircuitDef* GetCircuitDef(const char* name);
	CCircuitDef* GetCircuitDef(CCircuitDef::Id unitDefId);
//	const std::vector<CCircuitDef*>& GetKnownDefs() const { return knownDefs; }
private:
	void InitUnitDefs(float& outDcr);
//	void InitKnownDefs(const CCircuitDef* commDef);
	CircuitDefs defsById;  // owner
	NamedDefs defsByName;
//	std::vector<CCircuitDef*> knownDefs;
// ---- UnitDefs ---- END

// ---- WeaponDefs ---- BEGIN
public:
	using WeaponDefs = std::vector<CWeaponDef*>;

	CWeaponDef* GetWeaponDef(CWeaponDef::Id weaponDefId) const;
private:
	void InitWeaponDefs();
	WeaponDefs weaponDefs;  // owner
// ---- WeaponDefs ---- END

public:
	bool IsInitialized() const { return isInitialized; }
	bool IsLoadSave() const { return isLoadSave; }
	CGameAttribute* GetGameAttribute() const { return gameAttribute.get(); }
	std::shared_ptr<CScheduler>& GetScheduler() { return scheduler; }
	int GetLastFrame()    const { return lastFrame; }
	int GetSkirmishAIId() const { return skirmishAIId; }
	int GetTeamId()       const { return teamId; }
	int GetAllyTeamId()   const { return allyTeamId; }
	COOAICallback*        GetCallback()   const { return callback.get(); }
	CEngine*              GetEngine()     const { return engine.get(); }
	springai::Cheats*     GetCheats()     const { return cheats.get(); }
	springai::Log*        GetLog()        const { return log.get(); }
	springai::Game*       GetGame()       const { return game.get(); }
	CMap*                 GetMap()        const { return map.get(); }
	springai::Lua*        GetLua()        const { return lua.get(); }
	springai::Pathing*    GetPathing()    const { return pathing.get(); }
	springai::Drawer*     GetDrawer()     const { return drawer.get(); }
	springai::SkirmishAI* GetSkirmishAI() const { return skirmishAI.get(); }
	springai::Team*       GetTeam()       const { return team.get(); }
	CScriptManager*   GetScriptManager()   const { return scriptManager.get(); }
	CSetupManager*    GetSetupManager()    const { return setupManager.get(); }
	CEnemyManager*    GetEnemyManager()    const { return enemyManager.get(); }
	CMetalManager*    GetMetalManager()    const { return metalManager.get(); }
	CMapManager*      GetMapManager()      const { return mapManager.get(); }
	CThreatMap*       GetThreatMap()       const;
	CInfluenceMap*    GetInflMap()         const;
	CPathFinder*      GetPathfinder()      const { return pathfinder.get(); }
	CTerrainManager*  GetTerrainManager()  const { return terrainManager.get(); }
	CBuilderManager*  GetBuilderManager()  const { return builderManager.get(); }
	CFactoryManager*  GetFactoryManager()  const { return factoryManager.get(); }
	CEconomyManager*  GetEconomyManager()  const { return economyManager.get(); }
	CMilitaryManager* GetMilitaryManager() const { return militaryManager.get(); }

	int GetAirCategory()   const { return airCategory; }
	int GetLandCategory()  const { return landCategory; }
	int GetWaterCategory() const { return waterCategory; }
	int GetBadCategory()   const { return badCategory; }
	int GetGoodCategory()  const { return goodCategory; }

private:
	// debug
//	void DrawClusters();

	bool isInitialized;
	bool isLoadSave;
	bool isResigned;
	int lastFrame;
	int skirmishAIId;
	int teamId;
	int allyTeamId;
	std::unique_ptr<COOAICallback>        callback;
	std::unique_ptr<CEngine>              engine;
	std::unique_ptr<springai::Cheats>     cheats;
	std::unique_ptr<springai::Log>        log;
	std::unique_ptr<springai::Game>       game;
	std::unique_ptr<CMap>                 map;
	std::unique_ptr<springai::Lua>        lua;
	std::unique_ptr<springai::Pathing>    pathing;
	std::unique_ptr<springai::Drawer>     drawer;
	std::unique_ptr<springai::SkirmishAI> skirmishAI;
	std::unique_ptr<springai::Team>       team;

	static std::unique_ptr<CGameAttribute> gameAttribute;
	static unsigned int gaCounter;
	void CreateGameAttribute();
	void DestroyGameAttribute();
	std::shared_ptr<CScheduler> scheduler;
	std::shared_ptr<CScriptManager> scriptManager;
	std::shared_ptr<CSetupManager> setupManager;
	std::shared_ptr<CEnemyManager> enemyManager;
	std::shared_ptr<CMetalManager> metalManager;
	std::shared_ptr<CMapManager> mapManager;
	std::shared_ptr<CPathFinder> pathfinder;
	std::shared_ptr<CTerrainManager> terrainManager;
	std::shared_ptr<CBuilderManager> builderManager;
	std::shared_ptr<CFactoryManager> factoryManager;
	std::shared_ptr<CEconomyManager> economyManager;
	std::shared_ptr<CMilitaryManager> militaryManager;
	std::vector<std::shared_ptr<IModule>> modules;

	// TODO: Move into GameAttribute? Or use locally
	int airCategory;  // over surface
	int landCategory;  // on surface
	int waterCategory;  // under surface
	int badCategory;
	int goodCategory;

#ifdef DEBUG_VIS
private:
	std::shared_ptr<CDebugDrawer> debugDrawer;
public:
	std::shared_ptr<CDebugDrawer>& GetDebugDrawer() { return debugDrawer; }
#endif
};

} // namespace circuit

#endif // SRC_CIRCUIT_CIRCUIT_H_
