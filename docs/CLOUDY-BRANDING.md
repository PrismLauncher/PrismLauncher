# Cloudy Launcher branding foundation

## Reference

The current reference is a pixel-art cloud in pale blue and white against black. It is a direction, not a reason to add gradients, glow, glassmorphism, oversized hero panels, or ornamental badges.

## Product rules

- Cloudy Launcher remains an independent fork, never presented as official Prism Launcher.
- Keep GPL-3.0, copyright, and third-party notices intact.
- Keep existing launcher, account, instance, download, and mod-management behavior working while UI changes land incrementally.
- Prefer dense, calm desktop layouts with explicit states: loading, offline, error, empty, updating, and recovery.
- Use the cloud mark for identity and navigation context; do not replace functional icons with emoji.

## Implementation sequence

1. Audit existing Qt widgets, resources, themes, and application metadata.
2. Add Cloudy identity resources without deleting upstream resources.
3. Introduce navigation architecture behind small, buildable changes.
4. Build and verify Windows packaging after each major UI change.
5. Preserve a clear rollback path for every visual migration.
