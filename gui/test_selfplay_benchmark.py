import sqlite3
import tempfile
import unittest
from pathlib import Path

import chess

from gui.selfplay_benchmark import (
    _ai_score,
    _open_database,
    load_openings,
    selfplay_report,
)


class SelfplayBenchmarkTests(unittest.TestCase):
    def test_load_openings_accepts_full_fen_and_epd(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "openings.epd"
            path.write_text(
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n"
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -\n",
                encoding="utf-8",
            )
            openings = load_openings(path)

        self.assertEqual(2, len(openings))
        self.assertEqual(chess.STARTING_FEN, openings[0].fen())

    def test_ai_score_obeys_color(self):
        self.assertEqual(1.0, _ai_score("1-0", "white"))
        self.assertEqual(0.0, _ai_score("1-0", "black"))
        self.assertEqual(1.0, _ai_score("0-1", "black"))
        self.assertEqual(0.5, _ai_score("1/2-1/2", "white"))

    def test_report_uses_complete_color_swapped_pairs(self):
        with tempfile.TemporaryDirectory() as directory:
            database_path = Path(directory) / "selfplay.sqlite3"
            database = _open_database(database_path)
            cursor = database.execute(
                "INSERT INTO selfplay_runs(created_at, model_sha256, engine_sha256, "
                "book_sha256, nodes, pair_count, base_seed, max_plies) "
                "VALUES (0, 'm', 'e', 'b', 2000, 2, 1, 200)"
            )
            run_id = int(cursor.lastrowid)
            rows = [
                (run_id, 0, 0, "white", "1-0", "checkmate", 40, 1000.0),
                (run_id, 0, 1, "black", "0-1", "checkmate", 42, 1200.0),
                (run_id, 1, 0, "white", "1/2-1/2", "max-plies", 200, 3000.0),
                (run_id, 1, 1, "black", "1/2-1/2", "max-plies", 200, 3000.0),
            ]
            database.executemany("INSERT INTO selfplay_games VALUES (?, ?, ?, ?, ?, ?, ?, ?)", rows)
            database.commit()
            database.close()

            _, summary, markdown = selfplay_report(database_path, run_id, bootstrap_samples=100)

        self.assertEqual(2, summary["wins"])
        self.assertEqual(2, summary["draws"])
        self.assertEqual(0, summary["losses"])
        self.assertEqual(2, summary["completePairs"])
        self.assertEqual(50, summary["policyWeight"])
        self.assertIn("Tanult policy súlya: 0.50", markdown)
        self.assertAlmostEqual(0.75, summary["score"])
        self.assertEqual(0, summary["forfeits"])
        self.assertEqual(4, summary["nonForfeitGames"])
        self.assertIn("2 győzelem / 2 döntetlen / 0 vereség", markdown)


if __name__ == "__main__":
    unittest.main()
