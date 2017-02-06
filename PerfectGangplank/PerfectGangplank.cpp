#include "stdafx.h"
#include "PluginSDK.h"
#include "Color.h"

PluginSetup("Perfect Gangplank - Dave Chappelle");

IMenu* MainMenu;
IMenu* QMenu;
IMenu* WMenu;
IMenu* EMenu;
IMenu* RMenu;
IMenu* Misc;
IMenu* Drawings;
IMenuOption* ComboQ;
IMenuOption* AutoQ;
IMenuOption* FarmQ;
IMenuOption* HarassManaE;
IMenuOption* HarassManaW;
IMenuOption* FarmE;
IMenuOption* FarmEHit;
IMenuOption* HarassManaQ;
IMenuOption* FarmW;
IMenuOption* ComboW;
IMenuOption* QGapCloser;
IMenuOption* AutoE;
IMenuOption* AutoUlt;
IMenuOption* ComboE;
IMenuOption* ComboR;
IMenuOption* UltEnemies;
IMenuOption* DrawReady;
IMenuOption* DrawQ;
IMenuOption* DrawW;
IMenuOption* DrawE;
IMenuOption* DrawR;

ISpell2* Q;
ISpell2* W;
ISpell2* E;
ISpell2* R;


std::vector<IUnit*> barrelList;



void  Menu()
{
	MainMenu = GPluginSDK->AddMenu("Perfect Gangplank");
	QMenu = MainMenu->AddMenu("Q Settings");
	WMenu = MainMenu->AddMenu("W Settings");
	EMenu = MainMenu->AddMenu("E Settings");
	RMenu = MainMenu->AddMenu("R Settings");
	Drawings = MainMenu->AddMenu("Drawings");

	FarmQ = QMenu->CheckBox("Use Q Farm", true);
	HarassManaQ = QMenu->AddInteger("Mana Manager(%)(AutoQ)", 1, 100, 60);


	DrawReady = Drawings->CheckBox("Draw Only Ready Spells", true);

	DrawQ = Drawings->CheckBox("Draw Q", true);
	DrawW = Drawings->CheckBox("Draw W", true);
	DrawE = Drawings->CheckBox("Draw E", true);
	DrawR = Drawings->CheckBox("Draw R", true);
}
void LoadSpells()
{
	Q = GPluginSDK->CreateSpell2(kSlotQ, kTargetCast, false, false, static_cast<eCollisionFlags>(kCollidesWithYasuoWall));
	W = GPluginSDK->CreateSpell2(kSlotW, kConeCast, false, false, static_cast<eCollisionFlags>(kCollidesWithNothing));
	E = GPluginSDK->CreateSpell2(kSlotE, kCircleCast, false, false, static_cast<eCollisionFlags>(kCollidesWithNothing));
	R = GPluginSDK->CreateSpell2(kSlotR, kCircleCast, false, false, static_cast<eCollisionFlags>(kCollidesWithNothing));

}

void Qminion()
{
	if (!Q->IsReady())
		return;

	for (auto minion : GEntityList->GetAllMinions(false, true, true))
	{
		if (minion->GetTeam() != GEntityList->Player()->GetTeam()
			&& !minion->IsDead()
			&& minion->GetHealth() <= (GDamage->GetSpellDamage(GEntityList->Player(), minion, kSlotQ)
				+ GDamage->GetAutoAttackDamage(GEntityList->Player(), minion, true))
			&& GEntityList->Player()->IsValidTarget(minion, GEntityList->Player()->AttackRange()))
		{
			if (Q->CastOnPlayer())
			{
				GOrbwalking->SetOverrideTarget(minion);
				return;
			}
		}
	}
}


// W heal
void HealManager()
{
	if (GEntityList->Player()->HasBuff("IsRecalling")) return;
	if (W->IsReady() && GEntityList->Player()->HealthPercent() <= 15 && GEntityList->Player()->ManaPercent() >= 40)
	{
		W->CastOnPlayer();
	}
}


/*
* Credits to Katten for this logic
*/
PLUGIN_EVENTD(void) OnBuffAdd(IUnit* Source, void* BuffData)
{
	auto player = GEntityList->Player();

	if (BuffData == nullptr || !W->IsReady() || Source != player || player->IsDead())
		return;

	auto type = GBuffData->GetBuffType(BuffData);

	// filter removable buffs here
	if (type == BUFF_Blind || type == BUFF_Charm || type == BUFF_Flee ||
		type == BUFF_Polymorph || type == BUFF_Snare || type == BUFF_Stun ||
		type == BUFF_Taunt || type == BUFF_Suppression)
	{
		W->CastOnPlayer();
	}
}


