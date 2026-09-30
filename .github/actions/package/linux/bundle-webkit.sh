#!/usr/bin/env bash
# Makes the WebKitGTK copy that `sharun lib4bin --with-hooks` bundled into an AppDir independent of the
# working directory.
#
# libwebkit2gtk-4.1 has the directory of its helper processes (WebKitWebProcess, WebKitNetworkProcess,
# WebKitGPUProcess) and of its injected bundle compiled in: /usr/lib/<multiarch>/webkit2gtk-4.1. sharun's
# hooks copy that directory to shared/lib/webkit2gtk-4.1 (with sharun wrappers for the helpers) and rewrite
# the path to "././/lib/<multiarch>/webkit2gtk-4.1", i.e. relative to the working directory. The launcher
# changes its working directory to the data folder on start-up, so this script instead
#   1. rewrites the path (either form) to a same-length absolute path in /tmp, and
#   2. tells the launcher through <AppDir>/.env to point that path at shared/lib/webkit2gtk-4.1 on start-up
#      (see prepareRelocatedWebKit() in launcher/webview/LinuxWebView.cpp).
#
# usage: bundle-webkit.sh <AppDir> [multiarch]
set -euo pipefail

appdir=${1:?usage: bundle-webkit.sh <AppDir> [multiarch]}
multiarch=${2:-$(dpkg-architecture -q DEB_HOST_MULTIARCH)}
target=shared/lib/webkit2gtk-4.1

library=$(find "$appdir/shared/lib" -name 'libwebkit2gtk-4.1.so*' -type f | head -n 1)
if [[ -z "$library" ]]; then
	echo "bundle-webkit: libwebkit2gtk-4.1 was not bundled into $appdir" >&2
	exit 1
fi
for process in WebKitWebProcess WebKitNetworkProcess; do
	if [[ ! -x "$appdir/$target/$process" ]]; then
		echo "bundle-webkit: $target/$process is missing (was sharun lib4bin run with --with-hooks?)" >&2
		exit 1
	fi
done

original=
for candidate in "/usr/lib/$multiarch/webkit2gtk-4.1" "././/lib/$multiarch/webkit2gtk-4.1"; do
	if grep -aq "$candidate" "$library"; then
		original=$candidate
		break
	fi
done
if [[ -z "$original" ]]; then
	echo "bundle-webkit: $library does not contain a known helper path" >&2
	exit 1
fi

# Same length as the original, so the binary layout does not change.
prefix=/tmp/.materialmc-webkit-
padding=$((${#original} - ${#prefix}))
if ((padding < 6)); then
	echo "bundle-webkit: $original is too short to relocate" >&2
	exit 1
fi
# (head closing the pipe makes tr fail with SIGPIPE, which pipefail would turn into an error)
suffix=$(LC_ALL=C tr -dc 'a-zA-Z0-9' </dev/urandom 2>/dev/null | head -c "$padding" || true)
link=$prefix$suffix
if ((${#link} != ${#original})); then
	echo "bundle-webkit: could not generate the replacement path" >&2
	exit 1
fi

ORIGINAL=$original LINK=$link perl -0777 -pi -e 's/\Q$ENV{ORIGINAL}\E/$ENV{LINK}/g' "$library"
if grep -aq "$original" "$library" || ! grep -aq "$link" "$library"; then
	echo "bundle-webkit: rewriting the helper path in $library failed" >&2
	exit 1
fi
echo "bundle-webkit: $original -> $link (${library#"$appdir"/})"

# glvnd needs the Mesa vendor file to use the bundled libEGL_mesa (sharun points __EGL_VENDOR_LIBRARY_DIRS here)
vendor_dir=$appdir/share/glvnd/egl_vendor.d
if [[ ! -e "$vendor_dir/50_mesa.json" ]]; then
	if [[ ! -e /usr/share/glvnd/egl_vendor.d/50_mesa.json ]] || ! find "$appdir/shared/lib" -name 'libEGL_mesa.so*' | grep -q .; then
		echo "bundle-webkit: Mesa's EGL vendor was not bundled (WebKitGTK needs EGL)" >&2
		exit 1
	fi
	mkdir -p "$vendor_dir"
	cp /usr/share/glvnd/egl_vendor.d/50_mesa.json "$vendor_dir/"
fi

cat >>"$appdir/.env" <<EOF
MATERIALMC_WEBKIT_LINK=$link
MATERIALMC_WEBKIT_LINK_TARGET=$target
WEBKIT_DISABLE_SANDBOX_THIS_IS_DANGEROUS=1
EOF
echo "bundle-webkit: done"
