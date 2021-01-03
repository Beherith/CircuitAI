/*
 * FactoryScript.cpp
 *
 *  Created on: Apr 4, 2019
 *      Author: rlcevg
 */

#include "script/FactoryScript.h"
#include "script/ScriptManager.h"
#include "module/FactoryManager.h"
#include "util/Utils.h"
#include "angelscript/include/angelscript.h"

namespace circuit {

CFactoryScript::CFactoryScript(CScriptManager* scr, CFactoryManager* mgr)
		: IUnitModuleScript(scr, mgr)
{
	asIScriptEngine* engine = script->GetEngine();
	int r = engine->RegisterObjectType("CFactoryManager", 0, asOBJ_REF | asOBJ_NOHANDLE); ASSERT(r >= 0);
	r = engine->RegisterGlobalProperty("CFactoryManager aiFactoryMgr", manager); ASSERT(r >= 0);
	r = engine->RegisterObjectMethod("CFactoryManager", "IUnitTask@+ DefaultMakeTask(CCircuitUnit@)", asMETHOD(CFactoryManager, DefaultMakeTask), asCALL_THISCALL); ASSERT(r >= 0);
	r = engine->RegisterObjectMethod("CFactoryManager", "CCircuitDef@ GetRoleDef(const CCircuitDef@, Type)", asMETHOD(CFactoryManager, GetRoleDef), asCALL_THISCALL); ASSERT(r >= 0);
	r = engine->RegisterObjectMethod("CFactoryManager", "void EnqueueTask(uint8, CCircuitDef@, const AIFloat3& in, uint8, float)", asMETHOD(CFactoryManager, EnqueueTask), asCALL_THISCALL); ASSERT(r >= 0);
}

CFactoryScript::~CFactoryScript()
{
}

void CFactoryScript::Init()
{
	asIScriptModule* mod = script->GetEngine()->GetModule("main");
	int r = mod->SetDefaultNamespace("Factory"); ASSERT(r >= 0);
	InitModule(mod);
}

} // namespace circuit
