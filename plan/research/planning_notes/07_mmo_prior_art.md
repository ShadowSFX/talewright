# PokeMMO, MMO techniques, backend frameworks, DB licenses, Godot guidance, RO protocol facts

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

# MMO toolkit for a Godot fork: sourced feasibility report (research as of 2026-10-13)

Every claim below has a URL. "Not verified" means I found no reputable source for it. GitHub dates come from the GitHub REST API (`api.github.com/repos/...`) on 2026-10-13. Three sites refused my fetcher: pokemmo.com, its forums and the Valve developer wiki. Claims from them are marked "search excerpt only" where they were seen only through a search result.

**Bottom line:** integrate Nakama for accounts and social features, and build our own headless-Godot zone servers. This is detailed in the Recommendation at the end.

---

## 1. PokeMMO

**Caveat:** pokemmo.com and forums.pokemmo.com both returned HTTP 403, so nothing below is confirmed against the official site. The best available sources are publisher-supplied package listings (Debian, Flathub), press coverage, and one third-party reverse-engineering blog.

| Topic | Finding | Source | Confidence |
|---|---|---|---|
| Client language | The launcher starts "child java processes (Client and Updater)". | https://manpages.debian.org/unstable/pokemmo/pokemmo.6.en.html | Medium (Debian packaging) |
| Rendering | A `--swr` option says "Otherwise the internal LWJGL2 swr will be enabled". | same | Medium |
| libGDX | A reverse-engineering blog says the client "is written in Java and uses the LibGDX library for rendering". | https://fiereu.de/posts/pokemmo-packet-snooper-1/ | Low (unofficial). The search found no official source. |
| Networking | Client uses TCP to separate login and game servers, a custom protocol that is "encrypted and compressed", and Java NIO. Class/method names are obfuscated and change between updates. | https://fiereu.de/posts/pokemmo-packet-snooper-1/ | Low (unofficial reverse engineering) |
| Server language | A 2011 developer post on PokéCommunity reportedly says the server is Java. | https://www.pokecommunity.com/threads/pokemmo-pokemon-mmo.285170/ | **Not verified** (search excerpt only, page returned 403) |
| Platforms, scale, license | Windows, macOS, Linux, Android, iOS. "Tens of thousands of players online daily in a cross-play environment". "Online-only". Described as a "Multiplayer Nintendo DS and GBA game emulator". License: Proprietary. | https://flathub.org/apps/com.pokemmo.PokeMMO | Medium (publisher listing) |
| Peak concurrent players | No official figure found. Streamscharts covers Twitch viewers only, which is not a player count. | https://streamscharts.com/games/pokemmo | **Not verified** |
| ROM pipeline | Required ROM: Black/White 1 (NDS). Optional content: FireRed and Emerald (GBA). Optional visuals: HeartGold/SoulSilver. The client has a "Locate compatible ROMS" menu. Users must hold legal rights; the developers do not supply ROMs. | https://manpages.debian.org/unstable/pokemmo/pokemmo.6.en.html, https://packages.debian.org/sid/pokemmo-installer | Medium |
| History | Live since 2012. Originally needed the FireRed v1.0 USA ROM, with optional HG/SS. | https://tomsguide.com/us/gaming-pokemon-classic-mmorpg,news-16114.html, https://lutris.net/games/pokemmo/ | Medium (press) |
| Channels / instances | Nothing found on how PokeMMO uses channels. | n/a | **Not verified** |
| Battles run server-side | No source found. | n/a | **Not verified** |
| Trading / GTL market | The GTL is the in-game player marketplace. A third-party site claims a 2% listing fee and a 4-badge requirement. A setup guide says progress and inventory are stored on the server. | https://www.pikammo.fr/Pokemmo/en/money/gtl-trading, https://tech-insider.org/?p=20316 | Low (third-party) |
| Anti-cheat | Only claims from a bot vendor, which has a commercial interest. | https://pokemmobot.alwaysdata.net/blog/is-pokemmo-botting-safe.php | **Not verified** |
| Mods | The client has a `data\mods` folder and an in-client "Mod Management" screen. Themes are selected under Settings → Interface. | https://github.com/ssjshields/archetype | Low-medium (community mod's README) |
| Legal posture | OpenMMO, an open-source reimplementation of the PokeMMO server, was "Archived after formal request from PokeMMO". Its README says its purpose "conflict[s] with the interests of the PokeMMO developers". It was Kotlin + Netty with separate login and game server modules, and it generated data from the pret decompilations. | https://github.com/openmmo-org/OpenMMO, https://fiereu.de/projects/openmmo/ | High (primary) |
| Decompilation licensing | pret/pokeemerald has no license (`license: null`), so it can't be used as a dependency. | https://api.github.com/repos/pret/pokeemerald | High |
| Precedent | Pokénet closed after a Nintendo cease-and-desist in April 2010. | https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9net | Medium (search excerpt) |

