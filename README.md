# RSS Reader (Terminal Application)
*A Modern C++23 Learning Project*

## Overview

A terminal-based RSS/Atom feed reader written in modern C++23 using CMake, MinGW-w64, and CLion.

The application allows users to:

- Subscribe to RSS and Atom feeds
- Download and cache articles
- Search articles locally
- Track read/unread state
- Organize feeds into categories
- View feed statistics
- Export articles
- Eventually provide a rich TUI interface

---

# Goals

This project is designed to learn:

- Modern C++20/23
- CMake
- Dependency management
- Networking
- XML parsing
- SQLite
- Unit testing
- Clean architecture
- Concurrency
- Error handling
- Logging

Not intended as a toy project.

---

# Technology Stack

## Core

- C++23
- CMake 3.28+
- MinGW-w64
- CLion

## Libraries

### Required

- fmt
- spdlog
- sqlite3
- nlohmann-json
- libcurl
- pugixml
- Catch2

### Optional Later

- FTXUI
- cpr
- boost

## Build and test

Configure and build with the debug preset, then run the CTest suite:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --test-dir cmake-build-debug --output-on-failure
```

---

# Repository Layout

```text
rss-reader/

├── CMakeLists.txt
├── CMakePresets.json
├── README.md

├── docs/
│   ├── architecture.md
│   ├── roadmap.md
│   └── database.md

├── config/
│   └── default_config.json

├── data/
│   └── .gitkeep

├── scripts/
│   ├── bootstrap.ps1
│   └── package.ps1

├── third_party/

├── src/
│
│   ├── app/
│   │   ├── Application.hpp
│   │   └── Application.cpp
│
│   ├── cli/
│   │   ├── CommandDispatcher.hpp
│   │   ├── CommandDispatcher.cpp
│   │   ├── Commands.hpp
│   │   └── Commands.cpp
│
│   ├── config/
│   │   ├── ConfigManager.hpp
│   │   └── ConfigManager.cpp
│
│   ├── core/
│   │   ├── FeedManager.hpp
│   │   ├── FeedManager.cpp
│   │   ├── ArticleManager.hpp
│   │   └── ArticleManager.cpp
│
│   ├── database/
│   │   ├── Database.hpp
│   │   ├── Database.cpp
│   │   ├── FeedRepository.hpp
│   │   ├── FeedRepository.cpp
│   │   ├── ArticleRepository.hpp
│   │   └── ArticleRepository.cpp
│
│   ├── model/
│   │   ├── Feed.hpp
│   │   ├── Article.hpp
│   │   └── Category.hpp
│
│   ├── network/
│   │   ├── HttpClient.hpp
│   │   └── HttpClient.cpp
│
│   ├── parsers/
│   │   ├── RssParser.hpp
│   │   ├── RssParser.cpp
│   │   ├── AtomParser.hpp
│   │   └── AtomParser.cpp
│
│   ├── services/
│   │   ├── UpdateService.hpp
│   │   ├── UpdateService.cpp
│   │   ├── SearchService.hpp
│   │   └── SearchService.cpp
│
│   ├── logging/
│   │   ├── Logger.hpp
│   │   └── Logger.cpp
│
│   ├── util/
│   │   ├── Time.hpp
│   │   ├── String.hpp
│   │   ├── Url.hpp
│   │   └── Filesystem.hpp
│
│   ├── main.cpp
│
├── tests/
│
│   ├── unit/
│   │   ├── FeedManagerTests.cpp
│   │   ├── ParserTests.cpp
│   │   └── ConfigTests.cpp
│
│   ├── integration/
│   │   ├── DatabaseTests.cpp
│   │   └── UpdateTests.cpp
│
│   └── test_main.cpp
│
└── resources/
    ├── sample_feed.xml
    └── sample_atom.xml
```

---

# High-Level Architecture

```text
CLI
 │
 ▼
Application
 │
 ├── FeedManager
 ├── ArticleManager
 ├── SearchService
 └── UpdateService
         │
         ▼
     HttpClient
         │
         ▼
    RSS Parser
         │
         ▼
     SQLite
