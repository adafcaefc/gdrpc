#pragma once
#include <gd.h>
#undef snprintf
#include <filesystem>
#include "gd_rpc_config.h"
#include "gd_rpc_api.h"

namespace rpc
{
    enum class player_state 
    {
        level,
        editor,
        menu,
    };

    class loop {
    private:
        player_state state = player_state::menu;
        ::gd::GJGameLevel* game_level;
        gd_level level;

        bool update_presence = false, 
            update_timestamp = false;

        std::time_t current_timestamp = NULL;

        rpc::config::cformat config;

        std::string large_text;

        void update_presence_w(
            std::string&, 
            std::string&, std::string&,
            std::string&, std::string&);

        bool enabled = false;

        loop()
            : state(player_state::menu)
            , current_timestamp(std::time(nullptr))
            , update_presence(false)
            , update_timestamp(false)
        {

        }

    public:

        loop(loop const&) = delete;
        void operator=(loop const&) = delete;
        static loop* get()
        {
            static loop instance;
            return &instance;
        }

        void set_update_presence(const bool value)
        {
            this->update_presence = value;
        }

        void set_update_timestamp(const bool value)
        {
            this->update_timestamp = value;
        }

        bool get_reset_timestamp(int folder = 0)
        {
            if (static_cast<size_t>(folder) >= this->config._editor.size()) folder = 0;
            return this->config._editor.at(folder).reset_timestamp;
        }

        std::string get_executable_name()
        {
            char szFilePath[MAX_PATH + 1] = { 0 };
            GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
            return std::filesystem::path(szFilePath).filename().string();
        }

        void on_loop();
        void on_loop_level();
        void on_loop_editor();
        void on_loop_menu();

        void initialize_config();
        void initialize_loop();

        void close();

        void enable_discord();
        void disable_discord();

        player_state get_state() { return this->state; }
        void set_state(const player_state value) { this->state = value; }

        ::gd::GJGameLevel* get_game_level() { return this->game_level; }
        void set_game_level(::gd::GJGameLevel* value) { this->game_level = value; }

        bool is_enabled()
        {
            return this->enabled;
        }

        static DWORD WINAPI main_thread(LPVOID lpParam);
    };
}

