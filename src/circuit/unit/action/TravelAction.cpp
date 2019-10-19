/*
 * TravelAction.cpp
 *
 *  Created on: Feb 16, 2016
 *      Author: rlcevg
 */

#include "unit/action/TravelAction.h"
#include "unit/CircuitUnit.h"
#include "unit/CircuitDef.h"
#include "task/UnitTask.h"
#include "util/utils.h"

namespace circuit {

using namespace springai;

ITravelAction::ITravelAction(CCircuitUnit* owner, Type type, int squareSize, float speed)
		: IUnitAction(owner, type)
		, speed(speed)
		, pathIterator(0)
		, isForce(true)
{
	CCircuitUnit* unit = static_cast<CCircuitUnit*>(ownerList);
	CCircuitDef* cdef = unit->GetCircuitDef();
	int size = std::max(cdef->GetUnitDef()->GetXSize(), cdef->GetUnitDef()->GetZSize());
	int incMod = std::max(size / 4, 1);
	if (cdef->IsPlane()) {
		incMod *= 8;
	} else if (cdef->IsAbleToFly()) {
		incMod *= 3;
	} else if (cdef->IsTurnLarge()) {
		incMod *= 2;
	}
	increment = incMod * DEFAULT_SLACK / squareSize + 1;
	minSqDist = squareSize * increment;  // / 2;
	minSqDist *= minSqDist;
}

ITravelAction::ITravelAction(CCircuitUnit* owner, Type type, const std::shared_ptr<F3Vec>& pPath, int squareSize, float speed)
		: ITravelAction(owner, type, squareSize, speed)
{
	this->pPath = pPath;
}

ITravelAction::~ITravelAction()
{
}

void ITravelAction::OnEnd()
{
	CCircuitUnit* unit = static_cast<CCircuitUnit*>(ownerList);
	unit->GetTask()->OnTravelEnd(unit);
	isActive = false;
}

void ITravelAction::SetPath(const std::shared_ptr<F3Vec>& pPath, float speed)
{
	pathIterator = 0;
	this->pPath = pPath;
	this->speed = speed;
	isForce = true;
	isFinished = false;
}

int ITravelAction::CalcSpeedStep(int frame, float& stepSpeed)
{
	CCircuitUnit* unit = static_cast<CCircuitUnit*>(ownerList);
	const AIFloat3& pos = unit->GetPos(frame);
	int pathMaxIndex = pPath->size() - 1;

	int lastStep = pathIterator;
	float sqDistToStep = pos.SqDistance2D((*pPath)[pathIterator]);
	int step = std::min(pathIterator + increment, pathMaxIndex);
	float sqNextDistToStep = pos.SqDistance2D((*pPath)[step]);
	while ((sqNextDistToStep < sqDistToStep) && (pathIterator < pathMaxIndex)) {
		pathIterator = step;
		sqDistToStep = sqNextDistToStep;
		step = std::min(pathIterator + increment, pathMaxIndex);
		sqNextDistToStep = pos.SqDistance2D((*pPath)[step]);
	}

	if (!isForce && (pathIterator == lastStep) && ((int)sqDistToStep > minSqDist)) {
		return -1;
	} else {
		stepSpeed = speed;
	}

	if ((int)sqDistToStep <= minSqDist) {
		pathIterator = step;
		isFinished = (pathIterator == pathMaxIndex);
	}

	isForce = false;
	return pathMaxIndex;
}

} // namespace circuit
