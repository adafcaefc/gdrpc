#include "pch.h"
#include "gd_rpc_api.h"
#include "gd_rpc_curl_utils.h"
#include "gd_rpc_config.h"
#include "gd_rpc_settings.h"
#include <gd.h>
#undef snprintf

namespace rpc
{
    gdx::demon gd_client::get_demon_difficulty_value(const int difficulty)
    {
        switch (difficulty)
        {
        case 3:
            return gdx::demon::Easy;
        case 4:
            return gdx::demon::Medium;
        case 5:
            return gdx::demon::Insane;
        case 6:
            return gdx::demon::Extreme;
        case 0:
        case 1:
        case 2:
        default:
            return gdx::demon::Hard;
        }
    }

    std::string gd_client::get_difficulty_name(gd_level& level) 
    {
        if (level.is_auto) return "auto";

        if (level.is_demon) 
        {
            switch (level.demon_difficulty) 
            {
            case gdx::demon::Easy:
                return "easy_demon";
            case gdx::demon::Medium:
                return "medium_demon";
            case gdx::demon::Insane:
                return "insane_demon";
            case gdx::demon::Extreme:
                return "extreme_demon";
            case gdx::demon::Hard:
            default:
                return "hard_demon";
            }
        }

        switch (level.difficulty) 
        {
        case gdx::difficulty::Easy:
            return "easy";
        case gdx::difficulty::Normal:
            return "normal";
        case gdx::difficulty::Hard:
            return "hard";
        case gdx::difficulty::Harder:
            return "harder";
        case gdx::difficulty::Insane:
            return "insane";
        case gdx::difficulty::Demon:
            return "hard_demon";
        case gdx::difficulty::Na:
        default:
            return "na";
        }
    }

    gd_client::gd_client(
        const std::string& host,
        const std::string& prefix)
        : game_version(21)
        , secret("Wmfd2893gb7")
        , host(host)
        , prefix(prefix)
    {

    }

    std::string gd_client::post_request(
        const std::string& endpoint, 
        params& my_params)
    {
        const auto params_to_string = [](params& all_params)-> std::string
        {
            std::string buffer;
            for (auto& param : all_params)
            {
                buffer.append(fmt::format("{}={}&", param.first, param.second));
            }
            if (!buffer.empty()) buffer.pop_back();
            return buffer;
        };
        my_params.emplace("gameVersion", std::to_string(game_version));
        my_params.emplace("secret", secret);
        const auto result = post_http_request(host + prefix + endpoint, params_to_string(my_params));
        if (result.empty()) return std::string("-1"); // failed request
        return result;
    }

    bool gd_client::get_user_info(
        int account_id,
        gd_user& user) 
    {
        params _params({ { "targetAccountID", std::to_string(account_id) } });
        auto user_string = gd_client::post_request(urls.get_user_info, _params);

        try 
        {
            auto user_map = to_robtop(user_string);
            user.name = user_map.at(1);
            user.id = std::stoi(user_map.at(2), nullptr);
            user.account_id = std::stoi(user_map.at(16), nullptr);
            return true;
        }
        catch (const std::exception& e) 
        {
            GDRPC_LOG_ERROR("[GDRPC] failed to get user info, {}", e.what());
        }
        return false;
    }

    bool gd_client::get_player_info(
        const int player_id, 
        gd_user& user) 
    {
        params _params({ { "str", std::to_string(player_id) } });
        auto player_string = gd_client::post_request(urls.get_users, _params);

        try 
        {
            auto user_map = to_robtop(player_string);
            user.name = user_map.at(1);
            user.id = std::stoi(user_map.at(2), nullptr);
            user.account_id = std::stoi(user_map.at(16), nullptr);
            return true;
        }
        catch (const std::exception& e) 
        {
            GDRPC_LOG_ERROR("[GDRPC] failed to get player info, {}", e.what());
        }
        return false;
    }

    bool gd_client::get_user_rank(gd_user& user) 
    {
        params _params({ { "type", "relative" }, { "accountID", std::to_string(user.account_id) } });
        auto result = post_request(urls.get_scores, _params);

        auto leaderboard_list = explode(result, '|');

        bool found_user = false;
        robtop_map seglist;

        if (leaderboard_list.size() >= 24)
        {
            seglist = to_robtop(leaderboard_list.at(24));
            found_user = (std::stoi(seglist.at(16), nullptr) == user.account_id);
        }

        if (!found_user)
        {
            std::string lookup_string = ":16:" + std::to_string(user.account_id) + ":";
            const auto routine = [&lookup_string](std::string& entry) { return (entry.find(lookup_string) != std::string::npos); };
            auto player_entry = std::find_if(leaderboard_list.begin(), leaderboard_list.end(), routine);
            if (player_entry == leaderboard_list.end()) user.rank = -1;
            seglist = to_robtop(*player_entry);
        }

        user.rank = std::stoi(seglist.at(6), nullptr);
        return true;
    }

    bool parse_game_level(
        ::gd::GJGameLevel* in_memory, 
        gd_level& level) 
    {
        auto new_id = in_memory->levelID;
        auto level_location = in_memory->levelType;

        if (new_id == level.level_id &&
            level_location != ::gd::GJLevelType::kGJLevelTypeEditor)
        {
            return true;
        }

        level.level_id = new_id;
        level.stars = in_memory->stars;
        level.name = in_memory->levelName;
        level.is_demon = static_cast<bool>(in_memory->demon);
        level.is_auto = in_memory->autoLevel;

        if (level_location == ::gd::GJLevelType::kGJLevelTypeLocal)
        {
            level.author = "RobTop"; // author is empty on local levels
            level.difficulty = static_cast<gdx::difficulty>(in_memory->difficulty);
            if (level.difficulty == gdx::difficulty::Demon)
            {
                level.demon_difficulty = gdx::demon::Easy;
            }
        }
        else 
        {
            level.author = in_memory->userName;
            level.difficulty = static_cast<gdx::difficulty>(in_memory->ratingsSum / 10);
            if (level.is_demon) 
            {
                level.demon_difficulty = gd_client::get_demon_difficulty_value(in_memory->demonDifficulty);
            }
        }
        return true;
    }

    robtop_map to_robtop(
        std::string& string, 
        const char delimiter)
    {
        std::stringstream segments(string);
        std::string previous_segment, current_segment;
        robtop_map robtop;

        try
        {
            auto split_string = explode(string, delimiter);
            if (split_string.empty())
            {
                GDRPC_LOG_ERROR("[GDRPC] failed to parse robtop format for value ({})", string.c_str());
                return robtop;
            }
            for (auto it = split_string.begin(); it != split_string.end(); ++it)
            {
                if ((it - split_string.begin()) % 2 == 0) // get position, check if odd (aka key)
                {
                    robtop.emplace(std::stoi(*it, nullptr), *std::next(it));
                }
            }
        }
        catch (std::exception& e)
        {
            GDRPC_LOG_ERROR("[GDRPC] failed to get parse robtop format, {}", e.what());
        }

        return robtop;
    }

    std::vector<std::string> explode(
        std::string& string, 
        const char separator)
    {
        std::stringstream stream(string);
        std::string segment;
        std::vector<std::string> list;
        while (std::getline(stream, segment, separator)) list.push_back(segment);
        return list;
    }
}