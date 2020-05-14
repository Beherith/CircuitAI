#include "../role.as"


namespace Builder {

//AIFloat3 lastPos;

IUnitTask@ MakeTask(CCircuitUnit@ unit)
{
//	aiDelPoint(lastPos);
//	lastPos = unit.GetPos(ai.GetLastFrame());
//	aiAddPoint(lastPos, "task");

//	IUnitTask@ task = builderMgr.DefaultMakeTask(unit);
//	if ((task !is null) && (task.GetType() == 5)) {  // Type::BUILDER
//		aiAddPoint(task.GetPos(), "task");
//	}
//	return task;
	return builderMgr.DefaultMakeTask(unit);
}

}  // namespace Builder
