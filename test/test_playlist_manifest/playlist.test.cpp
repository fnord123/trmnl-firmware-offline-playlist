#include <playlist.h>
#include <unity.h>

static PlaylistManifest parse(const char *json) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  TEST_ASSERT_MESSAGE(!error, error.c_str());
  return parsePlaylist_manifest(doc);
}

void test_absent_playlist_is_not_present(void) {
  const PlaylistManifest manifest = parse(R"({"status":200,"image_url":"http://example.com/a.bmp"})");

  TEST_ASSERT_FALSE(manifest.present);
  TEST_ASSERT_FALSE(manifest.unchanged);
  TEST_ASSERT_EQUAL_STRING("", manifest.checksum.c_str());
  TEST_ASSERT_EQUAL_INT(0, (int)manifest.entries.size());
  TEST_ASSERT_FALSE(manifest.has_entries());
}

void test_unchanged_ack(void) {
  const PlaylistManifest manifest = parse(R"({"playlist":{"checksum":"abc123","unchanged":true}})");

  TEST_ASSERT_TRUE(manifest.present);
  TEST_ASSERT_TRUE(manifest.unchanged);
  TEST_ASSERT_EQUAL_STRING("abc123", manifest.checksum.c_str());
  TEST_ASSERT_EQUAL_INT(0, (int)manifest.entries.size());
  // An ack carries nothing to rotate over.
  TEST_ASSERT_FALSE(manifest.has_entries());
}

void test_empty_entries_deactivates(void) {
  const PlaylistManifest manifest = parse(R"({"playlist":{"checksum":"abc123","entries":[]}})");

  TEST_ASSERT_TRUE(manifest.present);
  TEST_ASSERT_FALSE(manifest.unchanged);
  TEST_ASSERT_EQUAL_STRING("abc123", manifest.checksum.c_str());
  TEST_ASSERT_EQUAL_INT(0, (int)manifest.entries.size());
  TEST_ASSERT_FALSE(manifest.has_entries());
}

void test_two_entries_keep_order_and_fields(void) {
  const PlaylistManifest manifest = parse(R"({
      "playlist": {
        "checksum": "sha:42",
        "entries": [
          {"filename": "weather.png", "url": "http://example.com/weather.png", "sha256": "aa"},
          {"filename": "clock.png", "url": "http://example.com/clock.png", "sha256": "bb"}
        ]
      }
    })");

  TEST_ASSERT_TRUE(manifest.present);
  TEST_ASSERT_FALSE(manifest.unchanged);
  TEST_ASSERT_EQUAL_STRING("sha:42", manifest.checksum.c_str());
  TEST_ASSERT_EQUAL_INT(2, (int)manifest.entries.size());
  TEST_ASSERT_TRUE(manifest.has_entries());

  TEST_ASSERT_EQUAL_STRING("weather.png", manifest.entries[0].filename.c_str());
  TEST_ASSERT_EQUAL_STRING("http://example.com/weather.png", manifest.entries[0].url.c_str());
  TEST_ASSERT_EQUAL_STRING("aa", manifest.entries[0].sha256.c_str());
  TEST_ASSERT_EQUAL_STRING("clock.png", manifest.entries[1].filename.c_str());
  TEST_ASSERT_EQUAL_STRING("http://example.com/clock.png", manifest.entries[1].url.c_str());
  TEST_ASSERT_EQUAL_STRING("bb", manifest.entries[1].sha256.c_str());
}

void test_missing_sha256_is_allowed(void) {
  const PlaylistManifest manifest =
    parse(R"({"playlist":{"checksum":"c1","entries":[{"filename":"a.png","url":"http://e/a.png"}]}})");

  TEST_ASSERT_TRUE(manifest.present);
  TEST_ASSERT_EQUAL_INT(1, (int)manifest.entries.size());
  TEST_ASSERT_EQUAL_STRING("", manifest.entries[0].sha256.c_str());
  TEST_ASSERT_TRUE(manifest.has_entries());
}

void test_entries_without_filename_or_url_are_dropped(void) {
  const PlaylistManifest manifest = parse(R"({
      "playlist": {
        "checksum": "c2",
        "entries": [
          {"filename": "", "url": "http://e/a.png", "sha256": "aa"},
          {"filename": "b.png", "url": "", "sha256": "bb"},
          {"filename": "c.png", "url": "http://e/c.png", "sha256": "cc"}
        ]
      }
    })");

  TEST_ASSERT_TRUE(manifest.present);
  TEST_ASSERT_EQUAL_INT(1, (int)manifest.entries.size());
  TEST_ASSERT_EQUAL_STRING("c.png", manifest.entries[0].filename.c_str());
  TEST_ASSERT_TRUE(manifest.has_entries());
}

void test_all_entries_dropped_is_a_deactivation(void) {
  const PlaylistManifest manifest =
    parse(R"({"playlist":{"checksum":"c3","entries":[{"filename":"","url":""},{"filename":"b.png"}]}})");

  TEST_ASSERT_TRUE(manifest.present);
  TEST_ASSERT_FALSE(manifest.unchanged);
  TEST_ASSERT_EQUAL_INT(0, (int)manifest.entries.size());
  TEST_ASSERT_FALSE(manifest.has_entries());
}

void test_entries_without_checksum_are_rejected(void) {
  const PlaylistManifest manifest =
    parse(R"({"playlist":{"entries":[{"filename":"a.png","url":"http://e/a.png","sha256":"aa"}]}})");

  TEST_ASSERT_FALSE(manifest.present);
  TEST_ASSERT_FALSE(manifest.unchanged);
  TEST_ASSERT_EQUAL_STRING("", manifest.checksum.c_str());
  TEST_ASSERT_EQUAL_INT(0, (int)manifest.entries.size());
  TEST_ASSERT_FALSE(manifest.has_entries());
}

void test_non_object_playlist_is_absent(void) {
  TEST_ASSERT_FALSE(parse(R"({"playlist":null})").present);
  TEST_ASSERT_FALSE(parse(R"({"playlist":"abc123"})").present);
  TEST_ASSERT_FALSE(parse(R"({"playlist":[]})").present);
}

void setUp(void) {}
void tearDown(void) {}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_absent_playlist_is_not_present);
  RUN_TEST(test_unchanged_ack);
  RUN_TEST(test_empty_entries_deactivates);
  RUN_TEST(test_two_entries_keep_order_and_fields);
  RUN_TEST(test_missing_sha256_is_allowed);
  RUN_TEST(test_entries_without_filename_or_url_are_dropped);
  RUN_TEST(test_all_entries_dropped_is_a_deactivation);
  RUN_TEST(test_entries_without_checksum_are_rejected);
  RUN_TEST(test_non_object_playlist_is_absent);
  UNITY_END();
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  process();
  return 0;
}
