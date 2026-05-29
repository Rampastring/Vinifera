/*******************************************************************************
/*                 O P E N  S O U R C E  --  V I N I F E R A                  **
/*******************************************************************************
 *  @brief  Vinifera replacement of the save file header.
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Copyright (c) 2020-2026 Vinifera contributors
 ******************************************************************************/

#include "always.h"

#include "vinifera_savever.h"

#include "debughandler.h"
#include "tibsun_defines.h"

#include <comdef.h>
#include <iterator>


/**
 *  Saves the version information to the storage.
 *
 *  @author: tomsons26, ZivDero
 */
HRESULT ViniferaSaveVersionInfo::Save(IStorage* storage) const
{
    if (storage == nullptr) {
        return E_POINTER;
    }

    DEBUG_INFO("Saving version information.\n");

    HRESULT res = Save_String(storage, L"Scenario Description", ScenarioDescription);
    if (FAILED(res)) {
        return res;
    }

    res = Save_String(storage, L"Player House", PlayerHouse);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Internal Version", InternalVersion);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Scenario Number", ScenarioNumber);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Campaign", CampaignNumber);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"GameType", GameType);
    if (FAILED(res)) {
        return res;
    }

    /**
     *  New Vinifera fields.
     */
    res = Save_Int(storage, L"Vinifera Version", ViniferaVersion);
    if (FAILED(res)) {
        return res;
    }

    res = Save_String(storage, L"Vinifera Commit Hash", ViniferaCommitHash);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Playthrough ID", PlaythroughID);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Difficulty", Difficulty);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Elapsed Time", ElapsedTime);
    if (FAILED(res)) {
        return res;
    }

    /**
     *  New DTA fields.
     */
    res = Save_String(storage, L"Mission Internal Name", MissionInternalName);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Player Side", PlayerSide);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int(storage, L"Client Difficulty", ClientDifficulty);
    if (FAILED(res)) {
        return res;
    }

    res = Save_Int_Array(storage, L"Global Flags", SpawnerGlobalFlagValues, std::size(SpawnerGlobalFlagValues));
    if (FAILED(res)) {
        return res;
    }

    res = Save_Bool(storage, L"Cheat Session", IsCheatSession);
    if (FAILED(res)) {
        return res;
    }

    res = Save_String(storage, L"Bonus Name", BonusName);
    if (FAILED(res)) {
        return res;
    }

    return S_OK;
}


/**
 *  Loads the version information from the storage.
 *
 *  @author: tomsons26, ZivDero
 */
HRESULT ViniferaSaveVersionInfo::Load(IStorage* storage)
{
    if (storage == nullptr) {
        return E_POINTER;
    }

    DEBUG_INFO("Loading version information.\n");

    HRESULT res = Load_String(storage, L"Scenario Description", ScenarioDescription);
    if (FAILED(res)) {
        return res;
    }

    res = Load_String(storage, L"Player House", PlayerHouse);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Internal Version", InternalVersion);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Scenario Number", ScenarioNumber);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Campaign", CampaignNumber);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"GameType", GameType);
    if (FAILED(res)) {
        return res;
    }

    /**
     *  New Vinifera fields.
     */
    res = Load_Int(storage, L"Vinifera Version", ViniferaVersion);
    if (FAILED(res)) {
        return res;
    }

    res = Load_String(storage, L"Vinifera Commit Hash", ViniferaCommitHash);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Playthrough ID", PlaythroughID);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Difficulty", Difficulty);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Elapsed Time", ElapsedTime);
    if (FAILED(res)) {
        return res;
    }

    /**
     *  New DTA fields.
     */
    res = Load_String(storage, L"Mission Internal Name", MissionInternalName);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Player Side", PlayerSide);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int(storage, L"Client Difficulty", ClientDifficulty);
    if (FAILED(res)) {
        return res;
    }

    res = Load_Int_Array(storage, L"Global Flags", SpawnerGlobalFlagValues, std::size(SpawnerGlobalFlagValues));
    if (FAILED(res)) {
        return res;
    }

    res = Load_Bool(storage, L"Cheat Session", IsCheatSession);
    if (FAILED(res)) {
        return res;
    }

    res = Load_String(storage, L"Bonus Name", BonusName);
    if (FAILED(res)) {
        return res;
    }

    return S_OK;
}


