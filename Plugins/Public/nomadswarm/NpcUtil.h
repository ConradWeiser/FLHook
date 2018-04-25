#pragma once
#include "Main.h"

namespace NPC
{
	pub::AI::SetPersonalityParams HkMakePersonality();
	void CreateSwarmNPC(uint shiparch, uint loadout, uint swarmTargetObj, Vector spawnPos, uint spawnSystem);
	void DeleteAllSwarmNPC();
	void BeginSwarmManuver();
}