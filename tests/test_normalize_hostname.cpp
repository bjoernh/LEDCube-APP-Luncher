#include <catch2/catch_test_macros.hpp>
#include "cube/detail/hostname_utils.hpp"

using cube::detail::normalize_hostname;

TEST_CASE("empty string returns empty", "[hostname]") {
    CHECK(normalize_hostname("") == "");
}

TEST_CASE("no dot: lowercase input is uppercased", "[hostname]") {
    CHECK(normalize_hostname("myhost") == "MYHOST");
}

TEST_CASE("no dot: already uppercase input is unchanged", "[hostname]") {
    CHECK(normalize_hostname("MYHOST") == "MYHOST");
}

TEST_CASE("no dot: mixed case input is uppercased", "[hostname]") {
    CHECK(normalize_hostname("MyHost") == "MYHOST");
}

TEST_CASE("one dot: only part before dot, uppercased", "[hostname]") {
    CHECK(normalize_hostname("myhost.local") == "MYHOST");
}

TEST_CASE("multiple dots: only first label, uppercased", "[hostname]") {
    CHECK(normalize_hostname("host.example.com") == "HOST");
}

TEST_CASE("dot at position 0: result is empty", "[hostname]") {
    CHECK(normalize_hostname(".hidden") == "");
}

TEST_CASE("uppercase FQDN: first label returned unchanged", "[hostname]") {
    CHECK(normalize_hostname("HOST.EXAMPLE.COM") == "HOST");
}
