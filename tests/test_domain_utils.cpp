#include "core/collect/domain_collector.h"

#include <gtest/gtest.h>

using namespace silicore::collect;

TEST(DomainUtils, NormalizeDomain) {
    EXPECT_EQ(normalize_domain(" https://Example.com/path "), "example.com");
    EXPECT_EQ(normalize_domain("http://sub.example.com///"), "sub.example.com");
}

TEST(DomainUtils, ParseCtSubdomains) {
    std::string body = R"([
        {"name_value": "a.example.com"},
        {"name_value": "b.example.com\nC.Example.com"}
    ])";
    auto subdomains = parse_ct_subdomains(body, "example.com", 10);
    EXPECT_EQ(subdomains.size(), 3u);
}

TEST(DomainUtils, ParseRdapInfo) {
    std::string body = R"({
        "handle": "EXAMPLE-1",
        "nameservers": [{"ldhName": "ns1.example.com"}],
        "entities": [{
            "roles": ["registrar"],
            "vcardArray": ["vcard", [["fn", {}, "text", "Example Registrar"]]]
        }]
    })";
    auto rdap = parse_rdap_info(body);
    EXPECT_EQ(rdap.handle, "EXAMPLE-1");
    EXPECT_EQ(rdap.registrar, "Example Registrar");
    ASSERT_EQ(rdap.name_servers.size(), 1u);
    EXPECT_EQ(rdap.name_servers[0], "ns1.example.com");
}
