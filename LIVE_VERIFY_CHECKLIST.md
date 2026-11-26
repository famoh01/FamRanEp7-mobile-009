# LIVE_VERIFY_CHECKLIST.md — RAN Online EP7 Mobile

> Purpose: steps to verify mobile protocol/input behavior against a real EP7 server.
> Status: **offline-verified only**. No live server/capture was performed in this sandbox (B3 accepted 2026-11-25).
> Do not claim live compatibility until these checks pass with evidence.

## Scope

- Thai login path (TH_PARAM): `THAI_NET_MSG_LOGIN` (2079), `THAI_NET_LOGIN_DATA` 56B native ABI, TEA key `"Steven Seagal Neck Break"`, password before userid.
- Heartbeat: REQ 160 / ANS 161, 24B answer, Agent key `NET_MSG_SND_ENCRYPT_KEY` (2099) and rolling TEA.
- TCP frame reassembly: dwSize header, NET_DATA_BUFSIZE 1024.
- Movement vertical slice (Phase 3.2): `SNETPC_GOTO` (NET_MSG_GCTRL_GOTO 3031), 36B, dwActState/vCurPos/vTarPos.

## Preconditions

- [ ] Live EP7 server endpoint (IP:port) for Login/Agent/Field as applicable.
- [ ] Test account credentials (do **not** commit credentials to repo).
- [ ] PC reference client/build that is known-good against same server (for capture comparison).
- [ ] Wireshark/tcpdump available on capture host.
- [ ] Mobile device/build with logging enabled (adb logcat or equivalent).

## A. Capture Setup

1. Start packet capture filtered to server IP/ports: `tcp.port == <port>`.
2. Record capture file name, server build/region, date/time, client build SHA.
3. Ensure capture host sees raw TCP payload (TLS not used by EP7 legacy protocol; confirm).

## B. Thai Login (Offline vectors → Live)

| Check | Offline evidence | Live verify step | Pass criteria |
|---|---|---|---|
| Opcode | 2079 (`EP7_THAI_NET_MSG_LOGIN`) | Capture login packet from mobile client | First login msg type == 2079 |
| Size | 56 bytes (`dwSize==56`) | Inspect packet length | Exactly 56 bytes on wire for struct |
| Field order | password[21] @ offset 12, userid[21] @ offset 33 | Hex dump fields before/after TEA context | Password field precedes userid |
| TEA | Key "Steven Seagal Neck Break" (16-byte derived), encrypt 21-byte fields | Compare encrypted field bytes to known-good PC client capture for same plaintext test (or decrypt on server log if available) | Byte-identical encrypted fields for same input/key/ABI |
| Tail padding | bytes 54-55 zero | Hex dump | tailPadding == 00 00 |
| Login FB | 2080 expected | Capture response | Server response opcode/flow matches expected Thai path |

## C. Heartbeat / Encrypt Key

| Check | Offline evidence | Live verify step | Pass criteria |
|---|---|---|---|
| REQ/ANS | 160/161 absolute | Capture heartbeat exchange | REQ==160, ANS==161 |
| ANS size | 24 bytes | Hex dump | dwSize==24 |
| Agent SND_ENCRYPT_KEY | opcode 2099, 24B | If Agent sends key, capture it | Mobile switches to rolling TEA mode |
| Rolling TEA | Each ANS encrypts key buffer (13 bytes) over prior round | Capture >=2 consecutive heartbeat rounds on Agent | Round N key == TEA(round N-1 key) as per minTea behavior |
| No unsolicited heartbeat | Client replies to REQ, no timer-driven send | Observe idle period | No ANS without REQ |

## D. Framing / Reassembly

| Check | Offline evidence | Live verify step | Pass criteria |
|---|---|---|---|
| dwSize framing | dwSize includes 8-byte header | Inspect multiple packets | dwSize matches actual frame |
| Fragmentation | Stream reassembles split/coalesced frames | Force Nagle/segmentation or capture natural splits | Mobile handles split frames; no desync |
| Invalid size | <8 or >1024 rejected/reset | Fuzz or negative test server (optional) | Client resets/disconnects safely, no crash |

## E. Movement Vertical Slice (Phase 3.2)

| Check | Offline evidence | Live verify step | Pass criteria |
|---|---|---|---|
| Opcode | NET_MSG_GCTRL_GOTO 3031 (TH_PARAM) | Capture movement while joystick active | Opcode == 3031 |
| Size | 36 bytes | Hex dump | dwSize == 36 |
| dwActState | EM_ACT_RUN 0x00000001 when running | Compare walk vs run joystick magnitude | Run sets bit 0; walk clears it |
| vCurPos/vTarPos | 3 floats each, offsets 12 and 24 | Decode floats | Positions plausible in world coords; no NaN/Inf |
| No invented opcode | Only GOTO/MOVESTATE evidenced | Code review + capture | No unknown movement opcode emitted by this slice |

## F. Reconnect / Session Timeout (needs source/live evidence)

- [ ] **Evidence gap**: No explicit reconnect/session-timeout constants were added in Phase 3.4 because no authoritative PC client constant was verified in the imported mobile codebase during this round. 
- [ ] Live verify: observe server-side idle timeout behavior, disconnect/reconnect flow, and whether client must re-login/reselect character.
- [ ] Capture: record any timeout-related packet or TCP reset behavior.

## G. Reporting

For each live verification run, record:
- Date/time, tester, device/OS, build SHA
- Server endpoint/build/region (no credentials)
- Capture file name/hash
- Table row result: PASS/FAIL/DEFERRED
- Any deviation from offline vectors (treat as blocker before claiming live compatibility)

## Known Limits

- Offline tests prove code behavior against static evidence and test vectors; they **do not** prove live EP7 interoperability.
- TH_PARAM ABI 56B for login is best static evidence; live capture confirmation still required (B8 note in prior docs).
- Movement slice sends GOTO intent; full movement sync/authority/collision remains future work.
