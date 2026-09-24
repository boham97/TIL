#!/bin/bash
# memcheck.sh - glibc malloc snapshot / trim helper for a running process
#
#   ./memcheck.sh <PID>              one snapshot
#   ./memcheck.sh <PID> <tag>        snapshot + append to ./memcheck.csv
#   ./memcheck.sh <PID> -w [sec]     watch loop (default 60s)
#   ./memcheck.sh <PID> -t           malloc_trim(0), before/after RSS
#   ./memcheck.sh <PID> -x [file]    dump malloc_info XML (default ./mi.xml)
#   ./memcheck.sh <PID> -i           show env tunables, threads, arena maps
#   ./memcheck.sh <PID> -s           thread backtraces -> ./bt_<pid>_<time>.txt

set -u
PID=${1:-}
[ -z "$PID" ] && { sed -n '2,12p' "$0" | sed 's/^# \?//'; exit 1; }
kill -0 "$PID" 2>/dev/null || { echo "no such process: $PID"; exit 1; }
MODE=${2:-}
CSV=./memcheck.csv

rss_kb()  { awk '/VmRSS/{print $2}'  /proc/$PID/status; }
vsz_kb()  { awk '/VmSize/{print $2}' /proc/$PID/status; }
nthr()    { awk '/Threads/{print $2}' /proc/$PID/status; }

# echoes: arena ordblks smblks hblks hblkhd usmblks fsmblks uordblks fordblks keepcost
mallinfo_raw() {
    gdb -p "$PID" -batch \
        -ex 'set $b = (unsigned long *) malloc(80)' \
        -ex 'call ((void (*)(void *)) mallinfo2)($b)' \
        -ex 'printf "MI %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n", \
             $b[0],$b[1],$b[2],$b[3],$b[4],$b[5],$b[6],$b[7],$b[8],$b[9]' \
        -ex 'call (void) free($b)' \
        -ex detach 2>/dev/null | awk '/^MI /{ $1=""; print; exit }'
}

mb() { awk -v v="$1" 'BEGIN{printf "%.1f", v/1048576}'; }

snapshot() {
    local tag="${1:-}" raw
    raw=$(mallinfo_raw)
    [ -z "$raw" ] && { echo "mallinfo2 call failed (gdb/ptrace permission?)"; exit 1; }
    set -- $raw
    local arena=$1 hblks=$4 hblkhd=$5 uord=$8 ford=$9 keep=${10}
    local frag=$((ford - keep))
    local rss=$(rss_kb) vsz=$(vsz_kb) thr=$(nthr)

    printf '%s%s\n' "$(date '+%F %T')" "${tag:+  [$tag]}"
    printf '  arena    = %8s MB   (uordblks + fordblks)\n' "$(mb $arena)"
    printf '  uordblks = %8s MB   <- in use by the app   ** leak indicator **\n' "$(mb $uord)"
    printf '  fordblks = %8s MB   <- free, still held by allocator\n' "$(mb $ford)"
    printf '  keepcost = %8s MB   <- top chunk (trim-able)\n' "$(mb $keep)"
    printf '  frag     = %8s MB   (fordblks - keepcost)\n' "$(mb $frag)"
    printf '  mmap     = %8s MB   in %s chunks   %s\n' "$(mb $hblkhd)" "$hblks" \
        "$([ "$hblks" -eq 0 ] && echo '<- 0 means mmap_threshold ratcheted up' || echo '<- mmap path active')"
    printf '  RSS      = %8s MB   VSZ = %s MB   threads = %s\n' \
        "$(mb $((rss*1024)))" "$(mb $((vsz*1024)))" "$thr"

    if [ -n "$tag" ]; then
        [ -f "$CSV" ] || echo "time,tag,arena,uordblks,fordblks,keepcost,hblks,hblkhd,rss_kb,threads" > "$CSV"
        echo "$(date '+%F %T'),$tag,$arena,$uord,$ford,$keep,$hblks,$hblkhd,$rss,$thr" >> "$CSV"
        echo "  -> appended to $CSV"
    fi
}

do_trim() {
    local before after r
    before=$(rss_kb)
    r=$(gdb -p "$PID" -batch -ex 'call (int) malloc_trim(0)' -ex detach 2>/dev/null \
        | awk '/^\$[0-9]+ = /{print $3; exit}')
    after=$(rss_kb)
    echo "malloc_trim(0) returned ${r:-?}   (1 = memory was released)"
    printf 'RSS %s MB -> %s MB   (freed %s MB)\n' \
        "$(mb $((before*1024)))" "$(mb $((after*1024)))" "$(mb $(( (before-after)*1024 )))"
}

dump_xml() {
    local f=${1:-./mi.xml}
    case "$f" in /*) ;; *) f="$PWD/${f#./}" ;; esac
    gdb -p "$PID" -batch \
        -ex "set \$f = (void *) fopen(\"$f\", \"w\")" \
        -ex 'call (int) malloc_info(0, $f)' \
        -ex 'call (int) fclose($f)' -ex detach >/dev/null 2>&1
    if [ -s "$f" ]; then
        echo "wrote $f"
        echo "  arenas  : $(( $(grep -c '<heap nr=' "$f") - 1 ))"
        grep -o 'size from="8[0-9]\{6\}"' "$f" | sort | uniq -c | sed 's/^/  8MB-ish: /'
        grep -o '<total type="mmap"[^/]*' "$f" | sed 's/^/  /'
    else
        echo "failed to write $f (target process may lack write permission there)"
    fi
}

show_info() {
    echo "== tunables in target env =="
    tr '\0' '\n' < /proc/$PID/environ 2>/dev/null \
        | grep -E 'GLIBC_TUNABLES|MALLOC_' || echo "  (none set)"
    echo "== process =="
    ps -o pid,ppid,user,nlwp,rss,vsz,etime,cmd -p "$PID"
    echo "== 64MB-aligned anon maps (thread arenas) =="
    grep -E '^7f[0-9a-f]+000000-' /proc/$PID/maps 2>/dev/null | head -20
    echo "== top RSS mappings =="
    pmap -x "$PID" 2>/dev/null | sort -k3 -n | tail -8
}

case "$MODE" in
    -t) do_trim ;;
    -x) dump_xml "${3:-./mi.xml}" ;;
    -i) show_info ;;
    -s) out="./bt_${PID}_$(date +%H%M%S).txt"
        gdb -p "$PID" -batch -ex 'set pagination off' -ex 'set print elements 0' \
            -ex 'set width 0' -ex 'thread apply all bt 20' -ex detach > "$out" 2>&1
        echo "wrote $out" ;;
    -w) iv=${3:-60}
        echo "watching every ${iv}s, Ctrl-C to stop"
        while kill -0 "$PID" 2>/dev/null; do
            snapshot "watch"; echo
            sleep "$iv"
        done ;;
    "") snapshot ;;
    *)  snapshot "$MODE" ;;
esac
