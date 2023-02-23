#include "StatsDisplay.h"
#include "../Basic/Texthook/Texthook.h"
#include "../Basic/Framehook/Framehook.h"
#include "../Basic/Boxhook/Boxhook.h"
#include "../../D2Ptrs.h"
#include "../../BH.h"

using namespace Drawing;

StatsDisplay *StatsDisplay::display;

StatsDisplay::StatsDisplay(std::string name) {
	int yPos = 20;
	int width = 350;

	InitializeCriticalSection(&crit);
	SetY(yPos);
	SetXSize(width);

	LoadConfig();

	SetName(name);
	SetActive(true);
	SetMinimized(true);

	BH::config->ReadKey("Character Stats", "VK_8", statsKey);
	display = this;
}

StatsDisplay::~StatsDisplay() {
	Lock();
	// Remove all hooks associated with the display
	while (Hooks.size() > 0) {
		delete (*Hooks.begin());
	}
	Unlock();
	DeleteCriticalSection(&crit);
}

void StatsDisplay::LoadConfig(){
	int height = 450 + 8 * 3 + 16 * 5;
	customStats.clear();

	BH::config->ReadToggle("Stats on Right", "None", false, Toggles["Stats on Right"]);

	vector<pair<string, string>> stats;
	BH::config->ReadMapList("Stat Screen", stats);
	for (unsigned int i = 0; i < stats.size(); i++) {
		std::transform(stats[i].first.begin(), stats[i].first.end(), stats[i].first.begin(), ::tolower);
		if (StatMap.count(stats[i].first) > 0) {
			StatProperties *sp = StatMap[stats[i].first];
			DisplayedStat *customStat = new DisplayedStat();
			customStat->name = stats[i].first;
			customStat->useValue = false;
			std::transform(customStat->name.begin(), customStat->name.end(), customStat->name.begin(), ::tolower);
			// Getting rid of the check for sp->saveParamBits > 0 to display weapon mastery values
			// if a param is supplied it will be used
			int num = -1;
			stringstream ss(Trim(stats[i].second));
			if (!(ss >> num).fail() && num > 0) {
				customStat->useValue = true;
				customStat->value = num;
			}
			customStats.push_back(customStat);
		}
	}
	if (customStats.size() > 0) {
		height += (customStats.size() * 16) + 8;
	}

	int xPos = Toggles["Stats on Right"].state ?
		*p_D2CLIENT_ScreenSizeX - 10 - GetXSize() : 10;
	SetX(xPos);
	SetYSize(height);
}

void StatsDisplay::SetX(unsigned int newX) {
	if (newX >= 0 && newX <= Hook::GetScreenWidth()) {
		Lock();
		x = newX;
		Unlock();
	}
}

void StatsDisplay::SetY(unsigned int newY) {
	if (newY >= 0 && newY <= Hook::GetScreenHeight()) {
		Lock();
		y = newY;
		Unlock();
	}
}

void StatsDisplay::SetXSize(unsigned int newXSize) {
	if (newXSize >= 0 && newXSize <= (Hook::GetScreenWidth() - GetX())) {
		Lock();
		xSize = newXSize;
		Unlock();
	}
}

void StatsDisplay::SetYSize(unsigned int newYSize) {
	if (newYSize >= 0 && newYSize <= (Hook::GetScreenHeight() - GetY())) {
		Lock();
		ySize = newYSize;
		Unlock();
	}
}

bool StatsDisplay::InRange(unsigned int x, unsigned int y) {
	return IsActive() &&
		x >= GetX() && y >= GetY() &&
		x <= GetX() + GetXSize() && y <= GetY() + GetYSize();
}

void StatsDisplay::Draw() {
	display->Lock();
	display->OnDraw();
	display->Unlock();
}

