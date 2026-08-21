"""End-to-end smoke test for the local YaC Studio GUI."""

from pathlib import Path
from playwright.sync_api import sync_playwright


ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    console_errors: list[str] = []
    page_errors: list[str] = []
    with sync_playwright() as playwright:
        browser = playwright.chromium.launch(headless=True)
        page = browser.new_page(viewport={"width": 1440, "height": 1000}, device_scale_factor=1)
        page.on("console", lambda message: console_errors.append(message.text) if message.type == "error" else None)
        page.on("pageerror", lambda error: page_errors.append(str(error)))
        page.goto("http://127.0.0.1:8765")
        page.wait_for_load_state("networkidle")

        page.wait_for_selector("#engine-state.online")
        try:
            page.wait_for_function("() => document.querySelectorAll('.square').length === 64")
        except Exception as error:
            raise AssertionError(
                f"Board did not render; console={console_errors}, page_errors={page_errors}, "
                f"toast={page.locator('#toast').inner_text()}"
            ) from error
        assert page.locator(".square").count() == 64
        assert page.locator("#iterations").get_attribute("max") == "200000"
        assert page.locator("#exploration").get_attribute("value") == "1.5"
        assert page.locator("#learned-policy").count() == 1
        assert page.locator("#policy-learning").count() == 1
        desktop_sizes = page.locator(".square").evaluate_all(
            "elements => elements.map(element => { const r = element.getBoundingClientRect(); return [r.width, r.height]; })"
        )
        assert max(width for width, _ in desktop_sizes) - min(width for width, _ in desktop_sizes) < 0.1
        assert max(height for _, height in desktop_sizes) - min(height for _, height in desktop_sizes) < 0.1
        assert max(abs(width - height) for width, height in desktop_sizes) < 0.1
        assert "20 legális" in page.locator("#legal-count").inner_text()

        page.locator("#random-puzzle-button").click()
        page.wait_for_function("() => document.querySelector('#puzzle-source').textContent.startsWith('Lichess ')")
        page.wait_for_function("() => !document.querySelector('#random-puzzle-button').disabled")
        assert page.locator("#puzzle-solution").is_visible()
        assert page.locator("#puzzle-best-move").inner_text() == "elrejtve"
        page.locator("#reveal-puzzle-button").click()
        revealed_move = page.locator("#puzzle-best-move").inner_text()
        assert len(revealed_move) in (4, 5)

        page.locator('[data-preset="start"]').click()
        page.wait_for_function("() => document.querySelector('#fen-input').value.startsWith('rnbqkbnr/pppppppp')")
        page.wait_for_function("() => document.querySelector('#puzzle-solution').hidden")
        page.locator("#sq-e2").click()
        assert page.locator("#sq-e4").evaluate("element => element.classList.contains('legal')")
        page.locator("#sq-e4").click()
        page.wait_for_function("() => document.querySelector('#fen-input').value.includes('4P3')")
        assert "e2e4" in page.locator("#move-list").inner_text()

        page.locator("#undo-button").click()
        page.wait_for_function("() => document.querySelector('#fen-input').value.startsWith('rnbqkbnr/pppppppp')")

        page.locator('[data-preset="mate6"]').click()
        page.wait_for_function("() => document.querySelector('#fen-input').value.startsWith('r4k2')")
        page.locator("#iterations").evaluate("element => { element.value = 800; element.dispatchEvent(new Event('input', {bubbles:true})); }")
        page.locator("#analyze-button").click()
        page.wait_for_function("() => document.querySelector('#best-move').textContent === 'e3e8'", timeout=15000)
        page.wait_for_function("() => document.querySelectorAll('#candidate-list tr:not(.candidate-empty)').length >= 3")
        candidate_rows = page.locator("#candidate-list tr:not(.candidate-empty)")
        assert any("e3e8" in row for row in candidate_rows.all_inner_texts())
        assert candidate_rows.first.locator(".candidate-score").inner_text()
        assert "%" in candidate_rows.first.locator(".candidate-prior").inner_text()
        assert candidate_rows.first.locator(".candidate-play").count() == 1
        assert "minta" in page.locator("#policy-status").inner_text()
        candidate_scores = candidate_rows.locator(".candidate-score").all_inner_texts()
        numeric_scores = [float(score.replace("+", "")) for score in candidate_scores]
        assert numeric_scores == sorted(numeric_scores, reverse=True)

        screenshot = ROOT / "build" / "yac-studio.png"
        screenshot.parent.mkdir(exist_ok=True)
        page.screenshot(path=str(screenshot), full_page=True)

        mobile = browser.new_page(viewport={"width": 390, "height": 844}, device_scale_factor=1)
        mobile.on("console", lambda message: console_errors.append(message.text) if message.type == "error" else None)
        mobile.goto("http://127.0.0.1:8765")
        mobile.wait_for_load_state("networkidle")
        mobile.wait_for_function("() => document.querySelector('#engine-state').classList.contains('online')")
        mobile.wait_for_function("() => document.querySelectorAll('.square').length === 64")
        assert mobile.locator(".square").count() == 64
        mobile_sizes = mobile.locator(".square").evaluate_all(
            "elements => elements.map(element => { const r = element.getBoundingClientRect(); return [r.width, r.height]; })"
        )
        assert max(width for width, _ in mobile_sizes) - min(width for width, _ in mobile_sizes) < 0.1
        assert max(height for _, height in mobile_sizes) - min(height for _, height in mobile_sizes) < 0.1
        assert mobile.evaluate("() => document.documentElement.scrollWidth <= window.innerWidth + 1")
        mobile.screenshot(path=str(ROOT / "build" / "yac-studio-mobile.png"), full_page=True)
        browser.close()

    assert not console_errors, f"Browser console errors: {console_errors}"
    assert not page_errors, f"Browser page errors: {page_errors}"
    print("YaC Studio browser test passed")


if __name__ == "__main__":
    main()
