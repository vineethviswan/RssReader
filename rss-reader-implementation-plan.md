# RSS Reader Implementation Plan

This plan turns the project brief in `RssReader.md` into an incremental build order. Each phase should leave the application buildable and usable before the next phase begins. It describes responsibilities, class/method boundaries, and call flow rather than providing the full implementation.

## 1. Product and architecture decisions

- **Application shape:** a single `rss` executable with a command-oriented CLI; keep command parsing separate from application services so a future TUI can reuse those services.
- **Dependency direction:** `cli -> app/services -> core/repositories -> database`; update services also depend on `network` and `parsers`. Lower layers must not depend on the CLI.
- **Persistence:** SQLite is the source of truth. Repositories own SQL and row mapping; managers/services own user-facing operations and orchestration.
- **Errors:** use `std::expected<T, Error>` at fallible boundaries where supported by the selected MinGW toolchain. Define a project `Error` type and propagate failures to the CLI, where they are logged and shown to the user. Do not silently substitute empty or success-shaped results.
- **Identifiers and dates:** settle on one ID type (for example `std::int64_t`) across models and repository APIs. Store timestamps in a documented normalized representation (UTC ISO-8601 or Unix seconds) and convert at the boundary.
- **Feed support:** implement RSS first, then Atom through the same parser result model. Keep parser-specific XML handling out of the database and service layers.
- **Scope order:** deliver terminal MVP first; implement search/read-state/categories/export and update robustness next; treat the FTXUI interface as a later client of the same services.

Before coding, resolve the source brief's schema gaps: categories are represented as text on feeds while tags use a join table; decide whether categories are initially a feed field (recommended for Phase 2) and defer article tags unless a concrete feature needs them. Add foreign-key enforcement, indexes, nullability, and schema versioning as part of database initialization.

## 2. CLion and toolchain setup (start here)

1. **Install prerequisites**
   - Install CLion and a MinGW-w64 distribution that includes a C++23-capable GCC, GDB, and the required Windows runtime libraries.
   - Install Git and CMake 3.28 or newer. Ensure the compiler, debugger, CMake, and Git are discoverable from the environment used to launch CLion.
   - Choose a dependency strategy before project configuration: use CMake packages/toolchain package manager where available, or documented prebuilt dependencies. Avoid checking machine-specific library paths into the project.
2. **Configure CLion**
   - Open the project root (the folder containing `CMakeLists.txt`) as a CMake project.
   - Under **Settings | Build, Execution, Deployment | Toolchains**, add/select the MinGW toolchain and verify compiler, debugger, and CMake detection.
   - Under **Settings | Build, Execution, Deployment | CMake**, create a Debug profile using that toolchain and a matching build directory. Add a Release profile after Debug works.
   - Configure CMake to require C++23 and verify the compiler accepts a small `std::expected` probe before adopting it throughout the codebase. If the chosen compiler/library lacks required support, upgrade it rather than scattering compatibility workarounds.
   - Configure the package/dependency paths through CMake presets or documented environment variables. Do not hard-code local CLion paths.
3. **Prove the setup**
   - Create the initial CMake target, build and run a minimal executable from CLion, and verify the debugger can stop at a breakpoint.
   - Add Catch2 and run one trivial test from CLion's test runner.
   - Record the tested compiler, CMake version, dependency setup, and configure/build/test commands in the project README.

**Exit check:** a clean Debug configure/build/test works from both CLion and a terminal using the documented setup.

## 3. Module map and responsibilities

Methods below are proposed public operations, not final signatures. Apply the project's error/result type consistently to operations that can fail.