**What this means for us:**
- PokeMMO's "bring your own legally owned ROM" model is the only documented precedent for using third-party assets.
- Our toolkit should ship no Nintendo data and should provide an import pipeline for user-owned content.
- Every architectural detail about PokeMMO beyond "Java client, TCP, separate login and game servers" is unverified.

---

## 2. MMO server architecture techniques

### Server authority and the trust model
- Gambetta: "the one and only authority regarding everything that happens in the world is the server". His advice is to "Always assume the worst". https://www.gabrielgambetta.com/client-server-game-architecture.html
- Koster's Laws of Online World Design frame this as "the client is in the hands of the enemy". Koster says he didn't originate it; it may come from Kelton Flinn. https://raphkoster.com/gaming/laws.shtml
- Godot's own docs: "For competitive or persistent multiplayer games, treat all client input as untrusted." https://docs.godotengine.org/en/stable/tutorials/networking/high_level_multiplayer.html

### Client-side prediction, server reconciliation, entity interpolation
- **Prediction and reconciliation.** Inputs carry sequence numbers. The server reports the last input it processed. The client adopts the server state and re-applies the inputs the server hasn't acknowledged yet. https://www.gabrielgambetta.com/client-side-prediction-server-reconciliation.html
- **When not to predict.** For state that can't easily be reversed, such as health in a turn-based game, the client should wait for the server to confirm. This applies directly to JRPG battles (same URL).
- **Entity interpolation.** "each player sees itself in the present but sees the other entities in the past". Dead reckoning fails "where the player's direction and speed can change instantly". https://www.gabrielgambetta.com/entity-interpolation.html
- **Valve Source engine** (search excerpt only; the page returned 403): https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking
  - Default 15 ms timestep, about 66.67 ticks/s.
  - About 20 snapshots/s to clients.
  - 100 ms interpolation delay (`cl_interp 0.1`), which survives one lost snapshot.
  - Extrapolation is limited to 0.25 s.
  - Delta snapshots are sent relative to the last acknowledged update.
- **Albion Online** predicts actions on the client "while sending updates to the authoritative server". https://unity.com/blog/albion-online-cross-platform-pvp-mmo-architecture
- **ARPG click-to-move.** Path of Exile reportedly offers predictive, lockstep and auto modes, with auto switching around 150 ms ping. This is community information, not an official GGG source. https://www.pathofexile.com/forum/view-thread/2223354 (low confidence)

### Tick rates
- **EVE Online** runs a 1 Hz server tick, chosen "to keep bandwidth requirements low enough for massive space battles". The trade-off: an input sent just after a tick "can feel like it takes a second or even more". https://www.eveonline.com/news/view/paint-your-ship-red-and-make-it-faster (2025-02-19)
- **Nakama** authoritative matches range from about 1 tick/s for turn-based games to "dozens of times per second". https://heroiclabs.com/docs/nakama/concepts/multiplayer/authoritative/
- **Ragnarok Online (rAthena)** moves units by per-cell timers of 150 ms by default, rather than a global tick. See §6.

### Bandwidth: delta compression, quantization, prioritization
- Gaffer's snapshot compression article:
  - The uncompressed example needs about 17.37 Mbps.
  - "Smallest three" quaternion encoding cuts orientation from 128 to 29 bits.
  - Position quantized at 512 values per meter.
  - Deltas against the last acknowledged baseline ("a big order of magnitude win").
  - About 15 kbps when the scene is at rest.
  - https://gafferongames.com/post/snapshot_compression/
- Priority accumulator: objects that miss a packet keep their accumulated priority, so "objects that don't fit are first in line" for the next packet. https://gafferongames.com/post/state_synchronization/
- Nakama recommends keeping data messages within the 1500-byte MTU. https://heroiclabs.com/docs/nakama/concepts/multiplayer/authoritative/

