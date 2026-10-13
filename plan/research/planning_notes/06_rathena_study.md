# rAthena d4b8e7b8 architecture study (clean-room technique extraction)

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

# rAthena architecture study (d4b8e7b8), as input for a clean-room MMO toolkit

**Scope.** I checked out commit `d4b8e7b8f16061cc2496d377ac8f777ce72a4f39` ("Remove duplicate Trade block from Sinulog Hat", 2026-10-06). Every rAthena path below is relative to `C:\Users\Relic\Desktop\ro server\rathena\`. Section 10's paths are relative to `C:\Users\Relic\Desktop\ro server\`. Everything was read-only: nothing was built, run or written. Anything I could not confirm is marked "not verified".

**Summary.** rAthena's robustness comes from a few disciplines:
- One thread per process, driven by a timer heap and socket polling.
- Timers that carry entity IDs rather than pointers, and check they still own the entity before acting.
- One-time login tokens passed between servers, plus an explicit login / logout / map-change state for each player that is replayed when a server link comes back.
- The server decides all movement; the client only sends a destination.
- A grid of 8x8-cell blocks for "who can see whom", with deferred deletion while iterating.
- Saves that write only what changed, spread over time.
- A bytecode script VM whose NPC dialogs pause and resume.
- Versioned YAML data files with override layers and live reload.

Its main weaknesses: synchronous SQL on the game thread, MyISAM tables with no transactions on character saves, raw C structs copied byte-for-byte between servers, hard-coded limits, and fragile packet handling.

---

## 1. Process topology

**Common core**
- All five servers subclass `Core`: `src/login/login.hpp:21`, `src/char/char.hpp:22`, `src/map/map.hpp:31`, `src/web/web.hpp:19`.
- `Core::start` initializes SQL, the DB maps, signals, timers and sockets, then runs the loop (`src/common/core.cpp:333-422`).

**What each server owns**

| Server | Owns |
|---|---|
| **login** | Accounts; `auth_db` (pending logins); `online_db` (`src/login/login.cpp:43-44`); a registry `ch_server[MAX_SERVERS=5]` of char-servers (`src/login/login.hpp:62,73`); IP bans; global account variables (`##`). |
| **char** | Character rows plus an in-memory `char_db` cache; its own `auth_db` and `online_char_db` (`src/char/char.cpp:78-86`); the map-server registry `map_server[MAX_MAP_SERVERS=2]` (`src/char/char.hpp:212-220`). It is also the **inter-server**: parties, guilds, storage, mail, pets and so on (`src/char/inter.cpp:1408-1453`). |
| **map** | The whole world simulation. Its "intif" link to the inter-server shares the same socket as the char link (`src/map/intif.cpp:51-52`). |
| **web** | An HTTP side-service on its own thread (`src/web/web.cpp:479-523`). It authenticates with a `web_auth_token` that login writes (`src/web/auth.cpp:16-36`; `src/login/account.cpp:651-709`). |

**Client handoff (login → char → map)**
1. **Login authenticates.** It creates two random 32-bit keys, `login_id1` and `login_id2` (`src/login/login.cpp:413-414`).
   - With no char-server online it refuses the client (`src/login/loginclif.cpp:71-81`).
   - If the account is already online it asks every char-server to kick it (0x2734) and answers "still recognizes last login" (`loginclif.cpp:84-98`).
   - Otherwise it sends the char-server list, creates an auth node, and marks the account online with a 30 s expiry timer (`loginclif.cpp:114-167`; `AUTH_TIMEOUT 30000` at `src/login/login.hpp:150`).
2. **Client connects to char** (0x65).
   - If an auth node exists from a map-server's "back to character select", it is accepted locally.
   - Otherwise char asks login with 0x2712 (`src/char/char_clif.cpp:821-891`).
   - Login checks account, both keys and sex, then **deletes the node: each auth is single-use** (`src/login/loginchrif.cpp:72-124`, erase at :108).
3. **Character select.** Char sends `HC_NOTIFY_ZONESVR` with the map-server's IP and port (`char_clif.cpp:902-921`), and creates a char-side auth node (`char_clif.cpp:1026-1037`, `:1192-1196`).
4. **Client connects to map.** `clif_parse_WantToConnection`:
   - rejects a double login, or an existing auth node that is not in `ST_LOGIN` (`src/map/clif.cpp:10713-10719`);
   - creates the session, then sends 0x2b26 (`clif.cpp:10721-10742`; `src/map/chrif.cpp:627-645`).
5. **Char validates.** It replies 0x2afd carrying the **entire `mmo_charstatus` struct** and deletes the node (`src/char/char_mapif.cpp:1004-1090`).
6. **Map loads the player in stages.** `chrif_authok` → `pc_authok` (`chrif.cpp:650-710`; `src/map/pc.cpp:2080`). Then:
   - variables are requested (`pc.cpp:2304`);
   - inventory, cart and storage are requested (`pc.cpp:2434-2436`);
   - skill cooldowns and bonus scripts are requested (`pc.cpp:2474-2475`);
   - status-effect data arrives and sets `pc_loaded` (`pc.cpp:14791-14804`).

   `LoadEndAck` is ignored until loading is complete (`clif.cpp:10756-10760`).

**How map-servers register and split maps**
- A map-server logs in to char with 0x2af8, sending a server account/password plus its own IP and port (`chrif.cpp:352-364`); char accepts in `chclif_parse_maplogin` (`char_clif.cpp:786-810`).
- It then sends its **non-instance** map list with 0x2afa (`chrif.cpp:367-375`).
- Char stores the list and forwards it to the other map-servers (0x2b04), and sends the newcomer the others' lists (`char_mapif.cpp:198-274`). `char_search_mapserver` finds which server hosts a map (`char.cpp:2093-2109`).
- **Each map-server loads only the `map:` lines in its own config** (`conf/map_athena.conf:126` imports `conf/maps_athena.conf`; parsed at `src/map/map.cpp:4166-4167`).

**Warping to a map on another map-server**
1. `pc_setpos` sees the map is not local, saves with `CSAVE_CHANGE_MAPSERV`, and sends 0x2b05 (`pc.cpp:7046-7073`; `chrif.cpp:423-451`).
2. Char creates an auth node flagged `changing_mapservers` and acks with 0x2b06 (`char_mapif.cpp:609-678`).
3. The source map-server tells the client where to go (`chrif.cpp:455-472`).

**Inter-server packet families**

