PR #15 exact-head failure evidence was retained for `d0b948ed60f9666d6d8478513b8ef6b036b8d65e`.

macOS job [109851531497](https://github.com/service-lasso/service-lasso-tui/actions/runs/36704529108/job/109851531497) passed Go tests and build, then failed the Python receipt guard because its fake-PTY test dereferences Windows-only `Backend.ConPTY` on macOS. The real ConPTY probe remains Windows-only and the Windows test/build job passed. This is a cross-platform test-harness repair, not acceptance evidence or a product regression finding. Raw log SHA-256: `240921589953f352b2086a3393e56e4932fec85136df31dfb0acf14146857252`.

The required repair is bounded: make the fake-PTY path tolerate an absent/explicit backend, retaining the Windows-only real probe and strict receipt assertions. Direct candidate acceptance, release, and GA remain open.
