#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace gd
{
    class GJGameLevel;
}

namespace rpc
{
    namespace gdx
    {
        enum class difficulty
        {
            Na,
            Easy,
            Normal,
            Hard,
            Harder,
            Insane,
            Demon
        };

        enum class demon
        {
            None,
            Easy,
            Medium,
            Hard,
            Insane,
            Extreme
        };
    }

    struct gd_level
    {
        int level_id = -1;
        std::string name;
        std::string author = "-";
        int author_id = -1;
        int stars = 0;
        gdx::difficulty difficulty = gdx::difficulty::Na;
        gdx::demon demon_difficulty = gdx::demon::None;
        bool is_auto = false;
        bool is_demon = false;
    };

    struct gd_user
    {
        int id = -1;
        int account_id = -1;
        std::string name;
        int rank = -1;
    };

    struct gd_urls
    {
        std::string get_user_info = "getGJUserInfo20.php";
        std::string get_users = "getGJUsers20.php";
        std::string get_scores = "getGJScores20.php";
    };

    typedef std::multimap<std::string, std::string> params;
    typedef std::unordered_map<int, std::string> robtop_map;

    class gd_client
    {
    private:
        std::string host;
        std::string prefix;

        const int game_version;
        const std::string secret;

        gd_urls urls;

        std::shared_ptr<httplib::Client> client;

        // makes an internet post request to boomlings.com
        std::string post_request(std::string, params&);

    public:
        gd_client(
            const std::string& host   = "http://boomlings.com",
            const std::string& prefix = "/database/");

        bool get_user_info(const int account_id, gd_user& user);
        bool get_player_info(const int player_id, gd_user& user);

        bool get_user_rank(gd_user& user);

        void set_urls(const gd_urls& value) { this->urls = value; }

        static gdx::demon get_demon_difficulty_value(const int difficulty);
        static std::string get_difficulty_name(gd_level& level);
    };

    robtop_map to_robtop(
        std::string& string,
        const char delimiter = ':');

    // splits a string by substring, much like in other languages
    std::vector<std::string> explode(
        std::string& string,
        const char separator);

    bool parse_game_level(
        ::gd::GJGameLevel* in_memory,
        gd_level& level);
}