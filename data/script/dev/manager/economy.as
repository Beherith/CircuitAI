#include "../../define.as"


namespace Economy {

void AiOpenStrategy(const CCircuitDef@ facDef, const AIFloat3& in pos)
{
}

/*
 * struct SResourceInfo {
 *   const float current;
 *   const float storage;
 *   const float pull;
 *   const float income;
 * }
 */
void AiUpdateEconomy()
{
	const SResourceInfo@ metal = aiEconomyMgr.metal;
	const SResourceInfo@ energy = aiEconomyMgr.energy;
	aiEconomyMgr.isMetalEmpty = metal.current < metal.storage * 0.2f;
	aiEconomyMgr.isMetalFull = metal.current > metal.storage * 0.8f;
	if (ai.frame < 3 * MINUTE) {  // TODO: Replace by "is 1st factory finished" or raw storage value condition
		aiEconomyMgr.isEnergyEmpty = false;
		aiEconomyMgr.isEnergyStalling = energy.current < energy.storage * 0.3f;
	} else {
		aiEconomyMgr.isEnergyEmpty = energy.current < energy.storage * 0.2f;
		aiEconomyMgr.isEnergyStalling = aiEconomyMgr.isEnergyEmpty || ((energy.income < energy.pull) && (energy.current < energy.storage * 0.6f));
	}
//	aiEconomyMgr.isEnergyStalling = aiMin(metal.income - metal.pull, .0f)/* * 0.98f*/ > aiMin(energy.income - energy.pull, .0f);
}

}  // namespace Economy