void Combo()
{
	if (ComboQ->Enabled())
	{
		if (Q->IsReady())
		{
			auto target = GTargetSelector->FindTarget(QuickestKill, SpellDamage, Q->Range());
			Q->CastOnTarget(target, kHitChanceHigh);
		}
	}
	if (ComboR->Enabled())
	{
		if (R->IsReady())
		{
			auto target = GTargetSelector->FindTarget(QuickestKill, SpellDamage, R->Range());
			int enemies = 0;
			Vec3 pos = Vec3();
			R->FindBestCastPosition(false, true, pos, enemies);
			if (enemies >= UltEnemies->GetInteger())
				R->CastOnPosition(pos);
		}
	}
}
void Farm()
{
	if (GEntityList->Player()->ManaPercent() >= HarassManaQ->GetInteger() && FarmQ->Enabled() && GOrbwalking->GetOrbwalkingMode() == kModeLaneClear)
	{
		Qminion();
	}
}


void Auto()
{
	if (W->IsReady())
	{
			HealManager();
	}

}
PLUGIN_EVENT(void) OnGameUpdate()
{
	if (GOrbwalking->GetOrbwalkingMode() == kModeCombo)
	{
		Combo();
	}
	if (GOrbwalking->GetOrbwalkingMode() == kModeLaneClear)
	{
		Farm();
	}
	if (GOrbwalking->GetOrbwalkingMode() == kModeLastHit)
	{
		Qminion();
	}
	Auto();

}


PLUGIN_EVENT(void) onCreateObject(IUnit* Object)
{
	auto name = Object->GetObjectName();

	if (!strcmp(Object->GetObjectName(), "Barrel"))
	{
		barrelList.push_back(Object);
	}
}


PLUGIN_EVENT(void) OnRender()
{
	for (auto unit : GEntityList->GetAllUnits())
	{
		if (unit == nullptr || unit->UnitFlags() != FL_CREEP)
			continue;

		auto name = unit->GetObjectName();

		if (name == nullptr)
			continue;

		if (!strcmp(name, "Barrel"))
		{
			Vec2 w2s;
			GGame->Projection(unit->GetPosition(), &w2s);
			GRender->DrawOutlinedCircle(w2s, Vec4(255, 255, 0, 255), W->Range());
		}
	}

	if (DrawReady->Enabled())
	{
		if (Q->IsReady() && DrawQ->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), Q->Range()); }

		if (E->IsReady() && DrawE->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), E->Range()); }

		if (W->IsReady() && DrawW->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), W->Range()); }

		if (R->IsReady() && DrawW->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), R->Range()); }

	}
	else
	{
		if (DrawQ->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), Q->Range()); }

		if (DrawE->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), E->Range()); }

		if (DrawW->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), W->Range()); }

		if (DrawR->Enabled()) { GRender->DrawOutlinedCircle(GEntityList->Player()->GetPosition(), Vec4(255, 255, 0, 255), R->Range()); }
	}
}
PLUGIN_API void OnLoad(IPluginSDK* PluginSDK)
{

	PluginSDKSetup(PluginSDK);
	Menu();
	LoadSpells();

	GEventManager->AddEventHandler(kEventOnCreateObject, onCreateObject);
	GEventManager->AddEventHandler(kEventOnGameUpdate, OnGameUpdate);
	GEventManager->AddEventHandler(kEventOnBuffAdd, OnBuffAdd);
	GEventManager->AddEventHandler(kEventOnRender, OnRender);
	GRender->NotificationEx(Color::DarkMagenta().Get(), 2, true, true, "PerfectGangplank V0.1 - Loaded");
}

PLUGIN_API void OnUnload()
{
	MainMenu->Remove();

	GEventManager->RemoveEventHandler(kEventOnCreateObject, onCreateObject);
	GEventManager->RemoveEventHandler(kEventOnBuffAdd, OnBuffAdd);
	GEventManager->RemoveEventHandler(kEventOnGameUpdate, OnGameUpdate);
	GEventManager->RemoveEventHandler(kEventOnRender, OnRender);

}