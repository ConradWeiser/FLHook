// Template for FLHookPlugin
// February 2016 by BestDiscoveryHookDevs2016
//
// This is a template with the bare minimum to have a functional plugin.
//
// This is free software; you can redistribute it and/or modify it as
// you wish without restriction. If you do then I would appreciate
// being notified and/or mentioned somewhere.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Includes
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
#include "Main.h"

#include "../hookext_plugin/hookext_exports.h"
#include <random>

static int set_iPluginDebug = 0;

/// A return code to indicate to FLHook if we want the hook processing to continue.
PLUGIN_RETURNCODE returncode;

void LoadSettings();

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	srand((uint)time(0));
	// If we're being loaded from the command line while FLHook is running then
	// set_scCfgFile will not be empty so load the settings as FLHook only
	// calls load settings on FLHook startup and .rehash.
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		if (set_scCfgFile.length()>0)
			LoadSettings();
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
	}
	return true;
}

/// Hook will call this function after calling a plugin function to see if we the
/// processing to continue
EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

bool bPluginEnabled = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Variables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
list<NPC::SwarmNpc> spawnedNpcIds;
vector<uint> npcNames;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Loading Settings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void LoadSettings()
{
	// Do nothing - This thing is hardcoded for fun
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool UserCmd_Swarm(uint iClientID, const wstring &wscCmd)
{

	//Verify that the user is in space
	uint playerShip;
	pub::Player::GetShip(iClientID, playerShip);
	if (!playerShip)
	{
		PrintUserCmdText(iClientID, L"ERR Not in space");
		return true;
	}

	// Get the players current location
	Vector pos;
	Matrix rot;
	uint system;
	pub::SpaceObj::GetLocation(playerShip, pos, rot);
	pub::SpaceObj::GetSystem(playerShip, system);

	// Create 40 NPCs 
	for(int i = 0; i < 40; i++)
	{
		NPC::CreateSwarmNPC(playerShip, pos, rot, system);
	}

	// Bring all ships to the ship radius + 500 meters directly towards the center of the sphere
	float shipRadius;
	Vector dunno;
	pub::SpaceObj::GetRadius(playerShip, shipRadius, dunno);
	for(auto& npc : spawnedNpcIds)
	{
		npc.currGotoOp.iGotoType = 0;
		npc.currGotoOp.iTargetID = playerShip;
		npc.currGotoOp.fRange = shipRadius + 500;
		npc.currGotoOp.goto_cruise = true;
		npc.currGotoOp.goto_no_cruise = false;
		pub::AI::SubmitDirective(npc.spaceObj, &npc.currGotoOp);

	}

	PrintUserCmdText(iClientID, L"OK");

	return true;
}

bool UserCmd_StopSwarm(uint iClientID, const wstring &wscCmd)
{
	// Delete all of the NPCs
	for(auto& npc : spawnedNpcIds)
	{
		pub::SpaceObj::Destroy(npc.spaceObj, DestroyType::VANISH);
	}

	PrintUserCmdText(iClientID, L"Deleted %i NPCs", spawnedNpcIds.size());

	spawnedNpcIds.clear();
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Actual Code
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Clean up when a client disconnects */
void ClearClientInfo(uint iClientID)
{
	returncode = DEFAULT_RETURNCODE;
}

bool swarmRotating = false;
bool cmdRun = false;
void HkTimerCheckKick()
{
	if(!swarmRotating && cmdRun)
	{
		// Check any NPC for distance. The front object works
		NPC::SwarmNpc npc = spawnedNpcIds.front();
		Vector npcPos, targetPos;
		Matrix npcRot, targetRot;

		float shipRadius;
		Vector dunno;
		pub::SpaceObj::GetRadius(npc.swarmTargetObj, shipRadius, dunno);

		pub::SpaceObj::GetLocation(npc.spaceObj, npcPos, npcRot);
		pub::SpaceObj::GetLocation(npc.swarmTargetObj, targetPos, targetRot);

		if(abs(HkDistance3D(npcPos, targetPos)) < shipRadius + 600)
		{

			ConPrint(L"Yay\n");
			// If one NPC is within the proper range, they all probably are. Nuke all of the goto operations and prepare the new complex order
			for(auto& currNpc : spawnedNpcIds)
			{
				pub::AI::DirectiveCancelOp op;
				pub::AI::SubmitDirective(currNpc.spaceObj, &op);

				//Generate four points on the sphere and set up the goto op
				pub::AI::DirectiveGotoOp goOp;
				goOp.iGotoType = 2;
				goOp.vSpline[1] = GeneratePointOnSphere(shipRadius + 600);
				goOp.vSpline[2] = GeneratePointOnSphere(shipRadius + 600);
				goOp.vSpline[3] = GeneratePointOnSphere(shipRadius + 600);
				goOp.vSpline[4] = GeneratePointOnSphere(shipRadius + 600);
				goOp.goto_no_cruise = true;
				goOp.fThrust = -1;

				pub::AI::SubmitDirective(currNpc.spaceObj, &goOp);
			}

			swarmRotating = true;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Client command processing
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef bool(*_UserCmdProc)(uint, const wstring &, const wstring &, const wchar_t*);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
	wchar_t *usage;
};

/**
This function is called by FLHook when a user types a chat string. We look at the
string they've typed and see if it starts with one of the above commands. If it
does we try to process it.
*/
bool UserCmd_Process(uint iClientID, const wstring &args)
{
	returncode = DEFAULT_RETURNCODE;
	if(args.find(L"/swarm") == 0)
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		UserCmd_Swarm(iClientID, args);
		cmdRun = true;
		return true;
	}
	else if(args.find(L"/stopswarm") == 0)
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		UserCmd_StopSwarm(iClientID, args);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Functions to hook
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "Nomad Swarm by Remnant";
	p_PI->sShortName = "swarm";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTimerCheckKick, PLUGIN_HkTimerCheckKick, 0));

	return p_PI;
}

double createRandomGuassianNumber()
{
	random_device rd;
	mt19937 gen(rd());
	normal_distribution<> dist{ 0, 1 };

	return round(dist(gen));
}

Vector GeneratePointOnSphere(float radius)
{
	float x = createRandomGuassianNumber();
	float y = createRandomGuassianNumber();
	float z = createRandomGuassianNumber();

	x = x * (1 / sqrt((x * x) + (y * y) + (z * z)));
	y = y * (1 / sqrt((x * x) + (y * y) + (z * z)));
	z = z * (1 / sqrt((x * x) + (y * y) + (z * z)));

	// This sphere's radius should be where the NPC stopped on it's approach
	x = x * radius;
	y = y * radius;
	z = z * radius;

	Vector newVec;
	newVec.x = x;
	newVec.y = y;
	newVec.z = z;

	return newVec;
}
