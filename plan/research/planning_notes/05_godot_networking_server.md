# Godot 4.7.2: networking, dedicated server, build stripping, libgodot, threading, persistence

_Planning-phase research report (2026-10-13), produced by a read-only research subagent and kept verbatim as evidence. Godot citations are at tag 4.7.2-stable; they are re-verified against the clone in task 2.2/2.3._

---

I verified everything below against the 4.7.2-stable source. Paths are relative to `godotengine/godot@4.7.2-stable`; the tree has 15,212 entries and was not truncated.

**Most important findings:**
- **No database:** Godot has no SQL or database driver of any kind.
- **No area of interest:** the high-level multiplayer has no spatial culling. Its cost grows with peers × synchronizers on every network frame.
- **One engine per process:** libgodot allows only one Godot instance per process.
- **Headless is capped at about 144 Hz by default.**
- **Apparent bug in `ENetMultiplayerPeer.create_server`:** its arguments are shifted by one position.

## 1. ENet (modules/enet, thirdparty/enet)

**ENetConnection** (`modules/enet/enet_connection.h`)
- **Methods:**
  - `create_host_bound(bind="*", port=0, max_peers=32, max_channels=0, in_bw=0, out_bw=0)` (h:112) and `create_host` (h:113).
  - `connect_to_host(addr, port, channels, data)` (h:115), `service(timeout)` / `check_events` / `flush` (h:116-118).
  - `bandwidth_limit`, `channel_limit`, `bandwidth_throttle`, `compress` (h:119-122), `pop_statistic` (h:123).
  - Godot additions: `dtls_server_setup`, `dtls_client_setup`, `refuse_new_connections` (h:130-133).
  - `broadcast(channel, bytes, flags)` (cpp:334-340 → `enet_host_broadcast`, cpp:43).
  - `socket_send` sends raw UDP from the host socket (cpp:342-372), which is useful for NAT punching.
  - Host statistics are reset each time they are read (cpp:226-247).
- **Limits:**
  - `max_peers` must be 1–4095 (cpp:308). This equals `ENET_PROTOCOL_MAXIMUM_PEER_ID = 0xFFF` (thirdparty/enet/enet/protocol.h:19).
  - Channels: 0–255, where 0 means 255 (cpp:309; protocol.h:18; thirdparty/enet/host.c:71-72).
  - Bandwidth is in bytes/s, and 0 means unlimited (host.c:18).
  - Default MTU is 1392 and ENet's maximum packet size is 32 MiB (enet/enet.h:222-223). Godot's `get_max_packet_size` reports 1<<24 (enet_packet_peer.cpp:85-86).
