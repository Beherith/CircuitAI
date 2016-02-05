/*
 * FightAction.cpp
 *
 *  Created on: Jan 13, 2015
 *      Author: rlcevg
 */

#include "unit/action/FightAction.h"
#include "unit/UnitManager.h"
#include "terrain/PathFinder.h"
#include "CircuitAI.h"
#include "util/utils.h"

#include "AISCommands.h"
#include "Command.h"

namespace circuit {

using namespace springai;

CFightAction::CFightAction(CCircuitUnit* owner, float speed)
		: IUnitAction(owner, Type::FIGHT)
		, speed(speed)
		, pathIterator(0)
{
	CCircuitUnit* unit = static_cast<CCircuitUnit*>(ownerList);
	CCircuitDef* cdef = unit->GetCircuitDef();
	int squareSize = unit->GetManager()->GetCircuit()->GetPathfinder()->GetSquareSize();
	int size = std::max(cdef->GetUnitDef()->GetXSize(), cdef->GetUnitDef()->GetZSize());
	int incMod = std::max(size / 4, 1);
	if (cdef->IsPlane()) {
		incMod *= 5;
	} else if (cdef->IsAbleToFly()) {
		incMod *= 3;
	} else if (cdef->IsTurnLarge()) {
		incMod *= 2;
	}
	increment = incMod * DEFAULT_SLACK / squareSize + 1;
	minSqDist = squareSize * increment / 2;
	minSqDist *= minSqDist;
}

CFightAction::CFightAction(CCircuitUnit* owner, const std::shared_ptr<F3Vec>& pPath, float speed)
		: CFightAction(owner, speed)
{
	this->pPath = pPath;
}

CFightAction::~CFightAction()
{
	PRINT_DEBUG("Execute: %s\n", __PRETTY_FUNCTION__);
}

void CFightAction::Update(CCircuitAI* circuit)
{
	CCircuitUnit* unit = static_cast<CCircuitUnit*>(ownerList);
	int frame = circuit->GetLastFrame();
	const AIFloat3& pos = unit->GetPos(frame);
	int pathMaxIndex = pPath->size() - 1;

	int lastStep = pathIterator;
	float sqDistToStep = pos.SqDistance2D((*pPath)[pathIterator]);
	int step = std::min(pathIterator + increment, pathMaxIndex);
	float sqNextDistToStep = pos.SqDistance2D((*pPath)[step]);
	while ((sqNextDistToStep < sqDistToStep) && (pathIterator <  pathMaxIndex)) {
		pathIterator = step;
		sqDistToStep = sqNextDistToStep;
		step = std::min(pathIterator + increment, pathMaxIndex);
		sqNextDistToStep = pos.SqDistance2D((*pPath)[step]);
	}

	float stepSpeed;
	if ((pathIterator == lastStep) && ((int)sqDistToStep > minSqDist)) {
		auto commands = std::move(unit->GetUnit()->GetCurrentCommands());
		bool isEmpty = commands.empty();
		utils::free_clear(commands);
		if (isEmpty) {
			stepSpeed = MAX_SPEED;
		} else {
			return;
		}
	} else {
		stepSpeed = speed;
	}
	pathIterator = step;
	unit->GetUnit()->Fight((*pPath)[step], UNIT_COMMAND_OPTION_RIGHT_MOUSE_KEY, frame + FRAMES_PER_SEC * 60);
	unit->GetUnit()->SetWantedMaxSpeed(stepSpeed);

	for (int i = 2; (step < pathMaxIndex) && (i < 4); ++i) {
		step = std::min(step + increment, pathMaxIndex);
		unit->GetUnit()->Fight((*pPath)[step], UNIT_COMMAND_OPTION_RIGHT_MOUSE_KEY|UNIT_COMMAND_OPTION_SHIFT_KEY, frame + FRAMES_PER_SEC * 60 * i);
	}
}

void CFightAction::SetPath(const std::shared_ptr<F3Vec>& pPath, float speed)
{
	pathIterator = 0;
	this->pPath = pPath;
	this->speed = speed;
}

} // namespace circuit