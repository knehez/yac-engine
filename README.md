# Yet Another Chess Engine (YAC-Engine)

[![Travis build status](https://api.travis-ci.com/knehez/yac-engine.svg?branch=master)](https://travis-ci.com/knehez/yac-engine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/6dc79cb15b5748cba7e4d68d482b3f8b)](https://www.codacy.com/manual/knehez/yac-engine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=knehez/yac-engine&amp;utm_campaign=Badge_Grade)
[![Coverage Status](https://coveralls.io/repos/github/knehez/yac-engine/badge.svg?branch=master)](https://coveralls.io/github/knehez/yac-engine?branch=master)

Simple 64-bit bitboard based chess engine written in C++. Developed in TDD style.

See .travis.yml for installation details.

# Installation steps

**1.) Clone repository**

**2.) Google test manual installation steps:**

    mkdir lib
    cd lib
    git clone https://github.com/google/googletest.git
    cd googletest
    git checkout tags/release-1.12.0

**3.) Build from source and launch tests**

    cd ../../
    mkdir build
    cd build
    cmake ..
    make
    cd build/test
    yace-test

## Development tools
**Bitboard calculator**

    http://cinnamonchess.altervista.org/bitboard_calculator/Calc.html

**Code Coverage Analysis** 

To generate code coverage analysis reports, setup and use OpenCppCoverage.

Setup from here

    https://github.com/OpenCppCoverage/OpenCppCoverage
    
Usage

    opencppcoverage --sources src -- yace_test.exe

## YaC Studio GUI

A local browser GUI is included for playing through the UCI interface and for
loading or analysing FEN positions. Build the engine in Release mode, then run:

    python gui/server.py

The application opens at `http://127.0.0.1:8765`. To use another engine binary:

    python gui/server.py --engine path/to/yace.exe

The GUI supports click-to-move play, legal move highlighting, undo, board
orientation, FEN presets, static evaluation and configurable MCTS analysis.
The `Felfedezés` control maps to the UCI `Exploration` option (0.25–5.00;
default 1.50): higher values spend more visits on less explored root moves.
When an MCTS rollout reaches checkmate, YaC first verifies forcing checking
lines up to eleven plies, then falls back to a general seven-ply AND/OR search.
The bounded verifier reports each candidate as `kényszermatt`, `védhető`, or
`nem eldöntött` in the move table. The shared proof-search deadline is exposed
as the UCI `MateProofTimeMs` option (default 2000 ms), so a difficult or
non-mating candidate cannot block a move indefinitely.

MCTS leaf positions use a phase-aware tapered evaluation built from bitboards:
material, piece activity, pawn structure, passed pawns, open rook files, bishop
pair and king safety are blended continuously between middlegame and endgame.
Opening positions use this inexpensive leaf value directly; tactical and
low-material positions retain deeper rollouts.

### Learned MCTS policy

YaC Studio includes an optional compact `18 -> 32 -> 16 -> 1` neural move policy. It
only changes the PUCT prior; legal move generation, implicit minimax backup and
proven mate results remain authoritative. A new model is exactly neutral.

Enable `Megbízható elemzések gyűjtése` to collect a weighted teacher sample
from a proven mate or from a stable search with at least 100,000 iterations.
The network trains between searches after 50 new samples, or on demand with
`Tanítás most`. Enable `Tanult prior használata` to blend the learned prior
with the tactical prior. The candidate table shows the resulting AI policy
distribution. Model and replay data are stored locally as `yac_policy.model`
and `yac_policy.samples`; the replay buffer retains up to 20,000 samples, and
the GUI can delete both files at any time.

Corresponding UCI options and commands are `LearnedPolicy`, `PolicyLearning`,
`PolicyWeight`, `TrainPolicy`, `ClearPolicy`, and `policystatus`.

### Lichess puzzle import

The learned policy can also receive externally verified move labels from the
CC0-licensed [Lichess puzzle database](https://database.lichess.org/#puzzles).
The importer applies the first UCI move to the exported FEN, then uses every
subsequent player move as a legal, one-hot policy target. Positions and moves
are validated by YaC's own move generator before they are stored.

Install the streaming Zstandard reader, download `lichess_db_puzzle.csv.zst`,
then run:

    python -m pip install -r gui/requirements-import.txt
    python gui/lichess_import.py lichess_db_puzzle.csv.zst --max-samples 2000

The compressed export can also be streamed directly without storing it first:

    python gui/lichess_import.py https://database.lichess.org/lichess_db_puzzle.csv.zst --max-samples 2000

By default the importer requires popularity >= 80, at least 100 plays and
rating deviation <= 100. Mate-in-one puzzles are skipped because several
different mating moves may be correct. Repeated imports are deduplicated in
`yac_lichess_import.sqlite3`; the policy samples remain in
`yac_policy.samples`. Optional filters include `--themes mate,mateIn2`,
`--min-popularity`, `--min-plays` and `--max-rating-deviation`.
Positions stored in `yac_policy_benchmark.sqlite3` are excluded automatically,
so a later training import cannot contaminate the held-out benchmark set.

The UCI batching commands used by the importer are `policyimport begin`,
`policylabel <uci-move> <confidence>` and `policyimport end`. Samples are
written once at the end of a batch instead of rewriting the replay file after
every imported position.

### Held-out policy benchmark

Create a test-only set that excludes every puzzle ID and canonical FEN already
used for training, then compare identical seeded MCTS searches with the learned
policy disabled and enabled:

    python gui/policy_benchmark.py collect --count 2000
    python gui/policy_benchmark.py run --limit 2000 --iterations 2000,10000,50000

The Markdown and CSV reports contain paired Top-1/Top-3 accuracy, target visits,
improved/regressed counts and an exact McNemar p-value. Interrupted runs keep
every completed pair and can be continued with `--resume-run <id>`. Held-out
positions are never written to `yac_policy.samples` and are automatically
excluded from later Lichess training imports.
Long runs can be processed in bounded chunks with `--max-pairs`, then resumed
under the same run ID.

### Paired self-play benchmark

To measure whether the learned policy improves complete games, run paired
AI-versus-control self-play. Each opening is played twice with swapped colors;
both engines use the same binary, node budget and random seed, while only the
`LearnedPolicy` option differs:

    python gui/selfplay_benchmark.py run --pairs 100 --nodes 2000 --mate-proof-ms 500

The learned prior blend can be varied reproducibly with `--policy-weight`
(0–100); the selected value is stored with the run and included in reports.

The first run downloads a small CC0 opening set from the official Stockfish
books repository. Results are committed after every game to
`yac_selfplay.sqlite3`, and games are appended to `build/selfplay/games.pgn`.
An interrupted run can be continued without replaying completed games:

    python gui/selfplay_benchmark.py run --resume-run 1 --max-games 20

The Markdown/CSV report includes W/D/L, score, a paired-bootstrap 95% interval
for the estimated Elo difference, and an exact win/loss sign-test p-value.
The selected mate-proof deadline is stored with the run and printed in the
report, so resumed and repeated measurements use the same search conditions.
Use many complete pairs before treating the Elo estimate as evidence; a small
run is only a technical smoke test.