| Module / class | Main methods and responsibility | Collaborators |
|---|---|---|
| `app::Application` | `run(args)`, `shutdown()`; construct dependencies, dispatch commands, own application lifetime | `ConfigManager`, `Logger`, `CommandDispatcher`, services |
| `cli::CommandDispatcher` | `dispatch(args)`, `print_usage()`; parse command and options, validate arguments, route to a command handler | `Commands`, managers/services |
| `cli::Commands` | `add_feed(url)`, `remove_feed(id)`, `list_feeds()`, `update(feeds)`, `list_articles(options)`, `show_article(id)`, `open_article(id)`, `search(query)`, state/favorite/stats/export handlers as added | `FeedManager`, `ArticleManager`, `UpdateService`, `SearchService` |
| `config::ConfigManager` | `load(path)`, `validate(config)`, `defaults()`; parse JSON, validate ranges and paths, return actionable errors | `Config`, `nlohmann-json` |
| `logging::Logger` | `initialize(config)`, `shutdown()`; set up spdlog sinks/level and log destination | `Config`, `spdlog` |
| `model::Feed` | Data: `id`, `name`, `url`, `category`; value type, not a service | repositories/managers |
| `model::Article` | Data: `id`, `feed_id`, title, link, summary, author, published, read, starred; value type | repositories/managers |
| `model::Category` | Data and category identity if categories become a normalized entity; do not introduce until the category storage decision is made | optional category repository/manager |
| `database::Database` | `open(path)`, `execute(sql)`, `prepare(sql)`, `begin_transaction()`, `migrate()`; own SQLite connection and schema lifecycle | SQLite, repositories |
| `database::FeedRepository` | `add(feed)`, `remove(id)`, `get(id)`, `get_by_url(url)`, `list(filter)`, `update_metadata(feed)` | `Database`, `Feed` |
| `database::ArticleRepository` | `upsert_many(feed_id, articles)`, `get(id)`, `list(query)`, `set_read(id, value)`, `set_starred(id, value)`, `search(query)` (when implemented) | `Database`, `Article` |
| `core::FeedManager` | `add_feed(url)`, `remove_feed(id)`, `list_feeds()`, `get_feed(id)`; enforce feed-level rules and coordinate repository operations | `FeedRepository`, URL utility |
| `core::ArticleManager` | `list_articles(options)`, `get_article(id)`, `mark_read(id, value)`, `mark_starred(id, value)`, `list_starred()`; article operations not tied to downloading | `ArticleRepository` |
| `network::HttpClient` | `get(url, request_options)`, return status/headers/body; own curl setup/cleanup and enforce timeout/error handling | libcurl |
| `parsers::RssParser` | `parse(xml) -> ParsedFeed`; validate XML/root and map RSS items into common parsed entries | pugixml, parsed-feed types |
| `parsers::AtomParser` | `parse(xml) -> ParsedFeed`; map Atom entries into the same common parsed entries | pugixml, parsed-feed types |
| `services::UpdateService` | `update_all()`, `update_feed(id)`, `update_one(feed)`; fetch, detect format, parse, persist, and report per-feed outcomes | feed/article repositories, `HttpClient`, parsers |
| `services::SearchService` | `search(query, filters)`; normalize/validate query and use repository search, later SQLite FTS | `ArticleRepository` |
| `util::Time` | parse/format normalized timestamps, convert to/from `sys_seconds` | parsers, repositories |
| `util::String` | trim and normalize text for parsing/search where needed | parsers, search |
| `util::Url` | validate supported HTTP(S) feed URLs and normalize only safe, well-defined cases | feed manager, HTTP client |
| `util::Filesystem` | resolve app data/config/export paths and create required directories with explicit errors | application, config, export |

### Main call flows

**Add a feed:** `main -> Application::run -> CommandDispatcher::dispatch -> Commands::add_feed -> FeedManager::add_feed -> Url validation -> FeedRepository::add -> Database`. The command prints the assigned ID only after persistence succeeds.

**Update feeds:** `Commands::update -> UpdateService::update_all -> FeedRepository::list -> for each feed: HttpClient::get -> parser selection and parse -> ArticleRepository::upsert_many -> update result`. In the initial sequential implementation, report each feed's failure without losing successful updates to other feeds.

**List/show/read:** `Commands -> ArticleManager -> ArticleRepository -> Database -> model values -> CLI formatting`. Read-state and favorite mutations follow the same route through manager methods and repository updates.

**Search:** `Commands::search -> SearchService::search -> ArticleRepository::search -> Database -> CLI formatting`. Later, change the repository implementation to FTS without changing the CLI contract.

## 4. Phase-by-phase build sequence

### Phase 0 — Project skeleton and CLion verification

**Build:** repository layout, `CMakeLists.txt`, `CMakePresets.json`, executable target, test target, `src/main.cpp`, and a minimal README setup section.

**Tasks**
- Establish targets for the application and tests; enable C++23, warnings, and CTest.
- Add required libraries incrementally through CMake: start with fmt/spdlog and Catch2, then SQLite, JSON, curl, and pugixml when their phases begin.
- Add an initial `Application` entry point that can print help/version and exit cleanly.
- Confirm Debug build, test discovery, and debugger operation in CLion before implementing domain logic.

**Exit check:** clean configure/build/test in CLion and terminal; no local absolute paths in tracked CMake files.

### Phase 1 — Core types, configuration, and logging

**Build:** `Error`/result conventions, `Feed`, `Article`, initial `Config`, `ConfigManager`, `Logger`, and small utilities.