| Range | Direction | Where defined |
|---|---|---|
| 0x27xx | login ↔ char | `src/login/loginchrif.cpp:803-824` |
| 0x2af8–0x2b2f | char ↔ map | length table and per-ID docs at `src/map/chrif.cpp:42-107` |
| 0x30xx | map → inter | `src/char/inter.cpp:58-70`, `:1408-1453` |
| 0x38xx | inter → map | `src/map/intif.cpp:37-49`, `:3785-3884` |

Representative packets:
- **0x2b01 save.** The whole status struct plus a "quitting" flag (`chrif.cpp:317-328`). Char checks the struct size before writing (`char_mapif.cpp:408-446`) and acks a final save with 0x2b21.
- **0x2aff full online list.** Char rebuilds who-is-where from it (`char_mapif.cpp:363-399`).
- **0x2b1c / 0x2b1d status-effect data** (`char_mapif.cpp:282-338`).
- **0x3001 whisper.** Char checks the name in SQL, then fans out to all map-servers, tracking replies by id and count (`inter.cpp:1167-1257`).
- **0x3004 variable save** (`inter.cpp:1272-1326`).
- **0x3000 broadcast** (`inter.cpp:1131-1135`).
- Party 0x382x, guild 0x383x, mail 0x384x, storage 0x388a-c (`intif.cpp:3801-3877`).

**Reconnect behavior**

*Map side, when char drops:*
- `chrif_on_disconnect` resets the map-server registry and schedules a reconnect in 1 s (`chrif.cpp:1498-1508`). A 10 s interval timer keeps retrying (`chrif.cpp:1855-1879`, `:1965`).
- While disconnected, `chrif_check` turns every inter call into a no-op (`chrif.cpp:124`, `:293`: "Character is saved on reconnect").
- Once reconnected (`chrif_on_ready`) it:
  - resends the full online list;
  - replays the pending auth states: resends final saves for `ST_LOGOUT`, re-requests `ST_MAPCHANGE`, fails `ST_LOGIN` nodes that have no data yet;
  - re-saves dirty storages and guild castles (`chrif.cpp:507-565`).
- Keepalive: on stall it sends a ping and gives up after `2*stall_time` (`chrif.cpp:1735-1743`).

*Char side, when a map-server drops:*
- Char tells the other map-servers (0x2b20) and marks that server's characters as `server=-2` (unknown) (`char_mapif.cpp:1505-1530`).
- Characters are re-claimed from the next 0x2aff list; a 10-minute cleanup timer clears leftovers (`char.cpp:2206-2229`, `:3247`).

*Char ↔ login:* char reconnects every 10 s and resyncs online accounts hourly (`src/char/char_logif.cpp:847-852`). Login marks every account of a dropped char-server offline (`loginchrif.cpp:866-879`).

## 2. Network layer

**Sessions and buffers**
- `socket_data` holds flags (eof, server, ping), read/write buffers, last-recv/last-send times, three function pointers (recv, send, parse) and a `session_data` pointer (`src/common/socket.hpp:92-114`).
- `session[MAXCONN]`, where `MAXCONN = FD_SETSIZE` (`socket.hpp:24-26`). `FD_SETSIZE` is 4096 on Windows (`CMakeLists.txt:90-92`; `src/map/map-server.vcxproj:110`).
- Read/write macros (RFIFO/WFIFO) are at `socket.hpp:31-65`.

| Buffer | Size and rule |
|---|---|
| Read buffer (RFIFO) | Fixed 2 KB (`src/common/socket.cpp:255-257`) |
| Write buffer (WFIFO) | 16 KB to start; grows in 16 KB steps; halves when ≤¼ used (`socket.cpp:259`, `:773-797`) |
| Client write cap (`WFIFO_MAX`) | 1 MB, then the client is disconnected (`socket.cpp:261-263`, `:863-867`) |
| Server links | 256 KB (`socket.hpp:28`); forced flush at 2× (`socket.cpp:874-876`) |

**Write-path checks** (`WFIFOSET`):
- A buffer overrun or a packet over 64 KB calls `exit()` (`socket.cpp:831-846`).
- A client packet larger than the client can handle is dropped (`socket.cpp:242-246`, `:858-861`).
- A partial send `memmove`s the rest; a hard send error clears the queue and sets eof (`socket.cpp:408-454`).

**Event loop** (`do_sockets`, `socket.cpp:892-1037`):
1. Flush the "send shortlist" (only sessions with pending output or eof; `socket.cpp:1689-1761`).
2. `select` or `epoll_wait` with timeout = time until the next timer.
3. Receive into the read buffers.
4. Flush again.
5. Call every session's parse function. Two checks run here:
   - **Stall timeout:** a client with no data received for `stall_time` gets eof; a server link gets a ping flag instead (`socket.cpp:1016-1024`).
   - **Oversized packet:** a read buffer stuck full at 2 KB means a packet too large to ever parse, so it is closed (`socket.cpp:1032-1035`).

Notes:
- `select` is the default; epoll is opt-in (`CMakeLists.txt:328-331`; `socket.cpp:934-946`; `conf/packet_athena.conf:19`).
- Sockets use `TCP_NODELAY` and no linger (`socket.cpp:314-354`).

**Slow clients**
- Data is queued until 1 MB, then the client is disconnected.
- The stall timer only looks at the *receive* side (it resets on any received data, `socket.cpp:396`). A client that sends keepalives but reads slowly survives until it hits the 1 MB cap.

**Flood and DDoS protection**
- Per-IP connection history in a 64K-bucket hash (`socket.cpp:1094`). `ddos_count` connections within `ddos_interval` flags the IP until `ddos_autoreset`; "allow" rules override the flag (`socket.cpp:1176-1209`). Old entries are swept every 5 minutes (`socket.cpp:1214-1245`, `:1618-1621`).
- Config: `conf/packet_athena.conf:22,28,36,58,62,67` (stall 60 s; 3000 ms / 5 attempts / 600 s). The code's default count is 10 (`socket.cpp:1089-1091`).
- Login adds: IP bans plus automatic bans after repeated failed passwords (`src/login/loginclif.cpp:222-223`, `:520-531`; defaults at `src/login/login.cpp:770-774`); a registration flood limit (`login.cpp:233`, `:777-778`); an optional client-executable hash check (`login.cpp:377-406`).

**Packet versioning**
- `PACKETVER` is a compile-time client date (`src/config/packets.hpp:12-16`).
- Packet structs use `#if PACKETVER` blocks plus `DEFINE_PACKET_HEADER` (`src/common/packets.hpp:19`, `:31-117`, `:187-233`; map-side `src/map/packets.hpp`, `packets_struct.hpp`).
- The map-server's packet table is built at startup from `clif_packetdb.hpp` and `clif_shuffle.hpp`. Each entry records ID, length (-1 = variable), handler, and field offsets (`clif.cpp:25808-25853`; macros at `src/map/clif_packetdb.hpp:7-8`). The shuffle file re-maps packet IDs per client date (`src/map/clif_shuffle.hpp:7-30`).