/**
 *  Loads a string from the storage.
 *
 *  @author: tomsons26, ZivDero
 */
HRESULT ViniferaSaveVersionInfo::Load_String(IStorage* storage, const WCHAR* name, std::string& string)
{
    string.clear();

    IStreamPtr stm;
    HRESULT res = storage->OpenStream(name, nullptr, STGM_SHARE_EXCLUSIVE, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    std::wstring buffer;
    WCHAR ch;

    do {
        res = stm->Read(&ch, sizeof(ch), nullptr);
        if (FAILED(res)) {
            return res;
        }

        if (ch) {
            buffer.push_back(ch);
        }

    } while (ch);

    int len = WideCharToMultiByte(CP_ACP, 0, buffer.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len > 1) {
        string.resize(len - 1);
        WideCharToMultiByte(CP_ACP, 0, buffer.c_str(), -1, string.data(), len, nullptr, nullptr);
    }

    return res;
}


/**
 *  Loads an integer from the storage.
 *
 *  @author: tomsons26
 */
HRESULT ViniferaSaveVersionInfo::Load_Int(IStorage* storage, const WCHAR* name, int& integer)
{
    integer = 0;

    IStreamPtr stm;
    HRESULT res = storage->OpenStream(name, nullptr, STGM_SHARE_EXCLUSIVE, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Read(&integer, sizeof(integer), nullptr);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Loads a boolean from the storage.
 *
 *  @author: Rampastring
 */
HRESULT ViniferaSaveVersionInfo::Load_Bool(IStorage* storage, const WCHAR* name, bool& boolean)
{
    boolean = false;

    IStreamPtr stm;
    HRESULT res = storage->OpenStream(name, nullptr, STGM_SHARE_EXCLUSIVE, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Read(&boolean, sizeof(boolean), nullptr);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Loads a vector of integers from the storage.
 *
 *  @author: Rampastring
 */
HRESULT ViniferaSaveVersionInfo::Load_Int_Vector(IStorage* storage, const WCHAR* name, std::vector<int>& values)
{
    IStreamPtr stm;

    HRESULT res = storage->OpenStream(name, nullptr, STGM_SHARE_EXCLUSIVE, 0, &stm);

    if (FAILED(res)) return res;

    int count = 0;
    ULONG read = 0;

    res = stm->Read(&count, sizeof(count), &read);
    if (FAILED(res) || read != sizeof(count)) return E_FAIL;

    values.resize(count);

    res = stm->Read(values.data(), sizeof(int) * count, &read);
    if (FAILED(res)) return res;

    return res;
}


/**
 *  Loads an array of integers from the storage.
 *
 *  @author: Rampastring
 */
HRESULT ViniferaSaveVersionInfo::Load_Int_Array(IStorage* storage, const WCHAR* name, int* array, size_t size)
{
    std::vector<int> vec;

    HRESULT res = Load_Int_Vector(storage, name, vec);
    if (FAILED(res)) {
        return res;
    }

    size_t count = std::min(size, vec.size());

    std::copy_n(vec.data(), count, array);

    if (count < size) {
        std::fill(array + count, array + size, 0);
    }

    return res;
}


/**
 *  Saves a string to the storage.
 *
 *  @author: tomsons26, ZivDero
 */
HRESULT ViniferaSaveVersionInfo::Save_String(IStorage* storage, const WCHAR* name, const std::string& string)
{
    int len = MultiByteToWideChar(CP_ACP, 0, string.c_str(), -1, nullptr, 0);
    std::wstring buffer(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, string.c_str(), -1, buffer.data(), len);

    IStreamPtr stm(nullptr);
    HRESULT res = storage->CreateStream(name, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Write(buffer.c_str(), sizeof(WCHAR) * len, nullptr);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Commit(STGC_DEFAULT);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Saves an integer to the storage.
 *
 *  @author: tomsons26
 */
HRESULT ViniferaSaveVersionInfo::Save_Int(IStorage* storage, const WCHAR* name, int integer)
{
    IStreamPtr stm(nullptr);
    HRESULT res = storage->CreateStream(name, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Write(&integer, sizeof(integer), nullptr);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Commit(STGC_DEFAULT);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Saves a boolean to the storage.
 *
 *  @author: Rampastring
 */
HRESULT ViniferaSaveVersionInfo::Save_Bool(IStorage* storage, const WCHAR* name, bool boolean)
{
    IStreamPtr stm(nullptr);
    HRESULT res = storage->CreateStream(name, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Write(&boolean, sizeof(boolean), nullptr);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Commit(STGC_DEFAULT);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Saves a vector of integers to the storage.
 *
 *  @author: Rampastring
 */
HRESULT ViniferaSaveVersionInfo::Save_Int_Vector(IStorage* storage, const WCHAR* name, const std::vector<int>& values)
{
    IStreamPtr stm;

    HRESULT res = storage->CreateStream(name, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm);

    if (FAILED(res)) {
        return res;
    }

    ULONG written = 0;

    int count = static_cast<int>(values.size());

    res = stm->Write(&count, sizeof(count), &written);
    if (FAILED(res)) return res;

    res = stm->Write(values.data(), sizeof(int) * count, &written);
    if (FAILED(res)) return res;

    return stm->Commit(STGC_DEFAULT);
}

/**
 *  Saves an array of integers to the storage.
 *
 *  @author: Rampastring
 */
HRESULT ViniferaSaveVersionInfo::Save_Int_Array(IStorage* storage, const WCHAR* name, const int* array, size_t size)
{
    std::vector<int> vec(array, array + size);
    return Save_Int_Vector(storage, name, vec);
}


/**
 *  Load a FILETIME from the storage.
 *
 *  @author: tomsons26
 */
HRESULT ViniferaSaveVersionInfo::Load_Time(IStorage* storage, const WCHAR* name, FILETIME& time)
{
    time.dwLowDateTime = 0;
    time.dwHighDateTime = 0;

    IStreamPtr stm;
    HRESULT res = storage->OpenStream(name, nullptr, STGM_SHARE_EXCLUSIVE, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Read(&time, sizeof(time), nullptr);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Save a FILETIME to the storage.
 *
 *  @author: tomsons26
 */
HRESULT ViniferaSaveVersionInfo::Save_Time(IStorage* storage, const WCHAR* name, const FILETIME& time)
{
    IStreamPtr stm(nullptr);
    HRESULT res = storage->CreateStream(name, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, 0, &stm);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Write(&time, sizeof(time), nullptr);
    if (FAILED(res)) {
        return res;
    }

    res = stm->Commit(STGC_DEFAULT);
    if (FAILED(res)) {
        return res;
    }

    return res;
}


/**
 *  Read the save version info from a save file.
 *
 *  @author: tomsons26
 */
bool Vinifera_Get_Savefile_Info(std::string_view name, ViniferaSaveVersionInfo& info)
{
    IStoragePtr storage;
    WCHAR wname[PATH_MAX];

    int len = MultiByteToWideChar(CP_ACP, 0, name.data(), (int)name.size(), wname, std::size(wname) - 1);
    wname[len] = L'\0';

    HRESULT result = StgOpenStorage(wname, nullptr, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, nullptr, 0, &storage);
    if (FAILED(result)) {
        return false;
    }

    result = info.Load(storage);
    if (FAILED(result)) {
        return false;
    }

    return true;
}
