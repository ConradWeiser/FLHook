#ifndef __MAIN_H__
#define __MAIN_H__ 1

#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <list>
#include <map>
#include <algorithm>
#include <FLHook.h>
#include <plugin.h>
#include <PluginUtilities.h>

using namespace std;


extern vector<uint> npcNames;

namespace NPC
{
	pub::AI::SetPersonalityParams HkMakePersonality();
	void CreateSwarmNPC(uint swarmTargetObj, Vector spawnPos, Matrix spawnRot, uint spawnSystem);
	void DeleteAllSwarmNPC();
	void BeginSwarmManuver();

	struct SwarmNpc
	{
		static pub::AI::DirectiveCancelOp cancelOp;
		pub::AI::DirectiveGotoOp currGotoOp;
		uint spaceObj;
		uint swarmTargetObj;
	};
}

double createRandomGuassianNumber();
Vector GeneratePointOnSphere(float radius);

extern list<NPC::SwarmNpc> spawnedNpcIds;

#endif
