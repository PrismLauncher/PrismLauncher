#!/usr/bin/env bash
# Seeds a throwaway instance library for headless screenshots of the UI work.
# Usage: make-demo-instances.sh <dir>   (creates/refreshes the directory)
set -euo pipefail

DIR="${1:?usage: make-demo-instances.sh <dir>}"
INSTANCES="$DIR/instances"
mkdir -p "$INSTANCES"

# Settings: skip the first-run wizard, use the Blocks theme, show the pixelated cat
cat > "$DIR/prismlauncher.cfg" <<EOF
Language=en_US
UserAskedAboutAutomaticJavaDownload=true
ApplicationTheme=blocks
TheCat=true
CatOpacity=30
JavaPath=/usr/bin/java
LastHostname=$(hostname)
EOF

cat > "$DIR/instgroups.json" <<'EOF'
{
    "formatVersion": 1,
    "groups": {
        "Modpacks": { "hidden": false, "instances": ["all-the-mods-10", "fabulously-optimized", "cobblemon-adventures", "skyfactory-4", "create-kitchen-sink"] },
        "Dev": { "hidden": false, "instances": ["neoforge-dev", "redstone-testing"] }
    }
}
EOF

pack_json() {
    # pack_json <mc-version> [loader-uid:version:name ...]
    local mc="$1"; shift
    printf '{"formatVersion":1,"components":[{"uid":"net.minecraft","version":"%s","cachedVersion":"%s","cachedName":"Minecraft"}' "$mc" "$mc"
    for spec in "$@"; do
        [ "$spec" = "-" ] && continue
        local uid="${spec%%:*}" rest="${spec#*:}"
        local ver="${rest%%:*}" name="${rest#*:}"
        [ "$name" = "$rest" ] && name="$uid"
        printf ',{"uid":"%s","version":"%s","cachedVersion":"%s","cachedName":"%s"}' "$uid" "$ver" "$ver" "$name"
    done
    printf ']}'
}

mkinst() {
    # mkinst <id> <name> <mc-version> [loader-uid:version ...]
    local id="$1" name="$2" mc="$3"; shift 3
    local dir="$INSTANCES/$id"
    mkdir -p "$dir"
    printf 'name=%s\niconKey=default\n' "$name" > "$dir/instance.cfg"
    pack_json "$mc" "$@" > "$dir/mmc-pack.json"
}

set_managed() {
    # set_managed <id> <type>
    printf 'ManagedPack=true\nManagedPackType=%s\n' "$2" >> "$INSTANCES/$1/instance.cfg"
}

set_playtime() {
    # set_playtime <id> <last-launch-epoch-ms> <total-seconds>
    printf 'lastLaunchTime=%s\ntotalTimePlayed=%s\n' "$2" "$3" >> "$INSTANCES/$1/instance.cfg"
}

NOW=$(date +%s%3N)

mkinst all-the-mods-10 "All the Mods 10" 1.21.1 net.neoforged:21.1.90:NeoForge
mkinst fabulously-optimized "Fabulously Optimized" 1.21.4 net.fabricmc.fabric-loader:0.16.10:"Fabric Loader"
mkinst cobblemon-adventures "Cobblemon Adventures" 1.21.1 net.fabricmc.fabric-loader:0.16.9:"Fabric Loader"
mkinst skyfactory-4 "SkyFactory 4" 1.12.2 net.minecraftforge:14.23.5.2860:Forge
mkinst create-kitchen-sink "Create: Kitchen Sink" 1.20.1 net.minecraftforge:47.2.0:Forge
mkinst vanilla-1-21-4 "Vanilla 1.21.4" 1.21.4
mkinst neoforge-dev "NeoForge Dev" 1.21.4 net.neoforged:21.4.10:NeoForge
mkinst redstone-testing "Redstone Testing" 1.20.1
mkinst hardcore-vanilla "Hardcore Vanilla" 1.21.1
mkinst modrinth-speedrun "Modrinth Speedrun" 1.21.1 net.fabricmc.fabric-loader:0.15.11:"Fabric Loader"
mkinst technic-classic "Technic Classic" 1.7.10 net.minecraftforge:10.13.4.1614:Forge

for id in fabulously-optimized cobblemon-adventures modrinth-speedrun; do
    set_managed "$id" modrinth
done
for id in all-the-mods-10 skyfactory-4 create-kitchen-sink; do
    set_managed "$id" flame
done

set_playtime all-the-mods-10 "$((NOW - 2*3600*1000))" 43200
set_playtime fabulously-optimized "$((NOW - 3*24*3600*1000))" 28800
set_playtime cobblemon-adventures "$((NOW - 26*3600*1000))" 7200
set_playtime vanilla-1-21-4 "$((NOW - 30*60*1000))" 5400
set_playtime neoforge-dev "$((NOW - 2*24*3600*1000))" 3600
set_playtime skyfactory-4 "$((NOW - 62*24*3600*1000))" 18000

echo "Seeded $(find "$INSTANCES" -mindepth 1 -maxdepth 1 -type d | wc -l) demo instances in $DIR"