**Packet obfuscation**
- On by default from client 2011-08-17 (`src/config/packets.hpp:44-61`); three keys per client date (`src/map/clif_obfuscation.hpp:7-30`).
- Only the 16-bit packet ID is XORed with a per-session generator state that advances on every packet (`clif.cpp:10724-10726`, `:25713-25725`, `:25782-25786`). It is not encryption.

**`clif_parse` (map-server)**
- Handles at most 3 packets per session per loop (`clif.cpp:25679`).
- On eof: autotraders are detached from the socket; active players get quit-save; half-logged-in players get `map_quit` (`clif.cpp:25684-25706`).
- Unknown packet ID: disconnect (`:25728-25755`). Variable length outside 4..32768: disconnect (`:25759-25772`).
- **A packet not yet fully received is also treated as an error and disconnects** (`:25773-25780`).
- State gating: with no session only `WantToConnection` is accepted; while not on a map only `LoadEndAck` (`:25788-25798`).

Login and char use a typed `PacketDatabase<Session>` that validates fixed/variable length before calling the handler; it too disconnects on a short read (`src/common/packets.hpp:665-752`; used at `src/login/loginclif.cpp:483-498`).

**Sending to groups** (`clif_send`): SELF / AREA (with "without self" and "without chat" variants) / PARTY / GUILD / BG / CHAT / ALL (`clif.cpp:477-700`). AREA scans ±`AREA_SIZE` cells for players (`:524-525`).

## 3. Main loop and timers

- **Loop:** `while RUNNING: next = do_timer(now); handle_main(next)`, where `handle_main` calls `do_sockets(next)` (`src/common/core.cpp:388-391`, `:429-434`).
- **Timer heap:** a binary min-heap of timer IDs over a reusable timer array with a free list (`src/common/timer.cpp:38-47`, `:209-249`).
- **API:**
  - `add_timer` (one-shot) and `add_timer_interval` (repeating) (`timer.cpp:253-291`).
  - `delete_timer` is lazy: it nulls the function and the entry is dropped when it pops (`:302-319`).
  - `settick_timer` does a linear search of the heap (`:330-353`).
- **`do_timer`** runs everything due (`:357-412`):
  - A timer more than 1 s late is called with the *current* tick, and repeating timers re-arm from now, so there is no catch-up storm (`:376-380`, `:401-404`).
  - It returns the time to the next timer, clamped to 20–1000 ms (`:20-25`, `:411`).
- **Tick source:** `GetTickCount64` on Windows, `CLOCK_MONOTONIC` elsewhere (`:147-167`).
- **Timer names:** functions are registered with names for diagnostics (`:57-95`).
- **Callbacks receive `(tid, tick, id, data)`.** Handlers look the entity up by ID and check they still own the timer, e.g. `ud->walktimer != tid` (`src/map/unit.cpp:554-557`), `sce->timer != tid` (`src/map/status.cpp:14155`), `md->spawn_timer != tid` (`src/map/mob.cpp:1050-1054`).
- **Single-threaded:** the only threads in `src/` are the web-server's HTTP thread and its SQL mutex (`src/web/web.cpp:92`, `:503`; `src/web/sqllock.cpp:10`). The memory pools are documented "not thread-safe" (`src/common/ers.hpp:30`), and pathfinding uses a global heap and "can't be called in parallel" (`src/map/path.cpp:267`).
- **Scaling is by process:** maps split across map-servers (≤2 per char-server, `char.hpp:212`), and ≤5 char-servers per login (`login.hpp:62`).

## 4. World simulation

**Map data**
- The map cache is a file header `{file_size, map_count}` followed by, per map, `{name, xs, ys, len}` and zlib-compressed cell bytes (`src/map/map.cpp:155-167`).
- Cache files are read in override order: `db/import` → `db/re` → `db/map_cache.dat` (`map.cpp:3920-3944`).
- Each map is decoded into `mapcell` bitfields (`map.cpp:3672-3717`), checked against `MAX_MAP_SIZE` 512×512 (`map.hpp:82`). Unknown or duplicate maps are removed (`map.cpp:3975-3994`).
- Cell types map to walkable/shootable/water: wall = 1, water = 3, cliff (shootable but not walkable) = 5 (`map.cpp:3280-3310`).
- Cell flags (`map.hpp:788-810`) and check kinds `cell_chk` (`map.hpp:764-786`: wall, water, cliff, pass, reach, no-pass, stack, npc, basilica, land-protector, no-vending, no-chat, maelstrom, icewall, no-buying-store).
- Bounds rule: the last row and column count as "no pass" (`map.cpp:3329-3331`).

**Block grid ("who can see whom")**
- `BLOCK_SIZE 8` (`map.cpp:120`). Each map has two arrays of linked lists: `block` and a separate `block_mob` (`map.hpp:823-827`; allocated at `map.cpp:4002-4007`).
- Add/remove: `map_addblock` and `map_delblock` (`map.cpp:407-506`). `map_moveblock` relinks only when the 8×8 block changes, and updates the per-cell object count used by cell-stacking rules (`map.cpp:518-568`).
- Iteration is **collect, then apply.** Matches are first copied into a global `bl_list` (capacity `BL_LIST_MAX` 1,048,576, `map.cpp:125-127`). The callbacks then run under a free-lock, and any entry unlinked meanwhile is skipped (`map.cpp:719-791`). The lock defers frees until it is released (`map.cpp:293-366`).
- **Movement visibility uses only the strip that entered or left view:** `map_foreachinmovearea` (`map.cpp:1077-1125`).
- On each step: "lost sight" for the outgoing strip, move, "came into view" for the incoming strip (`unit.cpp:621-631`). The view callbacks send spawn or clear-unit packets (`clif.cpp:5604-5690`). Moving entities are announced with `clif_move` (`clif.cpp:2066-2110`).
- `AREA_SIZE` comes from `battle_config.area_size` (`map.hpp:66`), default 14 (`conf/battle/client.conf:39`).

