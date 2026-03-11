#include "platforms/platforms.h"

namespace silicore::collect {
const std::vector<PlatformConfig>& embedded_platforms() {
    static const std::vector<PlatformConfig> kPlatforms = [](){
        std::vector<PlatformConfig> v;
        v.reserve(70);
        {
            PlatformConfig cfg;
            cfg.name = "About.me";
            cfg.url = "https://about.me/{username}";
            cfg.url_probe = "https://about.me/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Academia.edu";
            cfg.url = "https://independent.academia.edu/{username}";
            cfg.url_probe = "https://independent.academia.edu/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[^.]*$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.86;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Archive.org";
            cfg.url = "https://archive.org/details/@{username}";
            cfg.url_probe = "https://archive.org/details/@{username}?noscript=true";
            cfg.detection_methods = {
                "message"
            };
            cfg.exists_statuses = {
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
                "could not fetch an account with user item identifier",
                "The resource could not be found",
                "Internet Archive services are temporarily offline"
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "ArtStation";
            cfg.url = "https://www.artstation.com/{username}";
            cfg.url_probe = "https://www.artstation.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Asciinema";
            cfg.url = "https://asciinema.org/~{username}";
            cfg.url_probe = "https://asciinema.org/~{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Ask Fedora";
            cfg.url = "https://ask.fedoraproject.org/u/{username}";
            cfg.url_probe = "https://ask.fedoraproject.org/u/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Atcoder";
            cfg.url = "https://atcoder.jp/users/{username}";
            cfg.url_probe = "https://atcoder.jp/users/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Bandcamp";
            cfg.url = "https://www.bandcamp.com/{username}";
            cfg.url_probe = "https://www.bandcamp.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Behance";
            cfg.url = "https://www.behance.net/{username}";
            cfg.url_probe = "https://www.behance.net/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Bitbucket";
            cfg.url = "https://bitbucket.org/{username}";
            cfg.url_probe = "https://bitbucket.org/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[a-zA-Z0-9-_]{1,30}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Blogger";
            cfg.url = "https://{username}.blogspot.com";
            cfg.url_probe = "https://{username}.blogspot.com";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.75;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "BugCrowd";
            cfg.url = "https://bugcrowd.com/{username}";
            cfg.url_probe = "https://bugcrowd.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "BuyMeACoffee";
            cfg.url = "https://buymeacoffee.com/{username}";
            cfg.url_probe = "https://buymeacoffee.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,50}$";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.78;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Codeberg";
            cfg.url = "https://codeberg.org/{username}";
            cfg.url_probe = "https://codeberg.org/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Codeforces";
            cfg.url = "https://codeforces.com/profile/{username}";
            cfg.url_probe = "https://codeforces.com/profile/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "CodePen";
            cfg.url = "https://codepen.io/{username}";
            cfg.url_probe = "https://codepen.io/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,40}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Coderwall";
            cfg.url = "https://coderwall.com/{username}";
            cfg.url_probe = "https://coderwall.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Codewars";
            cfg.url = "https://www.codewars.com/users/{username}";
            cfg.url_probe = "https://www.codewars.com/users/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Credly";
            cfg.url = "https://www.credly.com/users/{username}";
            cfg.url_probe = "https://www.credly.com/users/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Crowdin";
            cfg.url = "https://crowdin.com/profile/{username}";
            cfg.url_probe = "https://crowdin.com/profile/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[a-zA-Z0-9._-]{2,255}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.86;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "CSSBattle";
            cfg.url = "https://cssbattle.dev/player/{username}";
            cfg.url_probe = "https://cssbattle.dev/player/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "CyberDefenders";
            cfg.url = "https://cyberdefenders.org/p/{username}";
            cfg.url_probe = "https://cyberdefenders.org/p/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[^\\\\/:*?\\"<>|@]{3,50}$";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.86;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "DailyMotion";
            cfg.url = "https://www.dailymotion.com/{username}";
            cfg.url_probe = "https://www.dailymotion.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Dev.to";
            cfg.url = "https://dev.to/{username}";
            cfg.url_probe = "https://dev.to/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "DeviantArt";
            cfg.url = "https://www.deviantart.com/{username}";
            cfg.url_probe = "https://www.deviantart.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.81;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Discogs";
            cfg.url = "https://www.discogs.com/user/{username}";
            cfg.url_probe = "https://www.discogs.com/user/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Discord";
            cfg.url = "https://discord.com/users/{username}";
            cfg.url_probe = "https://discord.com/users/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.6;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Disqus";
            cfg.url = "https://disqus.com/{username}";
            cfg.url_probe = "https://disqus.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "DockerHub";
            cfg.url = "https://hub.docker.com/u/{username}";
            cfg.url_probe = "https://hub.docker.com/v2/users/{username}/";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.75;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Dribbble";
            cfg.url = "https://dribbble.com/{username}";
            cfg.url_probe = "https://dribbble.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "EyeEm";
            cfg.url = "https://www.eyeem.com/u/{username}";
            cfg.url_probe = "https://www.eyeem.com/u/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Facebook";
            cfg.url = "https://facebook.com/{username}";
            cfg.url_probe = "https://facebook.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Flickr";
            cfg.url = "https://www.flickr.com/people/{username}";
            cfg.url_probe = "https://www.flickr.com/people/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.7;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "GitHub";
            cfg.url = "https://github.com/{username}";
            cfg.url_probe = "https://github.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[a-zA-Z0-9](?:[a-zA-Z0-9]|-(?=[a-zA-Z0-9])){0,38}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.95;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "GitLab";
            cfg.url = "https://gitlab.com/{username}";
            cfg.url_probe = "https://gitlab.com/api/v4/users?username={username}";
            cfg.detection_methods = {
                "message"
            };
            cfg.exists_statuses = {
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
                "["
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "HackerOne";
            cfg.url = "https://hackerone.com/{username}";
            cfg.url_probe = "https://hackerone.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "HackerRank";
            cfg.url = "https://www.hackerrank.com/{username}";
            cfg.url_probe = "https://www.hackerrank.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,30}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.82;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Instagram";
            cfg.url = "https://instagram.com/{username}";
            cfg.url_probe = "https://instagram.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.95;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Kaggle";
            cfg.url = "https://www.kaggle.com/{username}";
            cfg.url_probe = "https://www.kaggle.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Keybase";
            cfg.url = "https://keybase.io/{username}";
            cfg.url_probe = "https://keybase.io/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_]{1,32}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.84;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "LeetCode";
            cfg.url = "https://leetcode.com/{username}/";
            cfg.url_probe = "https://leetcode.com/{username}/";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "LinkedIn";
            cfg.url = "https://www.linkedin.com/in/{username}";
            cfg.url_probe = "https://www.linkedin.com/in/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Mastodon";
            cfg.url = "https://mastodon.social/@{username}";
            cfg.url_probe = "https://mastodon.social/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.82;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Medium";
            cfg.url = "https://medium.com/@{username}";
            cfg.url_probe = "https://medium.com/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "NPM";
            cfg.url = "https://www.npmjs.com/~{username}";
            cfg.url_probe = "https://www.npmjs.com/~{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Pastebin";
            cfg.url = "https://pastebin.com/u/{username}";
            cfg.url_probe = "https://pastebin.com/u/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.7;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Patreon";
            cfg.url = "https://www.patreon.com/{username}";
            cfg.url_probe = "https://www.patreon.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.75;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Pinterest";
            cfg.url = "https://www.pinterest.com/{username}";
            cfg.url_probe = "https://www.pinterest.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "ProductHunt";
            cfg.url = "https://www.producthunt.com/@{username}";
            cfg.url_probe = "https://www.producthunt.com/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,40}$";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.79;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "PyPI";
            cfg.url = "https://pypi.org/user/{username}/";
            cfg.url_probe = "https://pypi.org/user/{username}/";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Quora";
            cfg.url = "https://www.quora.com/profile/{username}";
            cfg.url_probe = "https://www.quora.com/profile/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9-]{2,60}$";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.76;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Reddit";
            cfg.url = "https://www.reddit.com/user/{username}";
            cfg.url_probe = "https://www.reddit.com/user/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Replit";
            cfg.url = "https://replit.com/@{username}";
            cfg.url_probe = "https://replit.com/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,32}$";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.83;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Roblox";
            cfg.url = "https://www.roblox.com/users/profile?username={username}";
            cfg.url_probe = "https://www.roblox.com/users/profile?username={username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.65;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Snapchat";
            cfg.url = "https://www.snapchat.com/add/{username}";
            cfg.url_probe = "https://www.snapchat.com/add/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.7;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "SoundCloud";
            cfg.url = "https://soundcloud.com/{username}";
            cfg.url_probe = "https://soundcloud.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.75;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "SourceForge";
            cfg.url = "https://sourceforge.net/u/{username}/profile";
            cfg.url_probe = "https://sourceforge.net/u/{username}/profile";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.7;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Spotify";
            cfg.url = "https://open.spotify.com/user/{username}";
            cfg.url_probe = "https://open.spotify.com/user/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.7;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "StackOverflow";
            cfg.url = "https://stackoverflow.com/users/{username}";
            cfg.url_probe = "https://stackoverflow.com/users/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "SteamCommunity";
            cfg.url = "https://steamcommunity.com/id/{username}";
            cfg.url_probe = "https://steamcommunity.com/id/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{2,32}$";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Telegram";
            cfg.url = "https://t.me/{username}";
            cfg.url_probe = "https://t.me/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Threads";
            cfg.url = "https://www.threads.net/@{username}";
            cfg.url_probe = "https://www.threads.net/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "GET";
            cfg.confidence_weight = 0.8;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "TikTok";
            cfg.url = "https://www.tiktok.com/@{username}";
            cfg.url_probe = "https://www.tiktok.com/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "TryHackMe";
            cfg.url = "https://tryhackme.com/p/{username}";
            cfg.url_probe = "https://tryhackme.com/p/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.85;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Twitch";
            cfg.url = "https://www.twitch.tv/{username}";
            cfg.url_probe = "https://www.twitch.tv/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Twitter/X";
            cfg.url = "https://x.com/{username}";
            cfg.url_probe = "https://x.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.9;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Unsplash";
            cfg.url = "https://unsplash.com/@{username}";
            cfg.url_probe = "https://unsplash.com/@{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,30}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.78;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "Vimeo";
            cfg.url = "https://vimeo.com/{username}";
            cfg.url_probe = "https://vimeo.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200,
                301,
                302
            };
            cfg.not_found_statuses = {
                404
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "^[A-Za-z0-9_-]{1,40}$";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.77;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "WordPress";
            cfg.url = "https://{username}.wordpress.com";
            cfg.url_probe = "https://{username}.wordpress.com";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.75;
            v.push_back(cfg);
        }
        {
            PlatformConfig cfg;
            cfg.name = "YouTube";
            cfg.url = "https://www.youtube.com/{username}";
            cfg.url_probe = "https://www.youtube.com/{username}";
            cfg.detection_methods = {
                "status_code"
            };
            cfg.exists_statuses = {
                200
            };
            cfg.not_found_statuses = {
            };
            cfg.error_messages = {
            };
            cfg.error_url = "";
            cfg.regex_check = "";
            cfg.headers = {
            };
            cfg.request_method = "HEAD";
            cfg.confidence_weight = 0.95;
            v.push_back(cfg);
        }
        return v;
    }();
    return kPlatforms;
}
} // namespace silicore::collect