**Tasks**
- Define shared ID, timestamp, error, and result conventions.
- Load JSON config, apply documented defaults when the file is absent, and reject malformed or out-of-range values with a useful diagnostic.
- Initialize logging before service startup; route fatal startup errors to stderr if logging cannot initialize.
- Add tests for valid config, absent config/defaults, invalid JSON, and invalid settings.

**Connection:** `Application` loads config, initializes logger, then constructs the remaining modules. Keep config and logging independent of SQLite and network code.

**Exit check:** app starts with defaults, reports config errors explicitly, and config tests pass.

### Phase 2 — SQLite foundation and feed persistence

**Build:** `Database`, schema migration/version table, `FeedRepository`, database tests, then `FeedManager`.

**Tasks**
- Open/create the configured database; enable SQLite foreign keys on every connection.
- Create versioned schema and indexes. Define how feed removal affects its articles (recommended: delete feed and its articles using a foreign-key cascade).
- Use prepared statements and transactions for writes; map SQLite failures into the shared error type.
- Implement repository add/get/list/remove and uniqueness handling for feed URLs.
- Implement `FeedManager` with URL validation and clear duplicate/not-found behavior.

**Connection:** the CLI has no SQL dependency. Feed commands call `FeedManager`, which delegates persistence to `FeedRepository`.

**Tests:** in-memory or temporary-database tests for migration, CRUD, duplicate URLs, foreign-key behavior, and transaction rollback.

**Exit check:** add/list/remove feeds work through a temporary CLI database and all persistence tests pass.

### Phase 3 — RSS parsing and article persistence

**Build:** common parsed-feed/parsed-entry types, `RssParser`, `ArticleRepository`, sample XML fixtures, parser and repository tests.

**Tasks**
- Map RSS channel metadata and item fields to domain parse results; tolerate optional fields while rejecting malformed documents with an error.
- Parse dates through `util::Time`; document behavior for missing or unparseable dates.
- Add article upsert in a transaction, deduplicating by a stable unique key (initially normalized article link; define behavior when a link is missing).
- Implement article retrieval/listing and deterministic ordering (newest first with stable tie-break).
- Keep parser output free of SQLite IDs and persistence details.

**Connection:** `RssParser::parse` produces values; `ArticleRepository::upsert_many` persists them for a feed.

**Tests:** normal RSS, optional/missing fields, malformed XML, date handling, duplicate articles, and persistence round-trip.

**Exit check:** fixture feeds parse and persist correctly without networking.

### Phase 4 — HTTP client and end-to-end feed updates

**Build:** `HttpClient`, `UpdateService`, `rss update`, update integration tests.

**Tasks**
- Use curl with explicit connect/overall timeouts, HTTP status checks, redirect policy, and bounded response handling. Initialize/clean up curl at a well-defined application lifetime.
- Fetch each configured feed sequentially first; do not add concurrency until error/reporting and repository behavior are stable.
- Detect RSS versus Atom from document/root structure or content, not solely from the URL suffix.
- For each feed: fetch -> parse -> transactional upsert -> report fetched/added/updated counts and any failure.
- Ensure one failing feed does not prevent attempts for remaining feeds; return an overall result that accurately indicates partial failure.

**Connection:** `UpdateService` depends on `HttpClient`, both parsers, and repositories; CLI formats the update summary.

**Tests:** use local fixture responses or a controlled test HTTP server; test HTTP errors, malformed feed, persistence, duplicate refresh, and partial failure. Avoid tests that depend on public feeds or the live internet.

**Exit check:** `rss update` refreshes all feeds and reports success, partial failure, or total failure accurately.

### Phase 5 — Article browsing and MVP completion

**Build:** `ArticleManager`, `rss list` filters, `rss show`, `rss open`, output formatting, MVP docs.

**Tasks**
- Implement list-all, list-by-feed, and unread filters supported by current schema; make invalid feed IDs and unsupported options explicit.
- Show article metadata and summary without mixing terminal presentation into repositories.
- Open article links through the platform browser mechanism only when configured; report launch failures rather than claiming success.
- Document commands, data/config locations, build steps, and known limitations.

**Connection:** CLI handlers call `ArticleManager`; it translates user filters into repository queries and returns model values.

**Tests:** manager filter behavior and CLI argument/format behavior, plus a manual smoke test of browser launch.

**Exit check:** Phase 1 MVP features from the brief work end-to-end from the terminal.

### Phase 6 — Read/unread, favorites, categories, and Atom