**Movement (server decides)**
- The client sends only a destination cell. The handler checks death, state and cell-stacking, then calls `unit_walktoxy` (`clif.cpp:11379-11425`).
- The server searches a path and rejects one longer than `max_walk_path` (17, `conf/battle/client.conf:42`). Under `OFFICIAL_WALKPATH` it also rejects paths over 14 cells with an obstacle in between (`unit.cpp:841-870`).
- Pathfinding is A* with Manhattan heuristic, straight cost 10 and diagonal 14, and no corner cutting (`src/map/path.cpp:337-399`; `path.hpp:11-14`, `MAX_WALKPATH 32`).
- Each cell step is its own timer with duration = speed, ×14/10 on diagonals (`unit.cpp:180-247`).
- The step handler re-checks the next cell, which also handles a path blocked mid-walk (`unit.cpp:542-632`).
- A new destination while walking is applied at the next cell center (`unit.cpp:894-899`).
- Knockback uses `path_blownpos` and only checks cells, not paths (`unit.cpp:1300-1349`).

**Monster AI**
- **"Hard" AI:** every 100 ms, for each player, every mob within `AREA_SIZE+ACTIVE_AI_RANGE` is processed (`mob.cpp:2348-2352`, `:2468-2476`).
- **"Lazy" AI:** every 1 s for all mobs (`mob.cpp:2359-2463`). It runs hard AI only for mobs recently near a player (`mob_active_time`), otherwise does rare idle walks and idle skills.
- Timing constants: think 100 ms, random walk 4 s, skill check 1 s (`src/map/mob.hpp:35-42`). Both timers are started at `mob.cpp:7363-7364`.
- A `next_thinktime` guard stops the two paths from running on the same tick (`mob.cpp:1854-1858`).
- **Respawn:** base delay plus a random range, scaled per mob class, minimum 1 s (`mob.cpp:1064-1098`).
- **Dynamic mobs:** spawned when the first player arrives on a map (`clif.cpp:10805-10806`; `map.cpp:3004-3026`); removed after a delay once it empties, sparing damaged mobs and MVPs (`map.cpp:3028-3096`; `unit.cpp:3717`).
- Mob data comes from YAML (`MOB_DB` v5, `src/map/mob.hpp:292`).

**Skills**
- Cast time goes through three adjustments, then a cast-end timer is armed (`unit.cpp:2469-2471`, `:2563`, `:2702-2761`).
- After-cast delay and the "can act again" tick: `unit.cpp:1972`, `:1986`; `skill.cpp:10385`.
- Per-skill cooldowns: `skill_blockpc_start` (`skill.cpp:13915`), saved through char (packet 0x2b15).
- Ground skills: a skill-unit group (`skill.cpp:5748`) stored in `skillunit_db`, all of them ticked every 100 ms (`skill.cpp:55`, `:60`, `:12485-12491`, `:16329`).
- Delayed multi-hit "timer skills" use pooled allocation (`skill.cpp:57`, `:4029-4030`).

**Status effects**
- Each effect is `{timer, val1..val4}`, stored per object in an `unordered_map` (`src/map/status.hpp:3470-3516`).
- Entry point `status_change_start` (`status.cpp:10152`). Each effect has its own timer with an ownership check (`status.cpp:14131-14175`).
- Stat recalculation uses dirty bits that propagate along dependencies, e.g. STR → base attack (`status.cpp:5837-5918`).
- Natural regeneration ticks every 500 ms (`status.cpp:16500`; `map.hpp:75`).
- On quit, effects flagged no-save are dropped and the rest are written to `sc_data` (`map.cpp:2269-2292`; `chrif.cpp:284-287`).

**Battle**
- Dispatcher `battle_calc_attack` (`src/map/battle.cpp:6753`) splits into weapon / magic / misc (`:5387`, `:5801`, `:6290`).
- Critical (`:3017`) and hit/flee (`:3228`) checks.
- Element damage table: `battle_attr_fix` reads `elemental_attribute_db` (`battle.cpp:511-526`), loaded from `db/re/attr_fix.yml` (`Header: Type ATTRIBUTE_DB`; parser declared at `status.hpp:163`).
- Delayed damage uses pooled allocation (`battle.cpp:9219`).

**Drops and EXP**
- Each mob keeps a damage log of up to `DAMAGELOG_SIZE` 20 (`map.hpp:67-69`; `mob.cpp:2672-2760`).
- `mob_dead` splits EXP among eligible damagers and parties (`mob.cpp:2934-3230`; `party.cpp:1238`). The MVP is the top damager (`mob.cpp:2898-2927`).
- Dropped items are reserved for the killer for a time (`map.cpp:2048-2058`).

**Instances**
- `instance_create` (`src/map/instance.cpp:608`).
- `map_addinstancemap` clones the source map's cells and allocates empty block grids in a free map slot (`map.cpp:2819-2893`).
- Idle and keep-alive timers end the instance (`instance.cpp:405-470`).
- Instance maps are never registered with char (`chrif.cpp:370-375`).

**Social and economy systems**

| System | Where it lives and how it syncs |
|---|---|
| Party / guild | Authoritative copy in char (`src/char/int_party.cpp`, `int_guild.cpp`). Guild saves are spread over time (`int_guild.cpp:55-92`); map-servers hold caches via intif (`intif.cpp:630-776`). |
| Storage | Loaded on demand, dirty-flagged (`src/map/storage.cpp:271,291,311`), saved through intif (`storage.cpp:461-465`); only one storage open at a time (`:137-145`). |
| Trade | `trade.cpp:586-686` (see section 6). |
| Vending | `vending.cpp:123-200`, persisted in `vendings` (`:396`). |
| RODEX mail | Map side `mail.cpp:462`; char side `int_mail.cpp:82-159`. |
| Chat rooms | Map-local only (`chat.cpp:77-191`). |

## 5. NPC script engine

**Compilation**
- Scripts are compiled once into bytecode: a typed opcode stream of ints, strings, names, args, functions, end-of-line markers and operators (`src/map/script.hpp:230-265`).
- A hashed symbol table maps names to IDs (`script.cpp:686-760`), with forward labels patched once known (`:804-860`). Entry point `parse_script_` (`:2475`).
- **Duplicate NPCs share the compiled script and label list** (`npc.cpp:4602-4604`; reference count at `script.cpp:3646`).

**Execution**
- A script state holds a stack, position, state, attached player (`rid`), NPC (`oid`), sleep data and a backup of the player's previous script (`script.hpp:314-336`). States: `RUN, STOP, END, RERUNLINE, GOTO, RETFUNC, CLOSE`.
- `run_script_main` is a stack VM loop (`script.cpp:4376-4477`) with budgets of `check_cmdcount` 65535 instructions and `check_gotocount` 2048 jumps (`:259`, `:4420-4424`, `:4472-4476`).
- After the loop it either:
  - arms a sleep timer (`:4479-4486`);
  - leaves the state attached to the player to resume later (`:4487-4499`); or
  - ends: frees the state, restores the player's previous script and saves changed variables (`:4500-4523`).

