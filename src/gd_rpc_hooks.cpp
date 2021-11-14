#include "pch.h"
#include "gd_rpc_loop.h"
#include "gd_rpc_log.h"
#include "../gdhook/minhook_wrapper.h"

namespace rpc
{
    // this handles x button close
    LONG_PTR oWindowProc;
    LRESULT CALLBACK nWindowProc(
        HWND hwnd, 
        UINT msg, 
        WPARAM wparam,
        LPARAM lparam)
    {
        switch (msg) 
        {
        case WM_CLOSE:
            loop::get()->close();
            break;
        }
        return CallWindowProc((WNDPROC)oWindowProc, hwnd, msg, wparam, lparam);
    }

    HMODULE GetCurrentModule()
    {
        HMODULE hModule = NULL;
        GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCTSTR)GetCurrentModule, 
            &hModule);
        return hModule;
    }

    void init_rpc()
    {
        if (!tm_settings::get()->gd_rpc_enable.active) return;
        CreateThread(NULL, 0, loop::main_thread, GetCurrentModule(), 0, NULL);
    }

    __SETUP_GD_HOOK__(void*, PlayLayer_create, gd::GJGameLevel* gameLevel)
    {
        if (loop::get()->get_state() != player_state::editor ||
            loop::get()->get_reset_timestamp(gameLevel->levelFolder)) 
        {
            loop::get()->set_update_timestamp(true);
        }
        loop::get()->set_state(player_state::level);
        loop::get()->set_update_presence(true);
        loop::get()->set_game_level(gameLevel);
        return o_PlayLayer_create(gameLevel);
    }

    __SETUP_GD_HOOK__(void*, PlayLayer_onQuit, gd::PlayLayer* playLayer)
    {
        loop::get()->set_state(player_state::menu);
        loop::get()->set_update_timestamp(true);
        loop::get()->set_update_presence(true);
        return o_PlayLayer_onQuit(playLayer);
    }

    __SETUP_GD_HOOK__(void*, PlayLayer_showNewBest, gd::PlayLayer* playLayer, char p1, float p2, int p3, char p4, char p5, char p6)
    {
        auto current_level = loop::get()->get_game_level();
        loop::get()->set_update_presence(true);
        return o_PlayLayer_showNewBest(playLayer, p1, p2, p3, p4, p5, p6);
    }

    __SETUP_GD_HOOK__(void*, EditorPauseLayer_onExitEditor, void* editorPauseLayer, void* p1)
    {
        loop::get()->set_state(player_state::menu);
        loop::get()->set_update_timestamp(true);
        loop::get()->set_update_presence(true);
        return o_EditorPauseLayer_onExitEditor(editorPauseLayer, p1);
    }

    __SETUP_GD_HOOK__(void*, LevelEditorLayer_create, gd::GJGameLevel* gameLevel)
    {
        if (loop::get()->get_state() != player_state::level ||
            loop::get()->get_reset_timestamp()) 
        {
            loop::get()->set_update_timestamp(true);
        }
        loop::get()->set_state(player_state::editor);
        loop::get()->set_update_presence(true);
        loop::get()->set_game_level(gameLevel);
        return o_LevelEditorLayer_create(gameLevel);
    }

    template <typename T> 
    T* offset_from_base(
        void* struct_ptr, 
        const std::uintptr_t addr)
    {
        return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(struct_ptr) + addr);
    }

    void fix_object_count(void* LevelEditorLayer, gd::GJGameLevel* level) 
    {
        const auto LevelEditorLayer_objectCount_offset = 0x3A0;
        const auto size_of_int = sizeof(int);
        level->objectCount_rand = *offset_from_base<int>(LevelEditorLayer, LevelEditorLayer_objectCount_offset - (size_of_int * 2));
        level->objectCount_seed = *offset_from_base<int>(LevelEditorLayer, LevelEditorLayer_objectCount_offset - size_of_int);
        level->objectCount = *offset_from_base<int>(LevelEditorLayer, LevelEditorLayer_objectCount_offset);
    }

    __SETUP_GD_HOOK__(void, LevelEditorLayer_addSpecial, gd::LevelEditorLayer* __this, void* object)
    {
        o_LevelEditorLayer_addSpecial(__this, object);
        auto new_object_count = *offset_from_base<int>(__this, 0x3A0);
        if (loop::get()->get_game_level()->objectCount >= new_object_count) return; // this should hopefully prevent spam upon level loading
        fix_object_count(__this, loop::get()->get_game_level());
        loop::get()->set_update_presence(true);
    }

    __SETUP_GD_HOOK__(void, LevelEditorLayer_removeSpecial, gd::LevelEditorLayer* __this, void* object)
    {
        o_LevelEditorLayer_removeSpecial(__this, object);
        auto new_object_count = *offset_from_base<int>(__this, 0x3A0);
        if (loop::get()->get_game_level()->objectCount < new_object_count) return; // this should hopefully prevent spam upon level exiting
        fix_object_count(__this, loop::get()->get_game_level());
        loop::get()->set_update_presence(true);
    }

    __SETUP_GD_HOOK__(void, CCDirector_end, cocos2d::CCDirector* __this)
    {
        loop::get()->close();
        o_CCDirector_end(__this);
    }

    void init_hook() 
    {
        try
        {
            loop::get()->initialize_config();
        }
        catch (const std::exception& e) 
        {
            GDRPC_LOG_ERROR("[GDRPC] failed to initialize config, {}", e.what());
            return;
        }

        std::string gd_name = loop::get()->get_executable_name();

        HMODULE gd_handle = GetModuleHandle(NULL);
        HMODULE cocos_handle = LoadLibraryA("libcocos2d.dll");

        // close button calls this, x button calls wndproc
        if (!gd_handle || !cocos_handle)
        {
            GDRPC_LOG_ERROR("[GDRPC] failed to get module handle");
            return;
        }
        
        // setup closes
        oWindowProc = SetWindowLongPtrA(GetForegroundWindow(), GWL_WNDPROC, (LONG_PTR)nWindowProc);

        __GD_HOOK__(0x1FB6D0, PlayLayer_create);
        __GD_HOOK__(0x20D810, PlayLayer_onQuit);
        __GD_HOOK__(0x1FE3A0, PlayLayer_showNewBest);
        __GD_HOOK__(0x75660, EditorPauseLayer_onExitEditor);
        __GD_HOOK__(0x15ED60, LevelEditorLayer_create);
        __GD_HOOK__(0x162650, LevelEditorLayer_addSpecial);
        __GD_HOOK__(0x162FF0, LevelEditorLayer_removeSpecial);
        __METHOD_HOOK__("libcocos2d.dll", "?end@CCDirector@cocos2d@@QAEXXZ", CCDirector_end);
    }
}