### Interest management / area of interest
- **Grid regions with hysteresis (Photon).** Regions become visible when they enter the inner radius and are unsubscribed when they leave the outer radius. The demo defaults are 1 cell inner and 3 cells outer. Photon has discontinued this MMO demo, so treat it as reference only. https://doc.photonengine.com/server/v4/applications/mmo/mmo-concept, https://doc.photonengine.com/server/v4/applications/mmo/mmo-demo-client-win-grid
- **Academic comparison.** Boulanger, Kienzle and Verbrugge (NetGames '06) found that cheap tile-based methods come close to ideal visibility-based filtering, and that obstacle-aware algorithms cut update messages substantially. https://www.sable.mcgill.ca/~clump/papers/boulanger-06-comparing.pdf
- **RO in practice (rAthena).** Units are bucketed into an 8-cell block grid (`#define BLOCK_SIZE 8`), with mobs in a separate list. Range queries iterate over blocks. The visible radius is `area_size: 14`. See §6.

### Zone sharding vs seamless world vs channels/instances
- **EVE (single shard).** Proxy blades are "the public facing segment". There are about 90–100 SOL blades running 2 nodes each, plus a database tier that peaked at "over 2,000 transactions per second" and about 38,000 IOPS (2008 figures). https://www.eveonline.com/news/view/my-node-was-equipped-with-the-following...
- **Guild Wars 2 megaserver.** It creates "as many copies of a map as are needed". Placement weighs "party, guild, language, home world, and other factors". https://www.guildwars2.com/en/news/introducing-the-megaserver-system/
- **Channels.** A channel is an extra copy of a zone spun up when a zone server is crowded (Runes of Magic community wiki, low confidence). https://ffxi.allakhazam.com/wiki/Channel_(RoM)
- **Seamless world limits.** A SpatialOS (vendor) interview: one game reached "20,000 players in a relatively low-fidelity but seamless world", and a single-server seamless area is limited to "a few hundred" users. This is press quoting a vendor. https://massivelyop.com/?p=217343
- **Albion Online** has one world per region, after starting with a single US server and adding Asia and Europe. https://unity.com/blog/albion-online-cross-platform-pvp-mmo-architecture, https://gcore.com/case-studies/albion-interactive/

### Gateway/proxy pattern and login/session tokens
- **rAthena** splits into login (6900), char (6121) and map (5121) servers. The map server connects to char, and char connects to login. https://github.com/rathena/rathena/blob/master/conf/map_athena.conf, https://github.com/rathena/rathena/blob/master/conf/char_athena.conf
- **rAthena session handoff.** The char server checks the client's `login_id1`/`login_id2` session keys against its auth table before accepting. https://github.com/rathena/rathena/blob/master/src/char/char_clif.cpp (around lines 826–880)
- **Nakama sessions.** Signed JWTs validated "in memory without a database lookup". Session tokens default to 60 s and refresh tokens to 3600 s, with "24 hours to 30 days" a typical range. https://heroiclabs.com/docs/nakama/concepts/session/
- **Godot** provides `SceneMultiplayer.auth_callback` and `auth_timeout`. https://docs.godotengine.org/en/stable/classes/class_scenemultiplayer.html
- **godot-tiny-mmo** splits into a Gateway (auth and routing), a Master (accounts and orchestration) and World servers (maps and instances). https://github.com/SlayHorizon/godot-tiny-mmo

### Anti-cheat: movement and cooldowns
- **Movement (rAthena).** The client sends only a destination. The server runs `path_search(...CELL_CHKNOPASS)`, rejects paths longer than `max_walk_path` (17), and advances the unit one cell per timer. See §6.
- **Cooldowns (rAthena).** `skill_blockpc_start` arms a server-side timer `sd.scd[skill_id] = add_timer(gettick() + tick, ...)` and then tells the client. https://github.com/rathena/rathena/blob/master/src/map/skill.cpp (around lines 13915–13928)
- **Generic speed-hack checks** (distance ≤ maxSpeed × elapsed time + tolerance, then snap back): community blogs and Roblox DevForum only. Low confidence. https://bugnet.io/blog/how-to-fix-client-authoritative-movement-speed-hack, https://devforum.roblox.com/t/how-to-prevent-speed-hacking/210733
- **Godot RCE risk.** `allow_object_decoding` "Deserialized objects can contain code which gets executed". https://docs.godotengine.org/en/stable/classes/class_scenemultiplayer.html

### Transaction integrity, dupes and persistence
- **rAthena autosave.** `autosave_time: 300`, staggered: "autosave of 60 secs with 60 characters online -> one char is saved every second". `minsave_time: 100` ms.
- **rAthena event-driven saves (`save_settings`).** Saves after every trade, vending transaction, storage close, mail with attachment, auction submission and bank transaction. The config says this "decrease[s] the chance of dupes/lost items when there's a server crash", at the cost of more lag. https://github.com/rathena/rathena/blob/master/conf/map_athena.conf
- **Guild data** autosaves every 60 s (char server). https://github.com/rathena/rathena/blob/master/conf/char_athena.conf
- **Idempotency keys.** A retried request with the same key returns "a cached result of the successful operation", so "the customer is charged only once". https://stripe.com/blog/idempotency
- **Write-behind caching.** Lower database load, but writes since the last sync are lost if the cache fails. https://documentation.softwareag.com/terracotta/ehcache_210/webhelp/ehcache-webhelp/co-write_about_write_through_behind.html, https://redis.antirez.com/fundamental/write-behind.html (a mirror, not redis.io)
- **Unique item IDs and atomic trades.** Only practitioner sources: https://www.creation.dev/blog/roblox-trading-system-design, https://gamespy-archives.quaddicted.com/sites/www.planetdiablo.com/features/articles/dupeqa/index.html. Low confidence; no studio postmortem found.

### Horizontal scaling
- **Agones** (Apache-2.0, release 1.61.0) is "a library for hosting, running and scaling dedicated game servers on Kubernetes", with Fleets and FleetAutoscaler. https://agones.dev/site/docs/, https://api.github.com/repos/googleforgames/agones
- **Colyseus.** Each Room belongs to one process. Scaling across processes uses RedisPresence and RedisDriver behind a load balancer. https://docs.colyseus.io/scalability
- **Nakama.** A match lives on a single host instance. Clustering is a Nakama Enterprise (paid) feature. https://heroiclabs.com/docs/nakama/concepts/multiplayer/authoritative/, https://heroiclabs.com/enterprise/

### Rate limiting / DDoS
- **rAthena connection flood check.** `ddos_count: 5` connects within `ddos_interval: 3000` ms triggers a block; `ddos_autoreset: 600000` ms lifts it. https://github.com/rathena/rathena/blob/master/conf/packet_athena.conf
- **rAthena login brute-force ban.** 7 failures within 5 minutes gives a 5-minute IP ban. Also limits registration floods. https://github.com/rathena/rathena/blob/master/conf/login_athena.conf
- **Nakama.** `max_message_size_bytes` defaults to 4096. Clients that exceed `outgoing_queue_size` are disconnected. https://heroiclabs.com/docs/nakama/getting-started/configuration/
- **Albion Online** buys DDoS protection from Gcore (vendor case study). https://gcore.com/case-studies/albion-interactive/

### Observability
- Nakama exposes Prometheus metrics via `prometheus_port` (0 = off by default) and warns that metrics "contain sensitive server information". https://heroiclabs.com/docs/nakama/getting-started/configuration/
- I found no MMO-specific tracing guidance from a reputable source. Not verified.

---

## 3. Open-source backends and reference projects

| Project | License | Language | Godot client | Activity (GitHub API) | Use as |
|---|---|---|---|---|---|
| **Nakama** | Apache-2.0 | Go (runtime in Go, Lua, TS/JS) | Official **nakama-godot** (Apache-2.0, GDScript, "Godot Engine 4.0+", `NakamaMultiplayerBridge` carries Godot RPCs over a Nakama match) | v3.41.0 released 2026-09-18. nakama-godot's last tagged release is v3.4.0 (2024-03-19), but commits continue (2026-09-21). | **Dependency** (separate service) |
| **Colyseus** | MIT | TypeScript / Node | Official GDExtension: "in beta, and may not be stable", "does not support Godot Mono" | 0.18 released 2026-08-25 | Possible dependency (rooms/instances); adds a Node runtime |
| **SpacetimeDB** | **BSL 1.1**: allowed with "no more than one SpacetimeDB instance in production" and not as a "Database Service". Converts to AGPLv3 + linking exception on **2031-10-08**. | Rust (modules in Rust, C#, TS, C++) | Official SDK is **C# only** (Godot .NET, NuGet `SpacetimeDB.ClientSDK.Godot` 2.12.0). A third-party GDScript client exists on the Godot Asset Store. | v2.12.0 released 2026-10-13 | Reference / optional plugin. Single-instance grant and all-in-RAM design are risky for a scale-out toolkit. |
| Mirror | MIT | C# (Unity) | n/a | pushed 2026-09-19 | Reference only |
| FishNet | **Custom license**. Competing "like Software" may not "use, reverse engineer, or implement Software in part or full". | C# (Unity) | n/a | pushed 2026-10-09 | **Do not study its code for our netcode** (clean-room risk) |
| OpenSimulator | BSD | C# | n/a | 0.9.3.0 on 2024-11-08 per Wikipedia (secondary); GitHub mirror pushed 2026-10-13 | Reference (virtual world, not game-shaped) |
| Ryzom Core | **AGPL-3.0** | C++ (NeL) | n/a | pushed 2026-10-12 | Reference only |
| TrinityCore | **GPL-2.0** | C++ | n/a | pushed 2026-10-13 | Reference only (WoW emulator) |
| AzerothCore | **GPL-2.0** | C++ | n/a | pushed 2026-10-13. Split into authserver and worldserver with three MySQL databases (`acore_auth`, `acore_characters`, `acore_world`). | Reference only |
| rAthena | **GPL-3.0** | C++ | n/a | pushed 2026-10-06 (rolling, no releases) | **Best design reference for an RO-like game.** No code copying. |
| Hercules | **GPL-3.0** | C | n/a | pushed 2026-10-11 | Reference only |
| Pokémon Showdown | MIT (code) | TypeScript | n/a | pushed 2026-10-13 | Reference for the turn-based battle engine. Pokémon data is Nintendo IP (legal review needed; not verified). |
| OpenMMO (PokeMMO server clone) | n/a | Kotlin + Netty | n/a | Archived at PokeMMO's request | Avoid |
| **godot-tiny-mmo** | MIT | GDScript (Godot 4.6) | Native. Gateway/Master/World split, `WebSocketMultiplayerPeer`, custom byte-packed protocol (not Godot's synchronizers), SQLite. Self-described "Experimental". | 286 stars, pushed 2026-07-23 | **Reference; MIT code can be borrowed** |
| Source of Mana | Repo license NOASSERTION. Search says code MIT, art CC BY-SA 4.0 (not verified). | GDScript | Native Godot 4 client and server | pushed 2026-10-04 | Reference |
| godot-monke-net | **No license file** | C# | Prediction, interpolation, lag compensation addon | pushed 2026-04-03 | Reference only (no license means no reuse) |

Sources for the table:
- Nakama: https://api.github.com/repos/heroiclabs/nakama, https://github.com/heroiclabs/nakama
- nakama-godot: https://github.com/heroiclabs/nakama-godot, https://api.github.com/repos/heroiclabs/nakama-godot/releases/latest
- Colyseus: https://api.github.com/repos/colyseus/colyseus, https://docs.colyseus.io/getting-started/godot
- SpacetimeDB: https://github.com/clockworklabs/SpacetimeDB/blob/master/LICENSE.txt, https://spacetimedb.com/docs/intro/faq, https://www.nuget.org/packages/SpacetimeDB.ClientSDK.Godot/, https://store.godotengine.org/asset/plaught-armor/spacetimedb-sdk/
- Mirror: https://api.github.com/repos/MirrorNetworking/Mirror
- FishNet: https://raw.githubusercontent.com/FirstGearGames/FishNet/main/LICENSE.md
- OpenSimulator: https://en.wikipedia.org/wiki/OpenSimulator, https://api.github.com/repos/opensim/opensim
- Ryzom Core: https://api.github.com/repos/ryzom/ryzomcore
- TrinityCore: https://api.github.com/repos/TrinityCore/TrinityCore
- AzerothCore: https://api.github.com/repos/azerothcore/azerothcore-wotlk, https://www.azerothcore.org/wiki/database-manual-setup
- rAthena / Hercules: https://api.github.com/repos/rathena/rathena, https://api.github.com/repos/HerculesWS/Hercules
- Pokémon Showdown: https://api.github.com/repos/smogon/pokemon-showdown
- godot-tiny-mmo: https://github.com/SlayHorizon/godot-tiny-mmo
- Source of Mana: https://api.github.com/repos/sourceofmana/sourceofmana, https://github.com/sourceofmana/sourceofmana
- godot-monke-net: https://api.github.com/repos/grazianobolla/godot-monke-net

**More detail on Nakama:**
- Features: users and social login, storage collections, friends and groups (usable as guilds), 1-on-1/group/global chat, realtime and turn-based multiplayer, parties, matchmaker, leaderboards, purchase validation, and a web console. https://github.com/heroiclabs/nakama
- It requires "CockroachDB or another Postgres wire-compatible server" (same URL).
- Staff say the open-source edition is "not feature limited", but clustering, failover and split-brain recovery are Enterprise features. https://forum.heroiclabs.com/t/questions-about-the-capabilities-of-nakama-open-source/1783, https://heroiclabs.com/enterprise/

**Godot community scale data point (March 2026):**
- A team using the high-level API with ENet saw "server CPU spikes followed by mass disconnects of all connected players" at about 80–100 clients, on a 2-vCPU / 2 GB VM.
- They replaced it with a custom TCP layer and stability "improved significantly".
- This is anecdotal and not a benchmark. https://forum.godotengine.org/t/our-experience-building-an-mmo-tech-demo-with-godot-4-4-1-net-c/134348

**Godot MMO tutorial series** (e.g. the "Ultimate Multiplayer" dedicated-server series): the page didn't load, so not verified. https://godotforums.org/d/24226-ultimate-multiplayer-dedicated-server-tutorial-series

---

## 4. Database client licenses: safe to embed?

| Component | License | In the MIT engine fork | In proprietary game servers | Source |
|---|---|---|---|---|
| libpq (PostgreSQL) | PostgreSQL License, "similar to the BSD or MIT licenses" | **Yes** | Yes | https://www.postgresql.org/about/licence/ <!-- codespell:ignore licence --> |
| libpqxx | BSD-3-Clause | **Yes** | Yes | https://api.github.com/repos/jtv/libpqxx |
| SQLite | Public domain: "does not require a license" | **Yes** | Yes | https://sqlite.org/copyright.html |
| MariaDB Connector/C | LGPL-2.1 | Only as an optional, dynamically linked module | Yes if dynamically linked. LGPL §6 requires a "suitable shared library mechanism" (or relinkable objects), terms allowing modification "and reverse engineering for debugging", and a notice. | https://api.github.com/repos/mariadb-corporation/mariadb-connector-c, https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html |
| MySQL Connector/C++ | "GPL with the FLOSS License Exception"; commercial license "upon request" | **No** | Only with a commercial license, or if never distributed (see below). Oracle's newer Universal FOSS Exception covers combining with OSI/FSF-licensed software only, not proprietary. | https://dev.mysql.com/doc/connector-cpp/en/connector-cpp-introduction.html, https://www.mysql.com/about/legal/licensing/foss-exception, https://oss.oracle.com/licenses/universal-foss-exception/ |
| Redis server | ≤7.2: BSD-3. 7.4: RSALv2 or SSPLv1. ≥8.0: RSALv2, SSPLv1 **or AGPLv3** (announced 2025-05-01) | Don't embed. Run as an external service. | Unmodified external service is fine. Modifying it and serving it over a network brings in AGPL §13. | https://redis.io/legal/licenses/, https://redis.io/blog/agplv3/ |
| hiredis (Redis C client) | BSD-3-Clause | **Yes** | Yes | https://api.github.com/repos/redis/hiredis |
| Valkey | BSD-3-Clause (9.1.2 released 2026-09-01) | Run as an external service | **Yes (cleanest choice)** | https://api.github.com/repos/valkey-io/valkey |
| libvalkey | NOASSERTION | Not verified | Not verified | https://api.github.com/repos/valkey-io/libvalkey |

**GPL rules that apply to the reference projects in §3:**
- Linking a GPL library makes the whole combination GPL, and static vs dynamic linking makes no difference. https://www.gnu.org/licenses/gpl-faq.html
- The GPL "does not require you to release your modified version" if you only use it privately, e.g. on your own servers (same URL).
- The AGPL closes that gap: the server "must also allow them to download the source code". https://www.gnu.org/licenses/why-affero-gpl.html

So copying rAthena, TrinityCore or AzerothCore code into the MIT fork would force the fork to GPL. Copying Ryzom code would add AGPL network obligations for anyone running it.

Godot itself is MIT, its third-party components are MIT-compatible, and games made with it can be proprietary. https://godotengine.org/license/

*This is not legal advice.*

---

## 5. Godot's official guidance on dedicated servers and multiplayer

**Exporting for dedicated servers** (https://docs.godotengine.org/en/stable/tutorials/export/exporting_for_dedicated_servers.html):
- Since 4.0 any binary can run with `--headless`.
- Use a dedicated export preset. "The general recommendation is to use **Strip Visuals** whenever possible."
- That export mode automatically adds the `dedicated_server` feature tag, checked with `OS.has_feature("dedicated_server")`. Headless mode is detected with `DisplayServer.get_name() == "headless"`. A custom flag can be read with `"--server" in OS.get_cmdline_user_args()`.
- Recommends export templates ("smaller and more optimized"), systemd for automatic restart, and Docker for scaling.

**High-level multiplayer** (https://docs.godotengine.org/en/stable/tutorials/networking/high_level_multiplayer.html):
- It "sacrifices some of the fine-grained control of low-level networking for greater ease of use". The low-level API is available "for maximum control".
- It "only uses UDP".
- Done wrong, it "may lead to cheats or exploits". "Treat all client input as untrusted."
- The authority is the server by default.
- The pages I fetched contain **no MMO or scale guidance at all**. I verified the absence on those pages only.

**ENetMultiplayerPeer** (https://docs.godotengine.org/en/stable/classes/class_enetmultiplayerpeer.html):
- `create_server(port, max_clients = 32, ...)`. "any number up to 4095 may be used, although the achievable number of simultaneous clients may be far lower and depends on the application."

**SceneMultiplayer** (https://docs.godotengine.org/en/stable/classes/class_scenemultiplayer.html):
- `auth_callback` and `auth_timeout` for authentication.
- `server_relay` can be turned off.
- `max_sync_packet_size` and `max_delta_packet_size` trade completeness against loss and congestion.
- `allow_object_decoding` carries a remote-code-execution warning.

**MultiplayerSynchronizer** (https://docs.godotengine.org/en/stable/classes/class_multiplayersynchronizer.html):
- Per-peer visibility via `set_visibility_for`, `add_visibility_filter` and manual `update_visibility()`.
- `replication_interval` and `delta_interval` (0 means every network frame).
- Spawners spawn and despawn nodes according to synchronizer visibility. These are usable hooks for area-of-interest filtering, but there is no built-in spatial AoI.

**Smaller server builds:** `disable_3d=yes` saves about 15%, and build profiles via `build_profile=...gdbuild`. The page doesn't mention dedicated servers. https://docs.godotengine.org/en/stable/engine_details/development/compiling/optimizing_for_size.html

The Godot 4.0 scene-replication blog post (2023) defers visibility to "a separate tutorial". https://godotengine.org/article/multiplayer-in-godot-4-0-scene-replication/

---

## 6. Ragnarok Online protocol facts (rAthena source, master branch as of 2026-10-13)

**Visibility radius**
- `area_size: 14`, commented "Visible area size (how many squares away from a player they can see)". https://github.com/rathena/rathena/blob/master/conf/battle/client.conf
- `#define AREA_SIZE battle_config.area_size`. https://github.com/rathena/rathena/blob/master/src/map/map.hpp

**Spatial grid**
- `#define BLOCK_SIZE 8`.
- `map_foreachinrangeV` loops blocks from `x0/BLOCK_SIZE` to `x1/BLOCK_SIZE` (same for y), reading separate `block` and `block_mob` arrays.
- https://github.com/rathena/rathena/blob/master/src/map/map.cpp

**Walk speed**
- `DEFAULT_WALK_SPEED 150` ("other than NPC"), `DEFAULT_NPC_WALK_SPEED 200`, `MIN_WALK_SPEED 20`, `MAX_WALK_SPEED 1000`. https://github.com/rathena/rathena/blob/master/src/common/mmo.hpp
- Each step arms `add_timer(tick + speed, unit_walktoxy_timer, ...)`. Diagonal steps take `speed * MOVE_DIAGONAL_COST / MOVE_COST`, i.e. ×14/10. https://github.com/rathena/rathena/blob/master/src/map/unit.cpp, https://github.com/rathena/rathena/blob/master/src/map/path.hpp
- Reading "150" as milliseconds per cell is my inference from timers being in ms ticks. Consistent with the code, but not stated in a doc comment.

**Server-side pathfinding**
- `clif_parse_WalkToXY` reads only the target `x, y` from the packet, then calls `unit_walktoxy(sd, x, y, 4)`. https://github.com/rathena/rathena/blob/master/src/map/clif.cpp (around lines 11379–11425)
- `unit_walktoxy` runs `path_search(..., CELL_CHKNOPASS)` and rejects `path_len > battle_config.max_walk_path`. https://github.com/rathena/rathena/blob/master/src/map/unit.cpp
- `max_walk_path: 17` ("how many cells a player can walk going to cursor"); `MAX_WALKPATH 32`. https://github.com/rathena/rathena/blob/master/conf/battle/client.conf, https://github.com/rathena/rathena/blob/master/src/map/path.hpp

**Movement broadcast**
- The mover receives `0087 <walk start time>.L <walk data>.6B (ZC_NOTIFY_PLAYERMOVE)`.
- Others receive `clif_move` → `clif_set_unit_walking(..., AREA_WOS)`, i.e. the area excluding self.
- So clients interpolate along a path the server has announced. https://github.com/rathena/rathena/blob/master/src/map/clif.cpp

**Packet versioning**
- `PACKETVER` uses YYYYMMDD (default `20211103`). Set it in `src/custom/defines_pre.hpp` or with `./configure --enable-packetver=`. https://github.com/rathena/rathena/blob/master/src/config/packets.hpp
- Features are gated on it, e.g. `MAX_CHARS` is 9, 12 or 15 depending on version.
- `PACKET_OBFUSCATION` needs PACKETVER ≥ 2011-08-17.
- The login packet `0x64` carries a `Version` field. https://github.com/rathena/rathena/wiki/Packets

**Persistence, cooldowns, rate limits:** covered in §2 (`autosave_time` 300 s staggered, `save_settings` on trades and vending, `skill_blockpc_start` timers, `ddos_*` and login IP bans).

---

## Recommendation

**Integrate Nakama for the "meta" layer and build our own Godot zone/instance servers.** Keep a backend-adapter interface so Nakama stays optional.

### Our own servers (shipped in the fork)
- **Zone/instance servers** run as headless Godot exports (`dedicated_server` tag, Strip Visuals; see §5).
- **Own low-level protocol.** Use `MultiplayerPeer`/`PacketPeer` and a compact binary protocol rather than `MultiplayerSynchronizer` for open-world entities. Reasons: the docs give no scale guidance, the community demo hit trouble at about 100 clients, and godot-tiny-mmo already bypasses the synchronizers (§3, §5).
- **ARPG mode (RO-like).**
  - Server-owned click-to-move: client sends destination, server pathfinds, enforces a max path length, and advances cells on per-unit timers (150 ms default).
  - 8-cell block grid with an AoI radius of about 14 cells, plus Photon-style inner/outer hysteresis.
  - Server-timed cooldowns.
  - Path-start broadcasts for client interpolation.
  - All modeled on rAthena's design (§6), written clean-room because rAthena is GPL.
- **JRPG mode (PokeMMO-like).**
  - Tile movement with prediction plus reconciliation for the local player (Gambetta).
  - Battles as server-side, instanced, turn-based simulations: the client waits for confirmation rather than predicting (Gambetta). They can run in Godot instance processes or as Nakama authoritative matches (1 Hz-class ticks are fine).
- **Topology.** Gateway/login → world directory → zone processes, with GW2-style channel/copy allocation. Agones (Apache-2.0) is an optional orchestrator.

### Nakama (Apache-2.0, run as a separate service)
- Covers: auth and JWT sessions, friends, groups/guilds, chat, parties, matchmaker, storage, wallets and leaderboards.
- Zone servers validate the Nakama session token on handoff, the same role as rAthena's `login_id1/2` keys.
- Market (GTL-like) and trades: transactional PostgreSQL with unique item IDs, idempotency keys and event-triggered saves (rAthena `save_settings`, Stripe idempotency).

### Pros of this option
- Avoids rebuilding mature social and account infrastructure. Nakama: 13.5k stars, release in September 2026.
- Official Godot 4 client.
- Apache-2.0 is compatible with an MIT fork; just keep its NOTICE.
- Prometheus metrics built in.

### Cons of this option
- A second language for server logic (Go, TS or Lua).
- Open-source Nakama has **no clustering**; that's an Enterprise license, so the meta layer has a single-node ceiling.
- Matches are pinned to one node.
- Needs CockroachDB or a Postgres-wire database.
- nakama-godot's last *tagged* release was in 2024, though commits continue.

### Alternatives
- **Build everything ourselves:** one language, full control, clean MIT. But the scope is large (auth, social, chat, storage, scaling) and the security risk is higher.
- **Colyseus** (MIT): a good fit for room-shaped instances, but adds a Node runtime, its Godot SDK is beta with no Mono support, and it has no account/social suite comparable to Nakama's.
- **SpacetimeDB**: an attractive all-in-one (the vendor says BitCraft runs entirely on it). But:
  - BSL 1.1 allows only **one production instance** and no DB-as-a-service until 2031-10-08.
  - The "practical limit is the available RAM".
  - The official Godot path is C# only.
  - Offer it as a third-party plugin at most, never a core dependency.

### License guardrails
- Engine fork may contain only permissive code: libpq/libpqxx, SQLite, hiredis, Godot/ENet (MIT).
- MariaDB Connector/C only as an optional, dynamically linked module. Avoid the MySQL Connector/C++ GPL connectors.
- Use Valkey (BSD-3) rather than Redis 8 (AGPL option) for caching and pub/sub, to avoid questions.
- Treat rAthena, Hercules, TrinityCore, AzerothCore (GPL) and Ryzom (AGPL) as design references only. Don't copy their code.
- Don't read FishNet's code for our netcode.
- Ship no Nintendo or Gravity data. Provide a user-owned asset import pipeline, the only documented precedent (PokeMMO's ROM model). Note the OpenMMO takedown and the 2010 Pokénet cease-and-desist (§1).

*Not legal advice; have counsel review the licensing and IP points before launch.*