**Dialogs pause the script**

| Command | What happens |
|---|---|
| `mes` | Sends text and marks a dialog open (`script.cpp:4923-4946`). |
| `next` | Sends the "next" button and sets `STOP` (`:4952-4969`). |
| `close` | Sets `CLOSE` (`:4994-5025`). `close2` sets `STOP` (`:5031`). |
| `select` / `input` | Use **RERUNLINE**. The first run sends the menu or input box, sets a "waiting for menu/input" flag and `RERUNLINE`. When the client answers, the *same command* runs again, reads the stored answer and continues (`:5225-5292`; `input` at `:6165-6189`). |

**Resuming**
- The client's reply packets (`NpcSelectMenu` / `NpcNextClicked` / `NpcAmountInput` / `NpcStringInput` / `NpcCloseClicked`, `clif.cpp:13319-13410`) store the answer and call `npc_scriptcont`.
- `npc_scriptcont` checks the NPC ID matches the player's current NPC, that the player is near it, and that no progress bar is running. It then maps `STOP` → `RUN`, keeps `RERUNLINE`, maps `CLOSE` → `END`, and calls `run_script_main` (`npc.cpp:2292-2378`).
- **An invalid menu index kicks the player** (`clif.cpp:13329-13334`).

**Timeouts**
- `SECURE_NPCTIMEOUT` is on by default: 180 s for input, 60 s for menus, 60 s for "next" (`src/config/secure.hpp:22-48`).
- A 1 s watchdog per player closes the NPC when idle too long (`npc.cpp:1135-1170`). It is armed when a script attaches (`script.cpp:4365-4369`).
- A sleeping script that resumes after its player changed or logged out ends (`script.cpp:4229-4243`).

**Disconnect mid-dialog**
- `map_quit` calls `npc_event_dequeue`, which frees the script state by default (`map.cpp:2250-2251`; `npc.hpp:1617`; `npc.cpp:1176-1205`). It then fires `OnPCLogoutEvent` (`map.cpp:2264`).
- `unit_free` also frees any waiting script (`unit.cpp:3988-3990`). `pc_close_npc` is at `pc.cpp:9736-9761`.

**Variable scopes** (by name prefix, `script.cpp:3248-3311`)

| Prefix | Scope | Stored |
|---|---|---|
| `@` | Temporary, per character | Memory only |
| `$` | Global | `mapreg` table |
| `$@` | Temporary global | Memory only |
| `#` | Per account | `acc_reg_*` |
| `##` | Global per account | Login server |
| `.` | Per NPC | Memory |
| `.@` | Per script call | Memory |
| `'` | Per instance | Memory |
| none | Permanent, per character | `char_reg_*` |

- Character and account variables are dirty-flagged (`pc.cpp:11547`, `:11614`) and only changed ones are sent (`intif.cpp:396-435`).
- Char writes each with `REPLACE`, and **0 or an empty string deletes the row** (`src/char/inter.cpp:601-653`).
- `##` variables are forwarded to login (`inter.cpp:610-615`).
- Mapreg: a new key or a delete is written immediately; updates set a dirty flag and are batched every 5 minutes (`src/map/mapreg.cpp:25-28`, `:62-112`, `:230-264`, `:341`).

**Events**
- Labels starting with `On` are exported as `NPC::Label` (`npc.cpp:1211-1246`). Built-in event names (`OnInit`, `OnPCLoginEvent`, `OnTouch`, `OnTimer`, `OnClock`, ...) are listed at `script.cpp:257-300`.
- Collected per event type by `npc_read_event_script` (`npc.cpp:6004-6041`).
- `OnTimer` (`npc.cpp:1470`, `:1542`); `OnInit` (`:1420`); `OnClock/OnMinute` checked every 1 s (`:1368`, `:1424`).
- `OnInterIfInit`, and `OnInterIfInitOnce` on the first char connection (`chrif.cpp:494-499`).
- A busy player's events wait in a queue of up to 2 (`map.hpp:73`; `npc.cpp:1193-1204`).

**Warps, shops, touch areas**
- Warp lines: `npc_parse_warp` (`npc.cpp:3894-3983`). Shops: `npc_parse_shop` (`:4002`).
- Touch areas mark map cells with `CELL_NPC` (`npc.cpp:4971-4998`). After each step the walker checks that flag before scanning the map's touch NPCs (`unit.cpp:683-698`; `npc.cpp:1958-1993`).

## 6. Persistence

**Schema**
- `sql-files/main.sql` holds `char` (:209), `inventory` (:716, with `unique_id bigint`), `cart_inventory` (:169), `storage` (:1090), `skill` (:1045), `char_reg_num/str` (:302/:315), `acc_reg_*` (:5/:18), `global_acc_reg_*` (:434/:447), `mapreg` (:865), `sc_data` (:946).
- **Tables are `ENGINE=MyISAM`**, e.g. `main.sql:309`, `:757`.

**When characters are saved**
- **Autosave saves one player per tick**, with interval = `autosave_interval/(players+1)`, minimum `minsave_interval` (`pc.cpp:13002-13041`; defaults 300 s / 100 ms at `conf/map_athena.conf:91,96`).
- Event-triggered saves via the `save_settings` bitmask: trade, vending, storage, pet, mail, auction, quest, bank, attendance (`map.hpp:1121-1133`; e.g. `trade.cpp:682-685`).
- Map change and quit use `CSAVE_*` flags (`chrif.hpp:20-26`).

**How char writes**
- `char_mmo_char_tosql` compares the incoming struct with the cached copy and writes only changed sections (status, memo, skills, hotkeys...). It updates the cache only if no SQL error occurred (`char.cpp:257-530`).
- Item lists (`char_memitemdata_to_sql`) are diffed: the current rows are read; matches are left alone or updated; vanished rows are deleted; new ones get one multi-row `INSERT` (`char.cpp:533-749`).

**Item identity and duplication prevention**
- Unique item ID = `(char_id << 32) | uniqueitem_counter++` (`pc.cpp:15334-15336`). The counter is saved in the `char` row (`char.cpp:297`). IDs are assigned on add for flagged items (`pc.cpp:6015-6016`) and to non-stackables missing one on load (`pc.cpp:12693-12730`).
- Trade:
  - both sides must lock, then both confirm;
  - on commit each offer is re-checked against an inventory snapshot, and over-offering triggers a GM alert and an optional ban;
  - both players are then saved (`trade.cpp:181-239`, `:586-686`).