- **Compression modes:** NONE, RANGE_CODER (ENet's own), FASTLZ, ZLIB (deflate), ZSTD (h:47-53; cpp:439-452, 497-513).
  - The docs say the mode must match on server and clients (ENetConnection.xml:43).
  - They also recommend keeping the default for small packets (xml:42).
- **IPv6:** works only with the bundled ENet. "Connecting to an IPv6 server isn't supported when using vanilla ENet" (cpp:106, 363; thirdparty/README.md:261-262).
- **DTLS:**
  - The server side needs `TLSOptions.server` (cpp:277-281); the client side is at cpp:296-300.
  - It is implemented by Godot socket adapters `ENetUDP` / `ENetDTLSClient` / `ENetDTLSServer` (thirdparty/enet/enet_godot.cpp:66, 169, 280). The DTLS handshake timeout is 8000 ms (enet_godot.cpp:285).
  - Operator-precedence issue: `peers.size() < max_clients && status == HANDSHAKING || status == CONNECTED` (enet_godot.cpp:366). The client cap is not applied on the CONNECTED branch.
  - ENet time comes from `OS::get_ticks_msec` (enet_godot.cpp:478-480).

**ENetPacketPeer** (`enet_packet_peer.h`)
- **Send flags:** `FLAG_RELIABLE`, `FLAG_UNSEQUENCED`, `FLAG_UNRELIABLE_FRAGMENT` (h:60-65). `send(channel, bytes, flags)` is bound at cpp:216.
- **Timing:**
  - `ping` and `ping_interval` (cpp:56-58).
  - `throttle_configure(interval, accel, decel)` (cpp:74-76).
  - `set_timeout(limit, min, max)` requires limit ≤ min ≤ max (cpp:79-82).
- **Disconnect:** `peer_disconnect`, `peer_disconnect_later`, `peer_disconnect_now` (h:102-104).
- **Statistics:** packet loss, `ROUND_TRIP_TIME` (plus variance and last value), and throttle values (h:80-95). `get_remote_address` and `get_remote_port` are also available.
- Minor: the channel check uses `>` where `>=` would be expected (cpp:202).

**ENetMultiplayerPeer**
- **API:**
  - `create_server(port, max_clients=32, max_channels=0, in_bw, out_bw)` and `create_client(..., local_port)` (h:120-121).
  - `create_mesh` and `add_mesh_peer` (h:122-123).
  - `get_host()` and `get_peer(id)` (h:127-128) give access to the underlying ENet objects for per-peer tuning.
- **Channels and modes:**
  - Channels 0 and 1 are reserved, so user channels are offset by 2 (h:48-52; cpp:56-57, 359-360).
  - UNRELIABLE maps to UNSEQUENCED|UNRELIABLE_FRAGMENT, UNRELIABLE_ORDERED to UNRELIABLE_FRAGMENT, and RELIABLE to RELIABLE (cpp:345-357).
- **Polling:** `poll()` calls `service(0)` and then drains `check_events`. It is non-blocking and runs in the caller's frame (cpp:166-200).
- **Apparent bug:** cpp:67 calls `host->create_host_bound(bind_ip, p_port, p_max_clients, 0, p_max_channels > 0 ? p_max_channels + SYSCH_MAX : 0, p_out_bandwidth)`. The signature is `(..., max_channels, in_bandwidth, out_bandwidth)` (enet_connection.h:112). As a result:
  - `p_in_bandwidth` is ignored.
  - The channel limit is always 0, which means 255.
  - If `max_channels > 0`, the host's incoming bandwidth becomes (channels + 2) bytes/s. ENet advertises that value to peers (host.c:253) and uses it to size the send window (thirdparty/enet/protocol.c:411-414).
  - I did not verify the runtime impact. Workarounds: patch it in your fork, or call `get_host().bandwidth_limit(0,0)` and `channel_limit(n)` after creating the server (cpp:206-214).
- **Thread safety:** none of the three .cpp files in modules/enet uses a mutex or lock. Treat ENet objects as owned by a single thread.
- **Vendored ENet:**
  - Version 1.3.18, commit 2662c0de (thirdparty/README.md:243-247; enet.h:30-32).
  - Godot adds `enet_godot.cpp`, `enet_godot.h` and `enet_godot_ext.h` (README:254-255) plus `patches/0001-godot-socket.patch` (README:259).
  - ENet's per-IP limit (`duplicatePeers`, enet.h:406; host.c:103) is not exposed by ENetConnection.

## 2. High-level multiplayer (modules/multiplayer)

**SceneMultiplayer**
- **Protocol:** commands are REMOTE_CALL, SIMPLIFY_PATH, CONFIRM_PATH, RAW, SPAWN, DESPAWN, SYNC, SYS, with system messages AUTH, ADD_PEER, DEL_PEER, RELAY (scene_multiplayer.h:68-84). The docs call it an "implementation detail and isn't meant to be used by non-Godot servers. It may change without notice." (SceneMultiplayer.xml:10).
- **Authentication:**
  - `auth_callback` (h:113) and `auth_timeout`, default 3000 ms (h:114).
  - `send_auth` and `complete_auth` (h:173-179).
  - While a peer is pending, only AUTH packets are accepted and they go to `callback(peer, bytes)` (cpp:96-119).
  - Timeouts are handled at cpp:144-155. A peer is admitted once both sides complete (cpp:478-495).
  - If no callback is set, peers are auto-accepted (cpp:358-364).
- **`server_relay`:**
  - It defaults to true (h:123).
  - On every admit, the server sends ADD_PEER to every existing peer and every existing peer to the newcomer (cpp:368-386).
  - That is O(N) reliable messages per join, and every client learns every peer ID. An MMO should turn this off.
- **Object decoding and packet sizes:**
  - `allow_object_decoding` defaults to false (h:122), with a remote-code-execution warning in the docs (xml:63-65).
  - `max_sync_packet_size` is 1350 and `max_delta_packet_size` is 65535 (scene_replication_interface.h:93-94).
- **Polling:**
  - Polling happens in `SceneTree::process`, the idle frame rather than physics (scene_tree.cpp:706-711). Replication is sent at the end of `poll` (scene_multiplayer.cpp:165).
  - The multiplayer API can only be used from the main thread (scene_tree.cpp:1813).
  - Several MultiplayerAPIs can live in one tree via `set_multiplayer(api, root_path)` (scene_tree.cpp:1841). This would let one process act as a client of the world server and as a zone server at the same time.
- **RPC:**
  - Modes are DISABLED, ANY_PEER and AUTHORITY (multiplayer_api.h:48-50). AUTHORITY is the default when configured (scene_rpc_interface.cpp:92).
  - The mode is enforced on receive (scene_rpc_interface.cpp:252-263). Outgoing RPCs are filtered by replication visibility (327-336).

**Spawner, synchronizer and replication config**
- **MultiplayerSpawner:** spawnable scenes, `spawn_limit` (0 means unlimited) and a custom `spawn_function` (multiplayer_spawner.h:51, 68-69, 105-113).
- **MultiplayerSynchronizer visibility:**
  - `replication_interval` and `delta_interval` default to 0, meaning every network frame (xml:55-65).
  - `public_visibility` defaults to true (xml:58).
  - `set_visibility_for`, `add_visibility_filter(Callable)` and `update_visibility` (h:108-116).
  - Filters are ANDed and then combined with the per-peer set (cpp:194-208). They are re-run every idle or physics frame whenever any filter exists (cpp:88-100, 306-308).
- **Sync and delta behaviour:**
  - Full syncs are sent unreliable (scene_replication_interface.cpp:836, 851). Deltas are sent reliable (746, 763).
  - Deltas use a 64-bit index mask (multiplayer_synchronizer.cpp:426-433), so the practical cap is 64 watched properties. This is inferred from the mask; I found no explicit check.
  - A single state larger than the MTU is dropped: "TODO Handle single state above MTU." (scene_replication_interface.cpp:832-833).
  - Data from a non-authority sender is ignored (776-781, 876-879).
- **SceneReplicationConfig:** modes NEVER, ALWAYS, ON_CHANGE, plus a spawn flag (scene_replication_config.h:42-52).

**Interest management:** there is none beyond per-peer booleans and script filters. Spawning follows synchronizer visibility, combined with OR (scene_replication_interface.cpp:390-452). There are no spatial structures.

**Scalability:**
- Every network frame, for each peer and each visible synchronizer, the state is read and encoded twice (a size pass and a write pass) (145-152, 824-843). Nothing is shared between peers. The source admits "This is a lazy implementation, we could optimize much more here..." (809).
- Each peer has its own bookkeeping: sets of sync and spawn nodes, timestamps and receive maps (h:63-70), plus a per-peer node-path cache (scene_cache_interface.h:45-68).
- Visibility filters cost N_sync × N_peers callable calls per frame (343-365).

## 3. Other transports and crypto
- **WebSocket:**
  - `WebSocketMultiplayerPeer.create_server(port, bind_ip, TLSOptions)` runs on a TCPServer (websocket_multiplayer_peer.h:71, 115). Handshake timeout is 3000 ms (h:68); buffer sizes and queue limits are at h:117-136.
  - For a server-side `WebSocketPeer`, call `accept_stream` with a stream from `TCPServer.take_connection` or a `StreamPeerTLS` (websocket_peer.h:84; WebSocketPeer.xml:44).
  - Heartbeat is at h:119; `max_queued_packets` defaults to 4096 (h:72). Built on wslay 1.1.1+git (README:1261-1264).
- **WebRTC:** native builds need a GDExtension. Otherwise you get a stub and "No default WebRTC extension configured." (webrtc_peer_connection.cpp:48-63).
- **UDP:** `PacketPeerUDP.bind(port, addr, recv_buf=65536)`, `wait`, broadcast and multicast (packet_peer_udp.h:72-94). `UDPServer` has `listen`, `poll`, `take_connection`, with at most 16 pending connections (udp_server.h:60-74).
- **DTLS and TLS:**
  - `DTLSServer.setup` / `take_connection` (dtls_server.h:48-50); `PacketPeerDTLS.connect_to_peer` (packet_peer_dtls.h:55).
  - `StreamPeerTLS.accept_stream` / `connect_to_stream` (stream_peer_tls.h:53-54).
  - `TLSOptions.client`, `client_unsafe`, `server` (core/crypto/crypto.h:89-91). mbedTLS is 3.6.7 (README:683-686).
  - HTTPClient's TLS is limited to 1.2 and 1.3, with no certificate revocation or pinning (HTTPClient.xml:15-16).
- **Crypto:**
  - `CryptoKey` and `X509Certificate` load/save (crypto.h:37-66).
  - `Crypto`: `generate_random_bytes`, `generate_rsa`, self-signed certificates, `sign`/`verify`, RSA `encrypt`/`decrypt`, `hmac_digest`, `constant_time_compare` (crypto.h:130-143). `HMACContext` is at crypto.h:101.
  - `AESContext` supports only ECB and CBC, with no GCM or other authenticated mode (aes_context.h:42-46).
  - `HashingContext` supports only MD5, SHA1 and SHA256 (hashing_context.h:41-43).
- **Custom transports:**
  - `MultiplayerPeerExtension` exposes `_get_packet` / `_put_packet` plus script variants and optional relay hooks (scene/main/multiplayer_peer.h:99-128).
  - `MultiplayerAPIExtension` can replace SceneMultiplayer entirely (multiplayer_api.h:82-110); `set_default_interface` is at h:54.
  - The variant codec is exposed through `encode_and_compress_variant(s)` (h:57-60).

## 4. Dedicated server support
- **Export side:**
  - The preset has a `dedicated_server` flag (editor/export/editor_export_preset.h:73, 149-150). The export plugin adds the `dedicated_server` feature tag (dedicated_server_export_plugin.cpp:66-76).
  - Per-file modes are `MODE_FILE_STRIP` and `MODE_FILE_KEEP` (preset.h:52-53). Stripping only applies when the export filter is `EXPORT_CUSTOMIZED` (plugin.cpp:82-104). Modes are inherited from parent directories (33-64).
  - A stripped resource is replaced by its `create_placeholder()` (122-127).
- **What can be stripped:**
  - Implemented by Texture2D (scene/resources/texture.cpp:116), Texture3D (225), Mesh (mesh.cpp:809), Material (material.cpp:154), and Texture2DArray, Cubemap and CubemapArray (image_texture.cpp:544-568).
  - It is not in audio_stream_wav.cpp or audio_stream.cpp. I only spot-checked a few files, so other resource types are not verified.
  - The shader baker is skipped for dedicated server exports (editor_export_platform_pc.cpp:49-50).
- **Runtime:**
  - The `dedicated_server` feature tag forces the headless display and Dummy audio (main/main.cpp:2290-2293). `--headless` does the same (main.cpp:1505-1508, help at 626).
  - `DisplayServerHeadless` reports no features and `window_can_draw` returns false (display_server_headless.h:59, 139-141).
  - Dummy rendering, physics and navigation servers exist (servers/rendering/dummy/*; main.cpp:371-372, 391-392). `TextServerDummy` is always registered (main.cpp:3201-3206).
- **Frame pacing:**
  - Because headless can't draw, `OS::add_frame_delay` sleeps `low_processor_mode_sleep_usec` each frame. The default is 6900 µs, which caps the loop at about 144 Hz (core/os/os.cpp:718-740; main.cpp:2263-2265, 5178).
  - `application/run/max_fps` (main.cpp:2255) applies when it gives a lower rate (os.cpp:724-728).
  - `--fixed-fps` (main.cpp:1953-1966) returns before the frame delay (5170-5172).
- **Physics and quitting:**
  - `physics/common/physics_ticks_per_second` defaults to 60 and `max_physics_steps_per_frame` to 8 (main.cpp:2252-2253). Under overload the extra steps are dropped (4955-4959).
  - `--quit` and `--quit-after` are at main.cpp:572-573, 1787-1794, 5157-5162. `OS::delay_usec` is at os.h:277.
- **How the main loop type is resolved** (main.cpp:4360-4436):
  - The editor always uses SceneTree (4361).
  - Otherwise the `--main-loop` argument is used (4120-4121), or the `application/run/main_loop_type` setting (4364-4366; default "SceneTree" in core/config/project_settings.cpp:1707).
  - A `--script` is used next (4368-4392), then a script global class (4394-4413).
  - Finally it does `ClassDB::instantiate(main_loop_type)` and casts to MainLoop (4420-4433).
  - So yes, a MainLoop subclass registered by a module or GDExtension works.
- **Writing a custom MainLoop:**
  - The virtuals are `initialize`, `iteration_prepare`, `physics_process`, `iteration_end`, `process`, `finalize` (core/os/main_loop.h:42-69).
  - `Engine.get_main_loop` is at core/core_bind.cpp:1954-1956 and Engine.xml:78-82.
- **Caveats for a non-SceneTree loop:**
  - Autoloads are only created when the loop is a SceneTree (main.cpp:4438-4439, 4499-4549).
  - Multiplayer polling and the MessageQueue flush both live in `SceneTree::process` (scene_tree.cpp:706-715). Whether `Main::iteration` flushes the queue on its own is not verified.
  - `--script` and `--main-loop` sit behind `OVERRIDE_PATH_ENABLED` (main.cpp:691-698). Export templates turn that off by default (SConstruct:272-277, 1101-1102), so use the project setting in production.

## 5. Build stripping
- **No `platform=server`:** the platform directories are android, ios, linuxbsd, macos, visionos, web and windows.
- **SConstruct options:**
  - `target` (164), `production` (186; it sets static C++, no debug symbols and `lto=auto` at 674-680), `threads` (187), `deprecated` (190), `minizip` and `brotli` (194-195).
  - Renderers `vulkan`, `opengl3`, `d3d12`, `metal` (197-200), plus `accesskit` and `sdl` (202-204).
  - `disable_3d`, `disable_advanced_gui`, `disable_physics_2d`, `disable_physics_3d`, `disable_navigation_2d`, `disable_navigation_3d`, `disable_xr` (264-270). These are rejected for editor builds (1056-1074). `disable_3d` also turns off 3D navigation, 3D physics and XR (1076-1080).
  - `build_profile` (279), `modules_enabled_by_default` (282, 476-485), `library_type` (309-315).
- **Build profile JSON:** keys are `disabled_classes` and `disabled_build_options` (SConstruct:645-664). Disabled classes are generated into `core/disabled_classes.gen.h` (core/SCsub:173; core/core_builders.py:10-14; core/object/class_db.h:55-59). The editor's profile tool maps its options to these SCons flags (editor/settings/editor_build_profile.cpp:52-80).
- **Linux platform toggles:** `x11`, `wayland`, `alsa`, `pulseaudio`, `dbus`, `speechd`, `fontconfig`, `udev` and others (platform/linuxbsd/detect.py:58-68).
- **Module dependencies:** GDScript optionally depends on jsonrpc and websocket (modules/gdscript/config.py:2; methods.py:324-334). The enet, multiplayer, mbedtls and websocket modules have no dependencies.
- **Likely smallest server build** (I have not built this): `platform=linuxbsd target=template_release production=yes modules_enabled_by_default=no module_gdscript_enabled=yes module_enet_enabled=yes module_multiplayer_enabled=yes module_mbedtls_enabled=yes disable_3d=yes disable_advanced_gui=yes disable_navigation_2d=yes vulkan=no opengl3=no x11=no wayland=no alsa=no pulseaudio=no dbus=no speechd=no fontconfig=no udev=no sdl=no accesskit=no`, plus a `disabled_classes` profile.
  - Keep 2D physics if the server does collision checks.
  - Leaving out mbedtls means no DTLS, TLS or Crypto (packet_peer_dtls.cpp:36-39).
  - Whether every flag combination compiles is not verified.

## 6. libgodot
- **API:**
  - `libgodot_create_godot_instance(argc, argv, GDExtensionInitializationFunction)` and `libgodot_destroy_godot_instance`, marked "@since 4.6" (core/extension/libgodot.h:48-71).
  - `GodotInstance` has `start`, `is_started`, `iteration`, `stop`, `focus_in/out`, `pause`, `resume` (godot_instance.h:48-58).
  - `iteration()` calls `DisplayServer::process_events()` and then `Main::iteration()` (godot_instance.cpp:81-83).
- **Building:** set `library_type=static_library|shared_library`, which defines `LIBGODOT_ENABLED` (SConstruct:309-315, 600-604). It is supported on linuxbsd (detect.py:92), macos (detect.py:91) and windows (detect.py:295).
- **Main limitation:** "Only one may be created per process" (GodotInstance.xml:7), enforced at platform/linuxbsd/libgodot_linuxbsd.cpp:42. A host program can embed and drive one engine; running several zones means several processes or several subtrees in one tree.

## 7. Threading
- **Primitives:**
  - `Thread` (core/os/thread.h:64-139, `is_main_thread` at :130).
  - `Mutex` (recursive) and `BinaryMutex` (non-recursive) (mutex.h:97-98).
  - `RWLock` (rw_lock.h:44-98) and `Semaphore` (semaphore.h:52-84).
- **WorkerThreadPool:**
  - `add_task`, `add_group_task`, `wait_for_task_completion`, `wait_for_group_task_completion`, `yield` and native/template variants (worker_thread_pool.h:237-298).
  - Sized from `threading/worker_pool/max_threads` (main.cpp:2142-2149).
  - "Every task must be waited for completion" (WorkerThreadPool.xml:61).
- **Node thread groups:**
  - `process_thread_group` can be INHERIT, MAIN_THREAD or SUB_THREAD (Node.xml:1076-1080, 1358-1365).
  - Sub-thread groups run as one pool group task that the frame waits on (scene_tree.cpp:1313-1314). Each group has its own call queue (1181, 1235).
  - Cross-thread calls go through `call_deferred_thread_group` and `call_thread_safe` (Node.xml:230-237).
- **Thread-safety guards exist only in debug builds:** they compile to nothing in release (scene/main/node.h:934-946). `Thread.set_thread_safety_checks_enabled` is documented at Thread.xml:45-52.
- **MessageQueue:** each thread can override it via a `thread_local` (message_queue.h:150-159), and the pool swaps it per task (worker_thread_pool.cpp:64-69).
- **Objects and resources:**
  - RefCounted uses an atomic `SafeRefCount` (ref_counted.h:34-39), and signal methods are thread-safe (Object.xml:539).
  - Threaded resource loading is available (ResourceLoader.xml:111-129).
  - No general guarantee that Resources can be mutated concurrently (not verified).

## 8. Persistence and databases
- **No database support:** no sqlite, mysql, postgres, libpq, lmdb, leveldb, rocksdb or redis anywhere in the tree, thirdparty or modules.
- **Compression:**
  - FASTLZ, DEFLATE, ZSTD, GZIP and BROTLI modes (core/io/compression.h:47-58).
  - Brotli is "Only brotli decompression is supported." (compression.cpp:51-52, 111-112); only the decoder is vendored (thirdparty/README.md:104).
  - Versions: zstd 1.5.7, zlib 1.3.2, brotli 1.2.0 (README:1304-1307, 1292-1295, 96-99).
- **Other storage helpers:** `StreamPeerGZIP` (stream_peer_gzip.h:54-57), plus `FileAccessCompressed`, `FileAccessEncrypted` and `ConfigFile` (core/io/*.h).
- **Variant marshalling:**
  - `decode_variant(..., p_allow_objects=false)` (marshalls.h:222) has a recursion-depth limit (marshalls.cpp:187).
  - Decoding objects returns ERR_UNAUTHORIZED unless explicitly allowed (718). When allowed, it instantiates any class by name and sets its properties (729-770). Never allow it on input from clients.
  - Script equivalents are `bytes_to_var` and `bytes_to_var_with_objects` (core/variant/variant_utility.cpp:1055-1107; @GlobalScope.xml:192 warning).
  - `StreamPeerBuffer.get_var(allow_objects=false)` and big-endian mode (stream_peer.h:63-94).
  - PackedByteArray has `encode_*`/`decode_*` for s8–u64, half, float, double and var, plus `compress`/`decompress` (PackedByteArray.xml:90-311).

## 9. HTTP and services
- **HTTPClient** (core/io/http_client.h):
  - `connect_to_host(host, port, TLSOptions)` (:169), `request` (:168), GET through PATCH (:116-125), `poll` (:192), `read_response_body_chunk` (:184).
  - Blocking mode "Useful mostly if running in a thread" (:186).
- **HTTPRequest node:** `request` and `request_raw`, `use_threads`, `accept_gzip`, `body_size_limit`, `max_redirects`=8, `timeout` (scene/main/http_request.h:79-153). It is a Node, so it needs a SceneTree.
- **JSON:** `parse`, `stringify`, `from_native`/`to_native` (core/io/json.h:80-91).
- **JSONRPC:** `set_method`, `process_action`, `process_string`, `make_*` (modules/jsonrpc/jsonrpc.h:37-70). It only handles the documents ("A helper to handle dictionaries which look like JSONRPC documents", JSONRPC.xml:4) and has no transport.
- **No HTTP server:** core/io has only the http_client files. This is fine for calling a REST backend from the server.

## 10. Time and determinism
- **Clocks:** `Time.get_ticks_msec/usec` (core/os/time.h:77-78) and `OS::get_ticks_usec` (os.h:281).
- **RandomPCG:**
  - Seed, state and increment are at random_pcg.h:54-72. `get_state`/`set_state` cover only `pcg.state`.
  - The docs warn: "The underlying algorithm is an implementation detail and should not be depended upon" (RandomNumberGenerator.xml:8).
  - Set the seed before restoring state (xml:90-94).
- **Which RNG calls are safe to share between client and server:**
  - Integer `rand()` / `rand(bounds)` are exact integer operations (random_pcg.h:75-80).
  - `randf` depends on whether a CLZ intrinsic is available and has a different fallback (109-119).
  - `randfn` uses cos, sqrt and log (122-135).
  - For a shared client/server battle simulation, use integers only, or vendor your own PCG.
- **Physics:** physics is fixed-step, and `--fixed-fps` gives repeatable stepping. No physics class doc claims determinism (my scan of all doc XMLs for "determinis" found no physics entries).

## 11. Documentation guidance on scale
I scanned every doc XML for MMO / massively / large-scale / thousands of. There are no hits outside rendering and editor topics. The relevant notes are:
- "implementation detail and isn't meant to be used by non-Godot servers. It may change without notice." (SceneMultiplayer.xml:10; MultiplayerPeer.xml:8)
- "Synchronization is not supported for [Object] type properties, like [Resource]." (MultiplayerSynchronizer.xml:11)
- "Visibility filters are not updated automatically, and must be updated manually" (MultiplayerSynchronizer.xml:101)
- "Changing this option while other peers are connected may lead to unexpected behaviors." (SceneMultiplayer.xml:88)
- "Deserialized objects can contain code which gets executed." (SceneMultiplayer.xml:65)
- In code: "This is a lazy implementation, we could optimize much more here..." (scene_replication_interface.cpp:809) and "TODO Handle single state above MTU." (:832)
- "TLS certificate revocation and certificate pinning are currently not supported." (HTTPClient.xml:16)
- "could affect performance negatively if the task being distributed between threads is not computationally expensive" (WorkerThreadPool.xml:43)

## Verdict
**(a) Game clients: solid.** Use SceneTree, GDScript, Resources, ENet with DTLS, HTTPRequest and Crypto as they are. Don't build on SceneMultiplayer's wire protocol, which may change without notice. Speak your own protocol over `ENetConnection`/`ENetPacketPeer`, or through a `MultiplayerAPIExtension`.

**(b) Zone and world servers: usable foundation.**
- **What works:**
  - ENetConnection (4095 peers per host, 255 channels, DTLS, zstd, RTT stats, broadcast).
  - Headless and dedicated-server export with placeholder resources.
  - A ClassDB-registered custom MainLoop, or SceneTree with multiple MultiplayerAPIs.
  - Build profiles and module stripping, WorkerThreadPool, safe Variant marshalling with objects off, compression, HTTPClient for backend calls, and HMAC/RSA/CSPRNG.
- **Things to fix or work around:**
  - One engine per process, so plan multi-process sharding.
  - ENet objects are not thread-safe; give each host one owning thread.
  - Raise the ~144 Hz headless cap or set `max_fps` for your tick rate.
  - Patch the `create_server` argument bug and expose ENet's `duplicatePeers` per-IP limit.
  - Turn off `server_relay`.
  - Thread guards disappear in release builds.

**(c) Must be built new:**
- **Area of interest:** a spatial grid, cell subscriptions, and snapshots encoded once and shared, instead of SceneReplication's per-peer × per-synchronizer encoding.
- **Protocol schema and versioning:** don't rely on the SceneMultiplayer protocol or raw Variant blobs.
- **Persistence:** there is no database in the engine. Use a GDExtension database driver or an external service over HTTP.
- **Login → world → zone session tokens:** HMAC exists, but the token system must be written.
- **Server-to-server messaging, load balancing and zone handoff.**
- **Deterministic battle simulation:** fixed-point math and your own RNG.
- **Validation, rate limiting and per-IP limits.**
- **Message-level authenticated encryption:** AES has only ECB/CBC, so rely on DTLS for transport.
