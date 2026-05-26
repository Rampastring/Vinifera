/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Vinifera replacement of the save file header.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#pragma once

#include "vinifera_globals.h"

#include <minwindef.h>
#include <objidl.h>
#include <vector>


class ViniferaSaveVersionInfo
{
public:
    enum {
        ID_SCENARIO_DESCRIPTION = 2,
        ID_PLAYER_HOUSE = 3,
        ID_VERSION = 9,
        ID_INTERNAL_VERSION = 16,
        ID_START_TIME = 12,
        ID_LAST_SAVE_TIME = 13,
        ID_PLAY_TIME = 10,
        ID_EXECUTABLE_NAME = 18,
        ID_PLAYER_NAME = 4,
        ID_PLAYER_NAME2 = 8,
        ID_SCENARIO_NUMBER = 100,
        ID_CAMPAIGN = 101,
        ID_GAMETYPE = 102,

        ID_VINIFERA_VERSION = 105,
        ID_VINIFERA_COMMIT_HASH = 106,
        ID_PLAYTHROUGH_ID = 107,
        ID_DIFFICULTY = 108,
        ID_TOTAL_PLAY_TIME = 109,

        ID_MISSION_INTERNAL_NAME = 120,
        ID_PLAYER_SIDE = 121,
        ID_CLIENT_DIFFICULTY = 122,
        ID_SPAWNER_GLOBAL_FLAG_VALUES = 123,
        ID_IS_CHEAT_SESSION = 124,
        ID_BONUS_NAME = 125
    };

public:
    ViniferaSaveVersionInfo();

    void Set_Version(int num);
    int Get_Version() const;

    void Set_Internal_Version(int num);
    int Get_Internal_Version() const;

    void Set_Scenario_Description(const char* desc);
    const char* Get_Scenario_Description() const;

    void Set_Player_House(const char* name);
    const char* Get_Player_House() const;

    void Set_Campaign_Number(int num);
    int Get_Campaign_Number() const;

    void Set_Scenario_Number(int num);
    int Get_Scenario_Number() const;

    void Set_Unknown_String(const char* name);
    const char* Get_Unknown_String() const;

    void Set_Player_Name(const char* name);
    const char* Get_Player_Name() const;

    void Set_Executable_Name(const char* name);
    const char* Get_Executable_Name() const;

    void Set_Start_Time(FILETIME& time);
    FILETIME Get_Start_Time() const;

    void Set_Play_Time(FILETIME& time);
    FILETIME Get_Play_Time() const;

    void Set_Last_Time(FILETIME& time);
    FILETIME Get_Last_Time() const;

    void Set_Game_Type(int id);
    int Get_Game_Type() const;

    void Set_Vinifera_Version(int num);
    int Get_Vinifera_Version() const;

    void Set_Vinifera_Commit_Hash(const char* hash);
    const char* Get_Vinifera_Commit_Hash() const;

    void Set_Playthrough_ID(int num);
    int Get_Playthrough_ID() const;

    void Set_Difficulty(int num);
    int Get_Difficulty() const;

    void Set_Total_Play_Time(int num);
    int Get_Total_Play_Time() const;

    void Set_Mission_Internal_Name(const char* name);
    const char* Get_Mission_Internal_Name() const;

    void Set_Player_Side(int side);
    int Get_Player_Side() const;

    void Set_Client_Difficulty(int clientdifficulty);
    int Get_Client_Difficulty() const;

    void Set_Spawner_Global_Flag_Values(std::vector<int>& values);
    void Get_Spawner_Global_Flag_Values(std::vector<int>& values) const;

    void Set_Is_Cheat_Session(bool value);
    bool Get_Is_Cheat_Session() const;

    void Set_Bonus_Name(const char* name);
    const char* Get_Bonus_Name() const;

    HRESULT Save(IStorage* storage);
    HRESULT Load(IStorage* storage);

private:
    HRESULT Load_String(IStorage* storage, int id, char* string);
    HRESULT Load_String_Set(IPropertySetStorage* storageset, int id, char* string);

    HRESULT Load_Int(IStorage* storage, int id, int* integer);
    HRESULT Load_Int_Set(IPropertySetStorage* storageset, int id, int* integer);

    HRESULT Load_Bool(IStorage* storage, int id, bool* boolean);
    HRESULT Load_Bool_Set(IPropertySetStorage* storageset, int id, bool* boolean);

    HRESULT Load_Int_Vector(IStorage* storage, int id, std::vector<int>& values);
    HRESULT Load_Int_Vector_Set(IPropertySetStorage* storageset, int id, std::vector<int>& values);

    HRESULT Load_Int_Array(IStorage* storage, int id, int* array, size_t size);
    HRESULT Load_Int_Array_Set(IPropertySetStorage* storageset, int id, int* array, size_t size);

    HRESULT Save_String(IStorage* storage, int id, char* string);
    HRESULT Save_String_Set(IPropertySetStorage* storageset, int id, const char* string);

    HRESULT Save_Int(IStorage* storage, int id, int integer);
    HRESULT Save_Int_Set(IPropertySetStorage* storageset, int id, int integer);

    HRESULT Save_Bool(IStorage* storage, int id, bool boolean);
    HRESULT Save_Bool_Set(IPropertySetStorage* storageset, int id, bool boolean);

    HRESULT Save_Int_Vector(IStorage* storage, int id, const std::vector<int>& values);
    HRESULT Save_Int_Vector_Set(IPropertySetStorage* storageset, int id, const std::vector<int>& values);

    HRESULT Save_Int_Array(IStorage* storage, int id, const int* array, size_t size);
    HRESULT Save_Int_Array_Set(IPropertySetStorage* storageset, int id, const int* array, size_t size);

    HRESULT Load_Time(IStorage* storage, int id, FILETIME* time);
    HRESULT Load_Time_Set(IPropertySetStorage* storageset, int id, FILETIME* time);

    HRESULT Save_Time(IStorage* storage, int id, FILETIME* time);
    HRESULT Save_Time_Set(IPropertySetStorage* storageset, int id, FILETIME* time);

private:
    int InternalVersion;
    int Version;
    char ScenarioDescription[128];
    char PlayerHouse[64];
    int CampaignNumber;
    int ScenarioNumber;
    char UnknownString[260];
    char PlayerName[64];
    char ExecutableName[260];
    FILETIME StartTime;
    FILETIME PlayTime;
    FILETIME LastSaveTime;
    int GameType;

    /**
     *  New Vinifera fields.
     */
    int ViniferaVersion;
    char ViniferaCommitHash[40];
    int PlaythroughID;
    int Difficulty;
    int TotalPlayTime;

    /**
     *  DTA-specific fields for client mission progression and other features.
     */
    char MissionInternalName[40];
    int PlayerSide;
    int ClientDifficulty;
    int SpawnerGlobalFlagValues[MAX_ENVIRONMENT_GLOBALS];
    bool IsCheatSession;
    char BonusName[40];
};

const WCHAR* Vinifera_Stream_Name_From_ID(int id);
bool Vinifera_Get_Savefile_Info(char const* name, ViniferaSaveVersionInfo& info);