- Vending purchases are rejected if the shop changed since the buyer looked (`vender_id != uid`, `vending.cpp:134`) and are checked for zeny overflow (:173-180).
- Re-login before the final save is acknowledged is refused (`clif.cpp:10713-10719`). The logout state is held until ack 0x2b21 (`chrif.cpp:195-205`, `:418-420`). Stuck logout nodes retry the save after 60 s (`chrif.cpp:743-764`).
- If char has the character marked on another map-server, that copy is kicked (`char.cpp:130-134`; `char_mapif.cpp:381-386`).

**Transactions:** used only for account creation and saves (`src/login/account.cpp:350-358`, `:575-720`), char deletion (`char_clif.cpp:113-116`) and mail save (`int_mail.cpp:89-156`). **Character saves are not transactional.**

**Logging**
- Item ("pick") log (`src/map/log.cpp:207-258`), zeny (:277), MVP (:308), GM commands (:341) and more.
- Log types form a bitmask (`log.hpp:30-64`).
- Logs use a separate SQL handle (`log.cpp:221`) and `INSERT DELAYED` unless the build sets `SQL_INNODB` (`log.cpp:49-55`). Writes are synchronous.

**Mapreg:** see section 5.

## 7. Content pipeline

**YAML database classes**
- `YamlDatabase` gives typed node readers and an abstract parse step per entry (`src/common/database.hpp:19-82`).
- `TypesafeYamlDatabase` is a key → shared pointer map (`:84-148`).
- `TypesafeCachedYamlDatabase` adds a flat array lookup sized **twice the largest key** (`:150-251`, `:194-220`).

**Loading**
- Each file must have a header `{Type, Version}`: newer than supported is rejected; between minimum and current is accepted with a warning (`src/common/database.cpp:37-76`).
- An optional `Clear: true` empties the database before this file loads (`:132-138`).
- The body is parsed, then each `Footer.Imports` path is loaded (optionally only in Renewal or pre-Renewal mode) (`:152-200`). Example: `db/item_db.yml:91-97` loads `db/re/...` then `db/import/item_db.yml`.
- Registrations: `ITEM_DB` v3 (`src/map/itemdb.hpp:3571`), `MOB_DB` v5 (`mob.hpp:292`), `SKILL_DB` v4 (`skill.hpp:335`), `STATUS_DB` v4 (`status.hpp:3299`), `PLAYER_GROUP_DB` (`pc_groups.hpp:117`).

**Reload**
- `reload()` is clear-then-load (`database.cpp:86-90`).
- Commands: `@reloaditemdb`, mobdb, skilldb, script, battleconf, statusdb... (`atcommand.cpp:4359-4424`; table at `:4584-4598`).
- `itemdb_reload` re-points every online player's cached item pointers, re-validates items, reloads combos and recalculates stats (`itemdb.cpp:4978-5000`).
- `@reloadscript` closes every player's NPC dialog, empties battleground queues, flushes, then reloads NPC files (`atcommand.cpp:4399-4419`); `OnInit` runs again (`npc.cpp:6138`).

**Config overrides**
- `import:` chains in each config file (e.g. `battle.cpp:9179-9180`; `conf/packet_athena.conf:70`; `conf/groups.yml:246-248`).
- Battle settings are a table of `{name, pointer, default, min, max}` (`battle.cpp:8246-8252`). Features the compiled client version cannot support are switched off (`battle.cpp:9080-9153`).

## 8. Admin and operations

**GM groups**
- `conf/groups.yml` defines Id, Name, Level, LogCommands, Commands, CharCommands, Permissions and Inherit (`conf/groups.yml:19-35`; admin group around `:230-244`).
- Inheritance is resolved at load (`src/map/pc_groups.cpp:180-205`).

**Commands**
- Dispatch resolves aliases, then checks permission (`atcommand.cpp:1811-1824`; alias DB `:96-196`).
- The command table is built from `ACMD_DEF` entries (`:11491-11526`).
- `@kick` compares GM levels (`:3749-3776`); `@kickall` (`:3781-3800`); `@ban` (`:3450`); `@jail` (`:5554`); `@broadcast` (`:5953`).
- Commands are logged when the group has `LogCommands` (`log.cpp:341`).

**Ban and kick across servers**
1. Map sends 0x2b0e (`chrif.cpp:846`).
2. Char → login 0x2725. Login updates `unban_time`, saves, and broadcasts 0x2731 (`loginchrif.cpp:356-395`).
3. Char forwards 0x2b14 to every map-server and disconnects the account at char too (`char_logif.cpp:565-580`).
4. Map kicks the player (`chrif.cpp:1086`).

Login also kicks duplicate logins with 0x2734 (`loginclif.cpp:90-96`).

**Broadcast:** `intif_broadcast` 0x3000 (`intif.cpp:207-224`) → char sends to every map-server (`inter.cpp:1052`, `:1131-1135`). Hack reports reach online GMs through the "whisper to GMs" path (`inter.cpp:1260-1269`; e.g. `trade.cpp:213-232`).

**Status and console**
- Char reports user counts to login every 5 s (`char.cpp:3240`); login shows them as low/medium/high bands (`login.cpp:484-495`).
- Optional bandwidth and RAM stats in the console title (`socket.cpp:1039-1054`).
- Console command interface (`char.cpp:2128-2132`).

**Crash and shutdown**
- SIGINT/SIGTERM trigger a graceful shutdown; a 4th signal exits immediately (`core.cpp:114-125`).
- On map shutdown everyone is kicked (and so saved) and buffers are flushed (`map.cpp:5316-5325`).
- On a crash signal the map-server tries to save every online character (`map.cpp:5122-5139`).

## 9. Scale and robustness limits

**Hard limits**

| Limit | Value | Where |
|---|---|---|
| Maps per map-server | 1500 | `src/common/mmo.hpp:42` |
| Map index entries | 2000 | `src/common/mapindex.hpp:10` |
| Map-servers per char-server | 2 | `src/char/char.hpp:212` |
| Char-servers per login | 5 | `src/login/login.hpp:62` |
| Connections | `FD_SETSIZE` (4096 on Windows) | `socket.hpp:24-26` |
| NPCs per map | 512 | `map.hpp:65` |
| Spawn entries per map | 128 | `map.hpp:72` |
| Walk path length | 32 | `path.hpp:14` |
| Objects collected per area scan | 1,048,576 | `map.cpp:125` |
| Party / guild size | 12 / 76 | `mmo.hpp:99-100` |
| Storage / cart | 600 / 100 | `mmo.hpp:91`, `:97` |
| Name length | 24 | `mmo.hpp:154` |
| Account / char ID ranges | start 2,000,000 / 150,000 | `mmo.hpp:176-178` |
| `mmo_charstatus` struct | must stay < 64 KB to fit one packet | `chrif.cpp:1941-1945` |

