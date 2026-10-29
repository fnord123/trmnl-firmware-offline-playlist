#include <playlist.h>
#include <unity.h>

static PlaylistManifest parse(const char *json) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  TEST_ASSERT_MESSAGE(!error, error.c_str());
  return parsePlaylist_manifest(doc);
}

// Every malformed manifest must land in the same state as "the server sent no playlist", so the
// caller falls back to stock behaviour and the device never rotates over a half-parsed set.
static void assert_rejected(const PlaylistManifest &manifest) {
  TEST_ASSERT_FALSE(manifest.present);
  TEST_ASSERT_FALSE(manifest.unchanged);
  TEST_ASSERT_EQUAL_STRING("", manifest.checksum.c_str());
  TEST_ASSERT_EQUAL_INT(0, (int)manifest.entries.size());
  TEST_ASSERT_FALSE(manifest.has_entries());
}

void test_absent_playlist_is_not_present(void) {
  const PlaylistManifest manifest = parse(R"({"status":200,"image_url":"http://example.com/a.bmp"})");

  assert_rejected(manifest);
}

void test_null_playlist_is_absent(void) { assert_rejected(parse(R"({"playlist":null})")); }

void test_non_object_playlist_is_rejected(void) {
  assert_rejected(parse(R"({"playlist":"abc123"})"));
  assert_rejected(parse(R"({"playlist":[]})"));
  assert_rejected(parse(R"({"playlist":42})"));
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

void test_entry_without_filename_rejects_manifest(void) {
  const PlaylistManifest manifest = parse(R"({
      "playlist": {
        "checksum": "c2",
        "entries": [
          {"filename": "good.png", "url": "http://e/good.png", "sha256": "aa"},
          {"filename": "", "url": "http://e/b.png", "sha256": "bb"}
        ]
      }
    })");

  assert_rejected(manifest);
}

void test_entry_without_url_rejects_manifest(void) {
  const PlaylistManifest manifest = parse(R"({
      "playlist": {
        "checksum": "c3",
        "entries": [{"filename": "a.png", "url": "", "sha256": "aa"}]
      }
    })");

  assert_rejected(manifest);
}

void test_entry_without_url_field_rejects_manifest(void) {
  const PlaylistManifest manifest = parse(R"({
      "playlist": {"checksum": "c4", "entries": [{"filename": "a.png", "sha256": "aa"}]}
    })");

  assert_rejected(manifest);
}

void test_non_object_entry_rejects_manifest(void) {
  const PlaylistManifest manifest = parse(R"({
      "playlist": {
        "checksum": "c5",
        "entries": [{"filename": "a.png", "url": "http://e/a.png"}, "http://e/b.png"]
      }
    })");

  assert_rejected(manifest);
}

void test_entries_not_an_array_rejects_manifest(void) {
  assert_rejected(parse(R"({"playlist":{"checksum":"c6","entries":{"filename":"a.png"}}})"));
  assert_rejected(parse(R"({"playlist":{"checksum":"c7","entries":"a.png"}})"));
  // A manifest that is neither an ack nor an entry list carries nothing usable.
  assert_rejected(parse(R"({"playlist":{"checksum":"c8"}})"));
}

void test_entries_without_checksum_reject_manifest(void) {
  assert_rejected(parse(R"({"playlist":{"entries":[{"filename":"a.png","url":"http://e/a.png"}]}})"));
  assert_rejected(parse(R"({"playlist":{"checksum":"","entries":[{"filename":"a.png","url":"http://e/a.png"}]}})"));
}

void setUp(void) {}
void tearDown(void) {}

void process() {
  UNITY_BEGIN();
  RUN_TEST(test_absent_playlist_is_not_present);
  RUN_TEST(test_null_playlist_is_absent);
  RUN_TEST(test_non_object_playlist_is_rejected);
  RUN_TEST(test_unchanged_ack);
  RUN_TEST(test_empty_entries_deactivates);
  RUN_TEST(test_two_entries_keep_order_and_fields);
  RUN_TEST(test_missing_sha256_is_allowed);
  RUN_TEST(test_entry_without_filename_rejects_manifest);
  RUN_TEST(test_entry_without_url_rejects_manifest);
  RUN_TEST(test_entry_without_url_field_rejects_manifest);
  RUN_TEST(test_non_object_entry_rejects_manifest);
  RUN_TEST(test_entries_not_an_array_rejects_manifest);
  RUN_TEST(test_entries_without_checksum_reject_manifest);
  UNITY_END();
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  process();
  return 0;
}
