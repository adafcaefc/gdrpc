#include "pch.h"
#include "gd_rpc_api.h"
#define CURL_STATICLIB
#include <curl/curl.h>
#include <gd.h>
#undef snprintf

namespace rpc
{
    static std::string _unsafe_post_http_request(
        const char* url,
        const char* fields)
    {
        std::string response_string;
        auto curl = curl_easy_init();

        if (!curl) return response_string;

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Accept: */*");
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "User-Agent: ");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        std::string header_string;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

        char* _url;
        long response_code;
        double elapsed;

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &_url);

        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        return response_string;
    }

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
        client = std::make_shared<httplib::Client>(host.c_str());
    }

    std::string gd_client::post_request(std::string url, params& params)
    {
        try
        {
            params.emplace("gameVersion", std::to_string(game_version));
            params.emplace("secret", secret);

            std::string full_url = prefix + url;

            auto res = client.get()->Post(full_url.c_str(), params);
            auto body = res->body;

            return body;
        }
        catch (...)
        {
            return "-1";
        }
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
            // throw the exceptions that will actually have a message
            // throw e;
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
        catch (const std::exception& e) {
            //throw e;
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
            // hey look its the checks
            // so basically you look for
            // :16:<id>:
            std::string lookup_string = ":16:" + std::to_string(user.account_id) + ":";
            auto player_entry =
                std::find_if(leaderboard_list.begin(), leaderboard_list.end(),
                    [&lookup_string](std::string& entry) {
                        return (entry.find(lookup_string) != std::string::npos);
                    });
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
        auto newID = in_memory->levelID;
        auto levelLocation = in_memory->levelType;

        // don't calculate more than we have to, but the editor keeps id 0
        if (newID == level.level_id && levelLocation != ::gd::GJLevelType::kGJLevelTypeEditor)
        {
            return true;
        }

        level.level_id = newID;
        level.stars = in_memory->stars;

        level.name = in_memory->levelName;

        // good robtop security
        level.is_demon = static_cast<bool>(in_memory->demon);
        level.is_auto = in_memory->autoLevel;

        if (levelLocation == 1) 
        {
            level.author = "RobTop"; // author is "" on these
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
        int position = 0;

        robtop_map robtop;

        auto split_string = explode(string, delimiter);

        for (auto it = split_string.begin(); it != split_string.end(); ++it) {
            // get position, check if odd (aka key)
            if ((it - split_string.begin()) % 2 == 0) {
                robtop.emplace(std::stoi(*it, nullptr), *std::next(it));
            }
        }

        return robtop;
    }

    // helper function
    std::vector<std::string> explode(
        std::string& string, 
        const char separator)
    {
        std::stringstream segmentstream(string);
        std::string segmented;
        std::vector<std::string> splitlist;

        while (std::getline(segmentstream, segmented, separator)) 
        {
            splitlist.push_back(segmented);
        }

        return splitlist;
    }
}