**Build:** article state mutations, category model/storage decision, `AtomParser`, expanded CLI commands.

**Tasks**
- Add `mark_read`/`mark_unread`, `star`/`unstar`, and starred listing through manager and repository methods.
- Add category assignment/filtering. If categories remain a text field on feeds, validate and document that model; normalize into a separate table only if category behavior requires it.
- Implement Atom parsing into the same common parsed-feed types as RSS.
- Ensure article refresh preserves user-managed state (read/starred) when an existing link is updated.
- Add optional export to a defined format/path with escaping and overwrite/error semantics.

**Tests:** state persistence across refresh, category filtering, Atom fixtures, and export output/error paths.

**Exit check:** all Phase 2 commands/features behave consistently for both feed formats where applicable.

### Phase 7 — Search, statistics, and feed health

**Build:** `SearchService`, repository search, statistics/health query operations, `rss search`, `rss stats`.

**Tasks**
- Start with parameterized SQLite search over title/summary; specify case behavior and escaping. Upgrade to SQLite FTS only after a baseline works and query semantics are tested.
- Add statistics queries for feed/article/unread/starred counts and database size.
- Define feed health fields (for example last attempted/last successful update, error text, and failure count) and migrate schema before exposing health output.
- Keep query and aggregation logic in repositories/services, not CLI handlers.

**Tests:** query terms with punctuation/quotes, empty results, filters, aggregate correctness, and health updates on success/failure.

**Exit check:** search and stats are reproducible against a seeded temporary database; health never hides update failures.

### Phase 8 — Parallel updates and reliability hardening

**Build:** bounded parallel update execution and robustness improvements only after sequential updates are correct.

**Tasks**
- Add configurable bounded worker count; use safe task ownership and cancellation/shutdown semantics appropriate to C++20/23 (`std::jthread`/`stop_token` if supported).
- Avoid concurrent use of one SQLite connection; persist through a serialized writer or a clearly defined connection-per-thread design.
- Preserve per-feed outcome reporting and protect rate limits/resource usage.
- Add retries only with explicit retryable-error rules, bounded attempts, and backoff.
- Test cancellation, partial failure, duplicate update races, and clean application shutdown.

**Exit check:** parallel mode is deterministic in persistence and reporting, bounded by configuration, and does not regress sequential mode.

### Phase 9 — Optional FTXUI terminal interface

**Build:** separate TUI presentation client using existing managers/services.

**Tasks**
- Add a TUI entry mode without moving business logic into UI callbacks.
- Implement feed sidebar, article list/preview, keyboard navigation, and theme support in small slices.
- Keep CLI commands available and keep service tests independent of FTXUI.

**Exit check:** users can navigate feeds/articles and change read/favorite state without changing the core service APIs.

## 5. Suggested implementation order within each phase

For each module, follow the same short loop:

1. Define the model/API and its error cases.
2. Implement the smallest useful behavior.
3. Add focused unit tests for normal and failure cases.
4. Integrate through the next higher layer.
5. Add an integration/CLI test for the completed user-visible flow.
6. Update README/docs and verify a clean build plus all relevant tests.

Avoid implementing all declared modules as empty scaffolding. Add a module when its phase can exercise it end-to-end.

## 6. Project-wide quality gates

- **Every phase:** configure/build, targeted tests, and CTest from a clean build directory.
- **At each persistence change:** migration test from the previous schema version, transaction/error tests, and temporary-database integration test.
- **At each command addition:** help/usage update, invalid-argument behavior, and at least one successful CLI flow.
- **Before v1:** verify MinGW-w64 + CLion + CMake setup from a clean checkout; exercise subscribe/update/list/search/read-state workflows; confirm tests do not require external network access.
- **Coverage:** use the brief's 80% target as a later measurement, prioritizing parser, service, and persistence behavior rather than writing tests solely to reach a percentage.
- **Documentation:** keep `README.md` for setup/commands and use `docs/architecture.md`, `docs/database.md`, and `docs/roadmap.md` for evolving design, schema, and phase status.

## 7. v1 completion checklist

- Subscribe to RSS and Atom feeds; remove feeds; persist all data locally.
- Update feeds and report per-feed failures honestly.
- List/show/search articles and track read/unread and starred state.
- Support categories and export if retained in the v1 scope.
- Build and test from CLion and terminal with the documented MinGW-w64/CMake setup.
- Include focused unit and integration tests, with no dependency on live feeds for normal test runs.
- Keep CLI, domain/service, network/parser, and persistence responsibilities separated so the optional TUI can reuse the same application services.
