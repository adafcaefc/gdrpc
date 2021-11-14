#pragma once
#include <toml.hpp>

namespace rpc 
{
    namespace config
    {
        static constexpr int LATEST_VERSION = 0;
        static constexpr auto DEFAULT_URL = "http://boomlings.com";
        static constexpr auto DEFAULT_PREFIX = "/database/";
        constexpr auto DEFAULT_APPLICATION_ID = "668228366893056001";

        struct presence
        {
            std::string detail;
            std::string state;
            std::string smalltext;

            void from_toml(const toml::value& table)
            {
                this->detail = toml::find<std::string>(table, "detail");
                this->state = toml::find<std::string>(table, "state");
                this->smalltext = toml::find<std::string>(table, "smalltext");
            }

            toml::value into_toml() const
            {
                return toml::table
                { 
                    { "detail", this->detail },
                    { "state", this->state },
                    { "smalltext", this->smalltext } 
                };
            }
        };

        struct cformat 
        {
            struct level 
            {
                presence saved;
                presence playtesting;

                void from_toml(const toml::value& table) 
                {
                    this->saved = toml::find<presence>(table, "saved");
                    this->playtesting = toml::find<presence>(table, "playtesting");
                }

                toml::value into_toml() const 
                {
                    return toml::table
                    { 
                        { "saved", this->saved },
                        { "playtesting", this->playtesting } 
                    };
                }
            };

            struct editor : presence 
            {
                bool reset_timestamp = false;

                void from_toml(const toml::value& table) 
                {
                    this->detail = toml::find<std::string>(table, "detail");
                    this->state = toml::find<std::string>(table, "state");
                    this->smalltext = toml::find<std::string>(table, "smalltext");
                    this->reset_timestamp = toml::find_or<bool>(table, "reset_timestamp", reset_timestamp);
                }

                toml::value into_toml() const 
                {
                    return toml::table
                    { 
                        {"detail", this->detail},
                        {"state", this->state},
                        {"smalltext", this->smalltext},
                        {"reset_timestamp", this->reset_timestamp} 
                    };
                }
            };

            struct user 
            {
                std::string ranked;
                std::string _default;
                bool get_rank;

                void from_toml(const toml::value& table) 
                {
                    this->ranked = toml::find<std::string>(table, "ranked");
                    this->_default = toml::find<std::string>(table, "default");
                    this->get_rank = toml::find<bool>(table, "get_rank");
                }

                toml::value into_toml() const 
                {
                    return toml::table
                    { 
                        {"ranked", this->ranked},
                        {"default", this->_default},
                        {"get_rank", this->get_rank} 
                    };
                }
            };

            struct settings 
            {
                int file_version = 0;
                std::string 
                    base_url, 
                    url_prefix, 
                    application_id;

                void from_toml(const toml::value& table) 
                {
                    this->file_version = toml::find<int>(table, "file_version");
                    this->base_url = toml::find_or<std::string>(table, "base_url", DEFAULT_URL);
                    this->url_prefix = toml::find_or<std::string>(table, "url_prefix", DEFAULT_PREFIX);
                    this->application_id = toml::find_or<std::string>(table, "application_id", DEFAULT_APPLICATION_ID);
                }

                toml::value into_toml() const 
                {
                    return toml::table
                    { 
                        {"file_version", this->file_version},
                        {"base_url", this->base_url},
                        {"url_prefix", this->url_prefix},
                        {"application_id", this->application_id} 
                    };
                }
            };

            void from_toml(const toml::value& table) 
            {
                if (table.at("level").type() == toml::value_t::array) 
                {
                    this->_level = toml::find<std::vector<cformat::level>>(table, "level");
                }
                else 
                {
                    this->_level.at(0) = toml::find<cformat::level>(table, "level");
                }

                if (table.at("editor").type() == toml::value_t::array)
                {
                    this->_editor = toml::find<std::vector<cformat::editor>>(table, "editor");
                }
                else
                {
                    this->_editor.at(0) = toml::find<cformat::editor>(table, "editor");
                }

                this->_user = toml::find<cformat::user>(table, "user");
                this->_menu = toml::find<presence>(table, "menu");

                if (table.contains("settings")) 
                {
                    // this table is still optional due to previous versions not containing it
                    this->_settings = toml::find<cformat::settings>(table, "settings");
                }
            }

            toml::value into_toml() const
            {
                return toml::table
                {
                    {"level", this->_level},
                    {"editor", this->_editor},
                    {"user", this->_user},
                    {"menu", this->_menu},
                    {"settings", this->_settings}
                };
            }

            std::vector<cformat::level> _level
            {
                {
                    {
                        "Playing {name}", 
                        "by {author} ({best}%)", 
                        "{stars}* {diff} ({id})"
                    },
                    {
                        "Playtesting a level", 
                        "", 
                        ""
                    }
                } 
            };
            std::vector<cformat::editor> _editor
            {
                {
                    {
                        "Editing a level", 
                        "{objects} objects", 
                        ""
                    }, 
                    false
                } 
            };
            cformat::user _user = { "{name} [Rank #{rank}]", "", true };
            presence _menu = { "Idle", "", "" };

            settings _settings =
            {
                config::LATEST_VERSION,
                config::DEFAULT_URL,
                config::DEFAULT_PREFIX,
                config::DEFAULT_APPLICATION_ID
            };
        };
    }
}