#include "pch.h"
#include "gd_rpc_loop.h"
#include "gd_rpc_settings.h"
#include "../gdhook/minhook_wrapper.h"

namespace rpc
{
    template <typename T, typename T_struct>
    T* offset_from_base(
        T_struct* struct_ptr,
        const std::uintptr_t addr)
    {
        return reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(struct_ptr) + addr);
    }

    static LONG_PTR o_WindowProc;
    static LRESULT CALLBACK h_WindowProc(
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
        return ::CallWindowProc((WNDPROC)o_WindowProc, hwnd, msg, wparam, lparam);
    }

    static void fix_object_count(gd::LevelEditorLayer* __this, gd::GJGameLevel* level)
    {
        level->objectCount_rand = __this->m_nObjectCountRand;
        level->objectCount_seed = __this->m_nObjectCountSeed;
        level->objectCount = __this->m_nObjectCount;
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
        GDRPC_LOG_INFO("[GDRPC] PlayLayer_create called");
        return o_PlayLayer_create(gameLevel);
    }

    __SETUP_GD_HOOK__(void*, PlayLayer_onQuit, gd::PlayLayer* __this)
    {
        loop::get()->set_state(player_state::menu);
        loop::get()->set_update_timestamp(true);
        loop::get()->set_update_presence(true);
        GDRPC_LOG_INFO("[GDRPC] PlayLayer_onQuit called");
        return o_PlayLayer_onQuit(__this);
    }

    __SETUP_GD_HOOK__(void*, PlayLayer_showNewBest, gd::PlayLayer* playLayer, char p1, float p2, int p3, char p4, char p5, char p6)
    {
        loop::get()->set_update_presence(true);
        GDRPC_LOG_INFO("[GDRPC] PlayLayer_showNewBest called");
        return o_PlayLayer_showNewBest(playLayer, p1, p2, p3, p4, p5, p6);
    }

    __SETUP_GD_HOOK__(void*, EditorPauseLayer_onExitEditor, void* __this, void* p1)
    {
        loop::get()->set_state(player_state::menu);
        loop::get()->set_update_timestamp(true);
        loop::get()->set_update_presence(true);
        GDRPC_LOG_INFO("[GDRPC] EditorPauseLayer_onExitEditor called");
        return o_EditorPauseLayer_onExitEditor(__this, p1);
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
        GDRPC_LOG_INFO("[GDRPC] LevelEditorLayer_create called");
        return o_LevelEditorLayer_create(gameLevel);
    }

    __SETUP_GD_HOOK__(void, LevelEditorLayer_addSpecial, gd::LevelEditorLayer* __this, void* object)
    {
        o_LevelEditorLayer_addSpecial(__this, object);
        const auto new_object_count = __this->m_nObjectCount;
        fix_object_count(__this, loop::get()->get_game_level());
        loop::get()->set_update_presence(true);
    }

    __SETUP_GD_HOOK__(void, LevelEditorLayer_removeSpecial, gd::LevelEditorLayer* __this, void* object)
    {
        o_LevelEditorLayer_removeSpecial(__this, object);
        fix_object_count(__this, loop::get()->get_game_level());
        loop::get()->set_update_presence(true);
    }

    __SETUP_GD_HOOK__(void, CCDirector_end, cocos2d::CCDirector* __this)
    {
        loop::get()->close();
        o_CCDirector_end(__this);
    }

    void init_rpc()
    {
        static bool __init = false;
        if (__init) return;
        ::CreateThread(nullptr, NULL, loop::main_thread, nullptr, NULL, nullptr);
        __init = true;
    }

    void init_hook() 
    {
        try
        {
            loop::get()->initialize_config();
        }
        catch (std::exception& e) 
        {
            GDRPC_LOG_ERROR("[GDRPC] failed to initialize config, {}", e.what());
            return;
        }
        
        __GD_HOOK__(0x1FB6D0, PlayLayer_create);
        __GD_HOOK__(0x20D810, PlayLayer_onQuit);
        __GD_HOOK__(0x1FE3A0, PlayLayer_showNewBest);
        __GD_HOOK__(0x75660, EditorPauseLayer_onExitEditor);
        __GD_HOOK__(0x15ED60, LevelEditorLayer_create);
        __GD_HOOK__(0x162650, LevelEditorLayer_addSpecial);
        __GD_HOOK__(0x162FF0, LevelEditorLayer_removeSpecial);
        __METHOD_HOOK__("libcocos2d.dll", "?end@CCDirector@cocos2d@@QAEXXZ", CCDirector_end);
        o_WindowProc = ::SetWindowLongPtr(GetForegroundWindow(), GWLP_WNDPROC, (LONG_PTR)h_WindowProc);
    }
}
