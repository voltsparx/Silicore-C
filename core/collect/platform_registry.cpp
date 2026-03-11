#include "core/collect/platform_registry.h"
#include "platforms/platform_index.h"

namespace silicore::collect {

const std::vector<PlatformConfig>& embedded_platforms() {
    static const std::vector<PlatformConfig> kPlatforms = []() {
        std::vector<PlatformConfig> v;
        v.reserve(70);
        v.push_back(build_platform_about_me());
        v.push_back(build_platform_academia_edu());
        v.push_back(build_platform_archive_org());
        v.push_back(build_platform_artstation());
        v.push_back(build_platform_asciinema());
        v.push_back(build_platform_ask_fedora());
        v.push_back(build_platform_atcoder());
        v.push_back(build_platform_bandcamp());
        v.push_back(build_platform_behance());
        v.push_back(build_platform_bitbucket());
        v.push_back(build_platform_blogger());
        v.push_back(build_platform_bugcrowd());
        v.push_back(build_platform_buymeacoffee());
        v.push_back(build_platform_codeberg());
        v.push_back(build_platform_codeforces());
        v.push_back(build_platform_codepen());
        v.push_back(build_platform_coderwall());
        v.push_back(build_platform_codewars());
        v.push_back(build_platform_credly());
        v.push_back(build_platform_crowdin());
        v.push_back(build_platform_cssbattle());
        v.push_back(build_platform_cyberdefenders());
        v.push_back(build_platform_dailymotion());
        v.push_back(build_platform_dev_to());
        v.push_back(build_platform_deviantart());
        v.push_back(build_platform_discogs());
        v.push_back(build_platform_discord());
        v.push_back(build_platform_disqus());
        v.push_back(build_platform_dockerhub());
        v.push_back(build_platform_dribbble());
        v.push_back(build_platform_eyeem());
        v.push_back(build_platform_facebook());
        v.push_back(build_platform_flickr());
        v.push_back(build_platform_github());
        v.push_back(build_platform_gitlab());
        v.push_back(build_platform_hackerone());
        v.push_back(build_platform_hackerrank());
        v.push_back(build_platform_instagram());
        v.push_back(build_platform_kaggle());
        v.push_back(build_platform_keybase());
        v.push_back(build_platform_leetcode());
        v.push_back(build_platform_linkedin());
        v.push_back(build_platform_mastodon());
        v.push_back(build_platform_medium());
        v.push_back(build_platform_npm());
        v.push_back(build_platform_pastebin());
        v.push_back(build_platform_patreon());
        v.push_back(build_platform_pinterest());
        v.push_back(build_platform_producthunt());
        v.push_back(build_platform_pypi());
        v.push_back(build_platform_quora());
        v.push_back(build_platform_reddit());
        v.push_back(build_platform_replit());
        v.push_back(build_platform_roblox());
        v.push_back(build_platform_snapchat());
        v.push_back(build_platform_soundcloud());
        v.push_back(build_platform_sourceforge());
        v.push_back(build_platform_spotify());
        v.push_back(build_platform_stackoverflow());
        v.push_back(build_platform_steamcommunity());
        v.push_back(build_platform_telegram());
        v.push_back(build_platform_threads());
        v.push_back(build_platform_tiktok());
        v.push_back(build_platform_tryhackme());
        v.push_back(build_platform_twitch());
        v.push_back(build_platform_twitter_x());
        v.push_back(build_platform_unsplash());
        v.push_back(build_platform_vimeo());
        v.push_back(build_platform_wordpress());
        v.push_back(build_platform_youtube());
        return v;
    }();
    return kPlatforms;
}

} // namespace silicore::collect