**Memory pools ("ERS")**
- Fixed-size free-list pools for frequently created objects; blocks of 2048 entries (`src/common/ers.hpp:1-40`; `ers.cpp:55`).
- Used for auth nodes, timer skills, delayed damage, map variables (`chrif.cpp:1959`; `skill.cpp:57`; `battle.cpp:9219`; `mapreg.cpp:80`).

**Legacy DB maps ("DBMap")**
- A hash table of 283 buckets, each a red-black tree, with deferred frees while iterating (`src/common/db.cpp:87-112`, `:136-199`, `:719`).
- Newer code uses `std::unordered_map` (e.g. `char.cpp:79-81`).
- SQL connections ping on a timer derived from the server's `wait_timeout` (`src/common/sql.cpp:196-215`).

**Threading:** none in the game path (section 3).

**Bottlenecks**
- Every SQL query blocks the loop: char saves, logs, and script `query_sql` (the user's own bridge notes this at `game/npc/relic/bridge.txt:9`).
- Monster AI scans per player, overlapping when players crowd (`mob.cpp:2348-2352`).
- `settick_timer` is O(n) (`timer.cpp:335`).
- `select` by default.
- AREA broadcasts copy the packet once per recipient (`clif.cpp:456-468`).

## 10. How the user's project wraps rAthena (paths relative to `C:\Users\Relic\Desktop\ro server\`)

- **No edits to rAthena.** `prepare_rathena.py` copies the pinned sources into `build/rathena-src`, lays `game/overlay/src/custom/*` on top (`defines_pre.hpp`, `script.inc`, `relic_*`), and applies anchored Python patches that stop the build if an anchor drifts (`CLAUDE.md:18-23`; `game/patches/0001-max-level-guard.py` with an `anchor`/`count` contract).
- **Immutable releases.** Each `runtime/releases/<time>-<rathena7>-<content8>/` is read-only; `current`/`previous` links switch between them; the `conf/import` and `db/import` folders are links into `runtime/state` (`CLAUDE.md:24-28`).
- **Generated, never hand-edited** config and data import folders, and client-gen output (`CLAUDE.md:29-33`).
- **Game-op bridge.** The web only queues rows in `rr_game_op`. A map-server NPC claims them every second (`OnTimer1000`), applies them with rAthena's own commands, and acks, with backoff on DB errors (`CLAUDE.md:38-42`; `docs/design/01-data-and-bridge.md:153-200`; `game/npc/relic/bridge.txt:3-31,121-135`).
- **Staged Apply.** A resumable state machine: planned → generated → validated → files_written → reload_queued → reloaded → exported → verified, or mismatch → rolled_back. It generates a complete new import generation, swaps the link, reloads in a fixed order, exports what the server loaded, compares field by field, and reverts by itself on a mismatch (`CLAUDE.md:43-46`; `docs/design/01-data-and-bridge.md:211-260`).
- **Wiki parity.** Python reads the same files with the same merge rules, and parity runs compare against the live server through the bridge (`CLAUDE.md:47-50`).
- **Client-gen and signed launcher** (`CLAUDE.md:51-54`). A headless test client drives login → char → map end to end (`CLAUDE.md:89-91`).
- **Lessons recorded from rAthena behavior:**
  - Cached YAML databases size their array to twice the largest ID; huge sentinel IDs made the map-server try to allocate ~62 GB and get OOM-killed (`CLAUDE.md:131-135`; `src/common/database.hpp:194-220`).
  - Kill versus graceful stop and the 4th-signal exit (`CLAUDE.md:136-139`; `src/common/core.cpp:118-125`).

---

## Techniques worth re-implementing

| # | Technique | Why it matters | rAthena reference |
|---|---|---|---|
| 1 | One loop: due timers first, then socket polling with timeout = next timer due | Deterministic, no locks, low idle CPU | `core.cpp:388-391`; `timer.cpp:357-412`; `socket.cpp:892-1037` |
| 2 | Timers carry entity ID plus data and check they still own the entity on fire; deletion is lazy | Use-after-free and stale-callback safety without reference counting | `unit.cpp:544-557`; `status.cpp:14155`; `timer.cpp:302-319` |
| 3 | Late timers use the current tick; repeating timers re-arm from now | No catch-up storms after a lag spike | `timer.cpp:376-380`, `:401-404` |
| 4 | Per-session buffers: bounded write queue, stall timeout, flush list of only "dirty" sessions | A slow or dead client cannot exhaust memory or CPU | `socket.cpp:257-263`, `:822-890`, `:1016-1024`, `:1689-1761` |
| 5 | Per-IP connection-rate limit plus allow/deny rules | Cheap pre-auth DDoS defense | `socket.cpp:1112-1210`; `conf/packet_athena.conf:21-67` |
| 6 | Table-driven packet dispatch with length checks and session-state gating | Malformed or out-of-phase packets never reach game logic | `common/packets.hpp:665-752`; `clif.cpp:25728-25798` |
| 7 | One-time handoff tokens between login → char → map, expiring in 30 s | No password reuse downstream; replay-resistant handoff | `login.cpp:136-150`; `loginchrif.cpp:84-108`; `char_mapif.cpp:1024-1077` |
| 8 | Online registry with conflict detection that kicks the older session | Stops double-login duplication | `char.cpp:121-160`; `loginclif.cpp:84-108` |
| 9 | Explicit login / logout / map-change state per player; final-save ack before freeing; replay on reconnect | Correct across server restarts and link drops | `chrif.cpp:136-205`, `:418-420`, `:507-565`, `:743-764` |
| 10 | Load in stages; the "enter world" packet is ignored until all parts arrive | No half-loaded players in the world | `pc.cpp:2304`, `:2434-2436`, `:14800-14804`; `clif.cpp:10756` |
| 11 | Auto-reconnect plus full resync of the online list in both directions | Self-healing cluster | `chrif.cpp:1498-1508`, `:1822-1879`; `char_mapif.cpp:363-399` |
| 12 | Save only what changed (cached last-saved copy; per-row item diff; dirty variables; zero deletes the row) | Database load drops by orders of magnitude | `char.cpp:257-749`; `intif.cpp:432-435`; `inter.cpp:601-653`; `mapreg.cpp:62-112` |
| 13 | Autosave one player per tick, plus saves triggered by risky events | Smooth database load; small loss window on crash | `pc.cpp:13002-13041`; `map.hpp:1121-1133`; `trade.cpp:682-685` |
| 14 | Unique item instance IDs `(owner_id << 32) | counter` | Duplicated items become traceable and auditable | `pc.cpp:15334-15336`; `log.cpp:225-234` |
| 15 | Two-phase trade (lock, confirm, re-validate against a snapshot, apply, save both); shop-version check on purchase | Classic duplication and race defenses | `trade.cpp:181-239`, `:586-686`; `vending.cpp:134` |
| 16 | 8x8 block grid, separate monster lists, collect-then-apply with deferred free | Fast area queries; safe when callbacks remove objects | `map.cpp:120`, `:293-366`, `:407-506`, `:719-791` |
| 17 | On movement, scan only the strip that entered or left view | Visibility updates scale with view edge, not view area | `map.cpp:1077-1125`; `unit.cpp:621-631` |
| 18 | Server-decided movement: client sends a destination; server runs A* with no corner cutting and steps cell by cell with speed-based timers; path length caps | Kills speed and teleport hacks; one source of truth | `clif.cpp:11379-11425`; `unit.cpp:180-247`, `:542-632`, `:841-916`; `path.cpp:337-399` |
| 19 | Cell bitflags including dynamic flags such as "NPC touch area here" | Touch triggers cost O(1) per step | `map.hpp:788-810`; `npc.cpp:4971-4998`; `unit.cpp:683-698` |
| 20 | Two-speed AI (100 ms near players, 1 s elsewhere); spawn monsters on first visitor, despawn after the map empties | CPU follows where players are | `mob.cpp:2348-2476`, `:7363-7364`; `map.cpp:3004-3096` |
| 21 | Stat recalculation by dirty bits that propagate along dependencies | Recompute only affected stats | `status.cpp:5837-5918` |
| 22 | Bytecode VM: dialog commands pause and resume; "re-run this line" for menus and input; instruction and jump budgets; dialog idle timeout; per-player event queue; duplicates share code | Safe, interruptible scripts | `script.cpp:4376-4524`, `:5225-5292`; `npc.cpp:1135-1205`, `:2292-2378`, `:4602-4604` |
| 23 | Variable scope by name prefix, each mapped to a storage backend | Script authors pick persistence explicitly | `script.cpp:3248-3311` |
| 24 | Versioned YAML data with header type and version, override imports, a Clear flag, and live reload that re-points derived state | Safe content iteration and mod layering | `database.cpp:37-200`; `itemdb.cpp:4978-5000` |
| 25 | Config descriptor table `{name, pointer, default, min, max}` with import chains | Validated, overridable config | `battle.cpp:8246-8252`, `:9158-9212` |
| 26 | Permission groups with inheritance plus per-group command logging | Auditable admin tooling | `conf/groups.yml:19-35`; `pc_groups.cpp:180-205` |
| 27 | Instances made by cloning cell data into a fresh map slot, with idle and lifetime timers | Cheap dungeon copies | `map.cpp:2819-2893`; `instance.cpp:405-470` |
| 28 | Pooled allocation for hot object types | Less fragmentation over long uptimes | `ers.hpp:1-40`; `ers.cpp:55` |

## Weaknesses to avoid

1. **Synchronous SQL on the game thread** (character saves, logs, script `query_sql`), and no worker threads (`char.cpp:305-336`; `log.cpp:242`; `web.cpp:503` is the only thread). Use an async persistence queue or worker.
2. **MyISAM tables and no transactions on character saves.** Each section is a separate UPDATE; partial failure is only counted (`main.sql:309`, `:757`; `char.cpp:525-527`). Use InnoDB (or equivalent) and an atomic save per character, ideally versioned.
3. **Raw C structs copied byte-for-byte between processes**, with a size check that fails silently (`chrif.cpp:317-328`, `:660-663`; `char_mapif.cpp:415-420`), under a 64 KB ceiling (`chrif.cpp:1941-1945`). Use a versioned, explicit format.
4. **Hand-maintained packet length tables and magic offsets per link** (`chrif.cpp:42-50`; `inter.cpp:58-70`; `intif.cpp:37-49`). Generate them from one schema.
5. **Hard compile-time limits**: 2 map-servers per char-server, 5 char-servers, `FD_SETSIZE` connections, 512 NPCs per map, 32-step paths (section 9). Also `select` by default (`CMakeLists.txt:328`).
6. **Weak auth and obfuscation.** Random 32-bit tokens (`login.cpp:413-414`). Map auth checks only `login_id1`, and the IP checks are commented out (`char_mapif.cpp:1054-1055`; `loginchrif.cpp:91-92`; `char_clif.cpp:866-867`). The "obfuscation" XORs only the packet ID (`clif.cpp:25717`, `:25785`). MD5 passwords (`CLAUDE.md:115`). Use TLS or authenticated encryption and long, IP-bound tokens.
7. **Fragile packet parsing.** A partially received packet is treated as an error and disconnects (`clif.cpp:25773-25780`; `common/packets.hpp:721-742`). The 3-packets-per-loop cap is admitted in its own comment to "aid packet spammers" (`clif.cpp:25679`). Reassemble properly and rate-limit with token buckets.
8. **Fatal `exit()` on a write-buffer misuse** (`socket.cpp:831-846`). Isolate the failure to the one session instead.
9. **Global mutable scratch state**: the shared `bl_list`, path search "can't be called in parallel" (`path.cpp:267`), a known node-collision FIXME in A* (`path.cpp:327-330`). This blocks future parallelism.
10. **Non-atomic reload.** Clear-then-load can leave an empty database on a parse error (`database.cpp:86-90`); `@reloadscript` closes everyone's dialogs (`atcommand.cpp:4399-4405`). Build the new copy, validate it, then swap.
11. **Cache sized from the largest ID** (`database.hpp:194-220`). One huge ID can exhaust memory (`CLAUDE.md:131-135`). Use dense remapping or hash maps.
12. **AI cost grows with overlapping players**: per-player area scans every 100 ms, only deduplicated by a tick guard (`mob.cpp:1854-1858`, `:2348-2352`). Schedule per map region instead.
13. **Saving from inside a crash signal handler**: I/O in a signal context is unsafe (`map.cpp:5122-5139`). Prefer frequent incremental saves plus a write-ahead log.
14. **Lazy timer deletion and O(n) `settick_timer`** (`timer.cpp:302-353`). Use a heap that supports removing by index.
15. **Small apparent bugs (not verified at runtime):**
    - `chmapif_server_destroy` closes the socket only when `fd == -1`, which looks inverted (`char_mapif.cpp:1483-1488`).
    - `char_online_data_cleanup` returns at the first still-connected character instead of continuing the loop (`char.cpp:2210-2213`).

    Both are reminders to test the reconnect and cleanup paths.