void StatsDisplay::OnDraw() {
	UnitAny *unit = D2CLIENT_GetPlayerUnit();
	bool isMerc = false;
	if (!unit)
		return;
	int column1 = GetX() + 5;
	int column2 = column1 + GetXSize() / 2;

	if (!IsMinimized()) {
		if (D2CLIENT_GetUIState(UI_MERC)) {
			unit = D2CLIENT_GetMercUnit();
			isMerc = true;
		}
		for(std::list<Hook*>::iterator it = Hooks.begin(); it != Hooks.end(); it++)
			(*it)->OnDraw();

		int y = GetY();
		RECT pRect;
		pRect.left = GetX();
		pRect.top = y;
		pRect.right = x + GetXSize();
		pRect.bottom = y + GetYSize();

		Drawing::Boxhook::Draw(GetX(),GetY(), GetXSize(), GetYSize(), White, Drawing::BTBlack);
		Drawing::Framehook::DrawRectStub(&pRect);

		Texthook::Draw(column1, (y += 8), None, 6, Gold,
				"Nombre :ÿc0 %s",
				isMerc ? "ÿc;Mercenario" : unit->pPlayerData->szName);
		Texthook::Draw(pRect.right - 5, y, Right, 6, Gold,
				L"Nivel:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_LEVEL, 0));
		Texthook::Draw(pRect.right - 5, y + 12, Right, 6, Gold,
				L"Experiencia Adicional:ÿc: %d%%",
				(int)D2COMMON_GetUnitStat(unit, STAT_ADDEXPERIENCE, 0));

		y += 8;

		BnetData* pData = (*p_D2LAUNCH_BnData);
		int xPacMultiplier = pData->nCharFlags & PLAYER_TYPE_EXPANSION ? 2 : 1;
		int resPenalty[3] = { RES_PENALTY_CLS_NORM, RES_PENALTY_CLS_NM, RES_PENALTY_CLS_HELL };
		int penalty = resPenalty[D2CLIENT_GetDifficulty()] * xPacMultiplier;
		int fMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXFIRERESIST, 0) + 75;
		int cMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXCOLDRESIST, 0) + 75;
		int lMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXLIGHTNINGRESIST, 0) + 75;
		int pMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXPOISONRESIST, 0) + 75;
		int mMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXMAGICDMGREDUCTPCT, 0) + 75;
		int pLengthReduce = (int)D2COMMON_GetUnitStat(unit, STAT_POISONLENGTHREDUCTION, 0);

		Texthook::Draw(column1, (y += 16), None, 6, Red, L"ÿc4Resistencia al Fuego:ÿc1 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_FIRERESIST, 0) + penalty, fMax);
		Texthook::Draw(column1, (y += 16), None, 6, Blue, L"ÿc4Resistencia al Hielo:ÿc3 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_COLDRESIST, 0) + penalty, cMax);
		Texthook::Draw(column1, (y += 16), None, 6, Yellow, L"ÿc4Resistencia al Relampago:ÿc9 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGRESIST, 0) + penalty, lMax);
		Texthook::Draw(column1, (y += 16), None, 6, Gold, L"Resistencia al Veneno:ÿc2 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_POISONRESIST, 0) + penalty, mMax);
		Texthook::Draw(column1, (y += 16), None, 6, Gold, L"Resistencia Magica:ÿc8 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTIONPCT, 0) + penalty, pMax);
		y += 8;

		int fAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_FIREABSORB, 0);
		int fAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_FIREABSORBPERCENT, 0);
		int cAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_COLDABSORB, 0);
		int cAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_COLDABSORBPERCENT, 0);
		int lAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORB, 0);
		int lAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORBPERCENT, 0);
		int mAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICABSORB, 0);
		int mAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICABSORBPERCENT, 0);
		int pAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_POISONABSORB, 0);
		int pAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_POISONABSORBPERCENT, 0);
		int phAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_PHYSABS, 0);
		int phbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_PHYSABSPERCENT, 0);
	
		Texthook::Draw(column1, (y += 16), None, 6, Red, L"ÿc4Absorción Elemental: ÿc1%dÿc0  ÿc3%dÿc0  ÿc9%dÿc0  ÿc8%dÿc0  ÿc2%dÿc0  ÿc4%dÿc0", fAbsorb, cAbsorb, lAbsorb, mAbsorb, pAbsorb, phAbsorb);
		Texthook::Draw(column1, (y += 16), None, 6, Red, L"ÿc4Absorción Porcentual: ÿc1%d%c  ÿc3%d%c  ÿc9%d%c  ÿc8%d%c  ÿc2%d%c  ÿc4%d%c", fAbsorbPct, '%', cAbsorbPct, '%', lAbsorbPct, '%',mAbsorbPct, '%', pAbsorbPct, '%', phbsorbPct, '%');
		y += 8;

		int dmgReduction = (int)D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTION, 0);
		int dmgReductionPct = (int)D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTIONPCT, 0);
		int magReduction = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTION, 0);
		int magReductionPct = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTIONPCT, 0);
		Texthook::Draw(column1, (y += 16), None, 6, Tan, L"ÿc4Daño Reducido: ÿc7%dÿc0/ÿc7%d%c", dmgReduction, dmgReductionPct, '%');
		y += 8;

		int fMastery = (int)D2COMMON_GetUnitStat(unit, STAT_FIREMASTERY, 0);
		int cMastery = (int)D2COMMON_GetUnitStat(unit, STAT_COLDMASTERY, 0);
		int lMastery = (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGMASTERY, 0);
		int pMastery = (int)D2COMMON_GetUnitStat(unit, STAT_POISONMASTERY, 0);
		int mMastery = (int)D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICDMGMASTERY, 0);

		int fPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYFIRERESREDUC, 0);
		int cPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYCOLDRESREDUC, 0);
		int lPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYLIGHTNRESREDUC, 0);
		int pPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYPSNRESREDUC, 0);
		int mPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICRESREDUC, 0);

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Maestrias Elementales:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
				fMastery, cMastery, lMastery, pMastery, mMastery);
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Penetraciones Elementales:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
				fPierce, cPierce, lPierce, pPierce, mPierce);
		y += 8;

		int classNum = pData->nCharClass;
		auto classArMod = CharList[classNum]->toHitFactor - 35;
		int dexAR = (int)D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0) * 5 + classArMod;
		int gearAR = (int)D2COMMON_GetUnitStat(unit, STAT_ATTACKRATING, 0);

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Ar Base :ÿc5 Dex:ÿc0 %dÿc5 Equipo:ÿc0% dÿc5 Total:ÿc0 %d",
				dexAR, gearAR, dexAR + gearAR);

		int gearDef = (int)D2COMMON_GetUnitStat(unit, STAT_DEFENSE, 0);
		int dexDef = (int)D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0) / 4;
		
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Def Base:ÿc5 Dex:ÿc0 %dÿc5 Equipo:ÿc0 %dÿc5 total:ÿc0 %d",
				dexDef, gearDef, dexDef + gearDef);

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Daño Base:ÿc5 1h:ÿc0 %d-%dÿc5 2h:ÿc0 %d-%d",
				(int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMDAMAGE, 0),
				(int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMDAMAGE, 0),
				(int)D2COMMON_GetUnitStat(unit, STAT_SECONDARYMINIMUMDAMAGE, 0),
				(int)D2COMMON_GetUnitStat(unit, STAT_SECONDARYMAXIMUMDAMAGE, 0));

		y += 8;

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Penetración Física:ÿc4 %d%%",
			(int)D2COMMON_GetUnitStat(unit, STAT_PHYSPIERCEPERCT, 0));

		Texthook::Draw(column2, y, None, 6, Gold,
			L"Evita Curación: ÿc1%d",
			(int)D2COMMON_GetUnitStat(unit, STAT_PREVENTMONSTERHEAL, 0));

		y += 8;

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"V. Lanzamiento:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_FASTERCAST, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
				L"Bloqueo:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_FASTERBLOCK, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"V. Movimiento:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_FASTERRUNWALK, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
				L"Recuperación:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_FASTERHITRECOVERY, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"V. Ataque:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_IAS, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
				L"V. Acción:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_ATTACKRATE, 0));
		
		y += 8;


		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Golpe Triturador:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_CRUSHINGBLOW, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
				L"Heridas Abiertas: ÿc0%d",
				(int)D2COMMON_GetUnitStat(unit, STAT_OPENWOUNDS, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Azote Mortal:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_DEADLYSTRIKE, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
				L"Golpe Critico: ÿc0%d",
				(int)D2COMMON_GetUnitStat(unit, STAT_CRITICALSTRIKE, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Robo de Vida:ÿc1 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_LIFELEECH, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
				L"Robo de Mana:ÿc3 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_MANALEECH, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Perforación de Proyectil:ÿc0 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_PIERCINGATTACK, 0) +
				(int)D2COMMON_GetUnitStat(unit, STAT_PIERCE, 0)
				);
		Texthook::Draw(column2, y, None, 6, Gold,
			L"No puedes ser Congelado: ÿc3%d",
			(int)D2COMMON_GetUnitStat(unit, STAT_CANNOTBEFROZEN, 0));

		y += 8;

		int minFire = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMFIREDAMAGE, 0);
		int maxFire = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMFIREDAMAGE, 0);
		int minLight = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMLIGHTNINGDAMAGE, 0);
		int maxLight = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMLIGHTNINGDAMAGE, 0);
		int minCold = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMCOLDDAMAGE, 0);
		int maxCold = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMCOLDDAMAGE, 0);
		int minPoison = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMPOISONDAMAGE, 0);
		int maxPoison = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMPOISONDAMAGE, 0);
		int poisonLength = (int)D2COMMON_GetUnitStat(unit, STAT_POISONDAMAGELENGTH, 0);
		int poisonLengthOverride = (int)D2COMMON_GetUnitStat(unit, STAT_SKILLPOISONOVERRIDELEN, 0);
		if (poisonLengthOverride > 0) {
			poisonLength = poisonLengthOverride;
		}
		int minMagic = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMMAGICALDAMAGE, 0);
		int maxMagic = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMMAGICALDAMAGE, 0);
		int addedPhys = (int)D2COMMON_GetUnitStat(unit, STAT_ADDSDAMAGE, 0);
		Texthook::Draw(column1, (y += 13), None, 6, Gold,
				L"Daño Adicional:ÿc0 %d",
				addedPhys);
		Texthook::Draw(column1, (y += 15), None, 6, Orange,
				"%d-%d",
				minMagic, maxMagic);
		Texthook::Draw(column1, (y += 15), None, 6, Red,
				"%d-%d",
				minFire, maxFire);
		Texthook::Draw(column1, (y += 15), None, 6, Blue,
				"%d-%d",
				minCold, maxCold);
		Texthook::Draw(column1, (y += 15), None, 6, Yellow,
				"%d-%d",
				minLight, maxLight);
		Texthook::Draw(column1, (y += 15), None, 6, Green,
				"%d-%d durante %.1fs",
				(int)(minPoison / 256.0 * poisonLength),
				(int)(maxPoison / 256.0 * poisonLength),
				poisonLength / 25.0);

		y += 8;

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Suerte:ÿc3 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_MAGICFIND, 0)
				);
		Texthook::Draw(column2, y, None, 6, Gold,
				L"Riqueza:ÿc9 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_GOLDFIND, 0));

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
				L"Oro en Baul:ÿc9 %d",
				(int)D2COMMON_GetUnitStat(unit, STAT_GOLDBANK, 0));

		//int cowKingKilled = D2COMMON_GetQuestFlag(D2CLIENT_GetQuestInfo(), 4, 10);
		//Texthook::Draw(column2, y, None, 6, Gold,
		//L"Cow King:ÿc0 %s", cowKingKilled ? L"killed" : L"alive");

		if (customStats.size() > 0) {
			y += 8;
			for (unsigned int i = 0; i < customStats.size(); i++) {
				int secondary = customStats[i]->useValue ? customStats[i]->value : 0;
				int stat = (int)D2COMMON_GetUnitStat(unit, STAT_NUMBER(customStats[i]->name), secondary);
				if (secondary > 0) {
					Texthook::Draw(column1, (y += 16), None, 6, Gold, "%s[%d]:ÿc0 %d",
							customStats[i]->name.c_str(), secondary, stat);
				} else {
					Texthook::Draw(column1, (y += 16), None, 6, Gold, "%s:ÿc0 %d",
							customStats[i]->name.c_str(), stat);
				}
			}
		}
	}
}

bool StatsDisplay::KeyClick(bool bUp, BYTE bKey, LPARAM lParam) {
	display->Lock();
	bool block = display->OnKey(bUp, bKey, lParam);
	display->Unlock();
	return block;
}

bool StatsDisplay::OnKey(bool up, BYTE kkey, LPARAM lParam) {
	UnitAny *unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return false;

	if (IsMinimized()) {
		if (!up && kkey == statsKey) {
			LoadConfig();
			SetMinimized(false);
			return true;
		}
	} else {
		if (!up && (kkey == statsKey || kkey == VK_ESCAPE)) {
			SetMinimized(true);
			return true;
		}
	}
	return false;
}

bool StatsDisplay::Click(bool up, unsigned int mouseX, unsigned int mouseY) {
	display->Lock();
	bool block = display->OnClick(up, mouseX, mouseY);
	display->Unlock();
	return block;
}

bool StatsDisplay::OnClick(bool up, unsigned int x, unsigned int y) {
	UnitAny *unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return false;

	if (!IsMinimized() && InRange(x, y)) {
		SetMinimized(true);
		return true;
	}
	return false;
}
