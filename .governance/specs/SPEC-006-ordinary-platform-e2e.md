# SPEC006 ordinary terminal platform E2E

Issue #44, parent #1, extends SPEC005 while preserving its ordinary-source-only scope.

## PLATFORM001 Windows terminal
Use actual Python3.12 AMD64 and hash-pinned pywinpty3.0.5 ConPTY on the matching Windows host. Read from its real socket-backed PtyProcess transport, wait for actual UI readiness before any key, verify distinct help/Escape dashboard, navigation, narrow layout, actual resize, reconnect successful Core request counts and q. EOF/non-live is not completion: require actual owned wait plus consistent numeric exit status. Forced cleanup always fails the stage. Controlled fake transport tests do not grant platform/native qualification.

## PLATFORM002 Privacy and tools
Before any secret/raw evidence write, import the shared private-directory bootstrap from the explicitly selected clean Core revision. Actual Windows current-owner+SYSTEM protected inheritable DACL must be read back and verified; chmod is never Windows privacy. Keep child HOME/APPDATA/cache/temp paths inside that root. Carry explicit actual npm-cli.js through Node process.execPath, preserve tool/source/build/cleanup provenance and token leak checks. Unknown privacy refuses before acquisition.

## PLATFORM003 Hosted applicability
Automatic matching-host Linux and Windows ordinary compatibility checks must pass. A macOS POSIX job is separate exploratory/deferred evidence, never GA/native PASS and never a replacement for Windows/Linux. Full v2/operator journeys, guarded OAuth permission denial, native profile/custody/publication/installed-byte acceptance remain separate blocked or unqualified obligations. Preserve existing Linux adapter behavior.