```

---

# Database Design

## feeds

```sql
CREATE TABLE feeds
(
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    url TEXT NOT NULL UNIQUE,
    category TEXT,
    created_at TEXT
);
```

---

## articles

```sql
CREATE TABLE articles
(
    id INTEGER PRIMARY KEY,

    feed_id INTEGER,

    title TEXT,
    link TEXT UNIQUE,

    summary TEXT,

    author TEXT,

    published TEXT,

    read INTEGER DEFAULT 0,

    starred INTEGER DEFAULT 0,

    FOREIGN KEY(feed_id)
    REFERENCES feeds(id)
);
```

---

## tags

```sql
CREATE TABLE tags
(
    id INTEGER PRIMARY KEY,
    name TEXT UNIQUE
);
```

---

## article_tags

```sql
CREATE TABLE article_tags
(
    article_id INTEGER,
    tag_id INTEGER
);
```

---

# Domain Models

## Feed

```cpp
struct Feed
{
    int id {};

    std::string name;

    std::string url;

    std::string category;
};
```

---

## Article

```cpp
struct Article
{
    int id {};

    int feed_id {};

    std::string title;

    std::string link;

    std::string summary;

    std::string author;

    std::chrono::sys_seconds published;

    bool read {};

    bool starred {};
};
```

---

# CLI Commands

## Feed Management

```bash
rss add <url>

rss remove <feed-id>

rss feeds
```

Examples:

```bash
rss add https://feeds.arstechnica.com/arstechnica/index

rss feeds
```

---

## Updating

```bash
rss update
```

Updates all feeds.

---

## Article Listing

```bash
rss list

rss list --feed 1

rss list --unread

rss list --today
```

---

## Reading

```bash
rss show 15

rss open 15
```

open launches browser.

---

## Search

```bash
rss search "coroutines"

rss search "llvm"
```

---

## Read Status

```bash
rss read 15

rss unread 15
```

---

## Favorites

```bash
rss star 15

rss starred
```

---

## Statistics

```bash
rss stats
```

Output:

```text
Feeds:              12
Articles:          892
Unread:             31
Starred:             7
Database Size:   15 MB
```

---

# Configuration

```json
{
  "database": "rss.db",
  "update_threads": 4,
  "timeout_seconds": 15,
  "log_level": "info",
  "open_browser": true
}
```

---

# Phase 1 (MVP)

## Features

- Add feed
- Remove feed
- Update feeds
- Parse RSS
- Store articles
- List articles
- Read article metadata
- SQLite persistence
- Logging
- Unit tests

Target:
~2 weeks

---

# Phase 2

## Features

- Read/unread tracking
- Search
- Favorites
- Categories
- Atom support
- Export

Target:
~2 weeks

---

# Phase 3

## Features

- Parallel feed updates
- Full-text search
- Feed statistics
- Feed health checking
- Article deduplication

Target:
~2 weeks

---

# Phase 4

## Features

- Terminal User Interface
- Keyboard navigation
- Feed sidebar
- Article preview pane
- Dark mode

Library:

```text
FTXUI
```

Target:
~3 weeks

---

# Recommended Modern C++ Topics

## C++20

- ranges
- views
- concepts
- spans
- format (or fmt)
- jthread
- stop_token

## C++23

- expected
- mdspan (optional)
- improved ranges

---

# Error Handling Strategy

Use:

```cpp
std::expected<T, Error>
```

Example:

```cpp
auto articles =
    repository.load_articles();

if (!articles)
{
    return articles.error();
}
```

Avoid exceptions in business logic where practical.

---

# Logging

```cpp
spdlog::info("Updating feed {}", name);

spdlog::warn("Feed unavailable");

spdlog::error("Database error {}", msg);
```

---

# Testing Strategy

## Unit Tests

Test:

- RSS parsing
- Feed management
- Search
- Configuration

Coverage goal:

```text
80%
```

---

## Integration Tests

Test:

- SQLite operations
- Feed updates
- End-to-end synchronization

---

# Future Ideas

- OPML import/export
- Feed auto-discovery
- Markdown export
- Pocket integration
- Read-it-later queue
- Embedded HTTP API
- Article summarization
- Plugin system
- Multi-user support
- Feed recommendation system

---

# Success Criteria

A successful v1 can:

1. Subscribe to feeds
2. Download updates
3. Store data locally
4. Search articles
5. Track read status
6. Operate entirely from the terminal
7. Build cleanly with:
   - MinGW-w64
   - CLion
   - CMake
8. Contain comprehensive unit tests
9. Use modern C++23 idioms throughout