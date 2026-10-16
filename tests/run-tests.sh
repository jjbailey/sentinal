#!/bin/bash
# tests/run-tests.sh
# Smoke and regression tests for sentinal; run from the source directory
# after "make". Uses a temporary directory and needs no root privileges.

BIN=${BIN:-$PWD}
T=$(mktemp -d)
trap 'kill $(jobs -p) 2>/dev/null; rm -rf "$T"' EXIT
fail=0

check() {	# check description command...
	local desc=$1
	shift

	if "$@"; then
		echo "ok:   $desc"
	else
		echo "FAIL: $desc"
		fail=1
	fi
}

ini() {		# ini name body -- write a valid INI file, print its path
	local f=$T/$1.ini

	printf '[global]\npidfile = %s/%s.pid\ndatabase = :memory:\n\n%s\n' "$T" "$1" "$2" > "$f"
	chmod 644 "$f"
	echo "$f"
}

# version and option handling

check "-V prints the version" "$BIN/sentinal" -V
"$BIN/sentinal" --bogus >/dev/null 2>&1
check "unknown option exits non-zero" test $? -ne 0
"$BIN/sentinal" -? >/dev/null 2>&1
check "-? exits zero" test $? -eq 0

# thread selection (-v)

mkdir -p "$T/d"
f=$(ini thr "[exp]
dirname = $T/d
pcrestr = ^x
expire = 1D
retmin = 1")
"$BIN/sentinal" -f "$f" -v 2>&1 | grep -q 'dfs: false   exp: true   slm: false   wrk: false'
check "-v reports an exp thread" test ${PIPESTATUS[1]} -eq 0

f=$(ini nothr "[none]
dirname = $T/d
pcrestr = ^x")
"$BIN/sentinal" -f "$f" -v 2>&1 | grep -q 'dfs: false   exp: false   slm: false   wrk: false'
check "-v reports no threads without a trigger key" test ${PIPESTATUS[1]} -eq 0

# INI file permissions

f=$(ini perm "[exp]
dirname = $T/d
pcrestr = ^x
expire = 1D")
chmod 666 "$f"
"$BIN/sentinal" -f "$f" -v >/dev/null 2>&1
check "group/other-writable INI is refused" test $? -ne 0

# expiration removes old matching files and keeps the rest

mkdir -p "$T/exp"
touch -d '3 days ago' "$T/exp/old1" "$T/exp/old2" "$T/exp/other-old"
touch "$T/exp/new"
f=$(ini expire "[e]
dirname = $T/exp
pcrestr = ^old
expire = 1D")
timeout 5 "$BIN/sentinal" -f "$f" >/dev/null 2>&1
check "expire removed old matches" test ! -e "$T/exp/old1" -a ! -e "$T/exp/old2"
check "expire kept non-matching and recent files" test -e "$T/exp/other-old" -a -e "$T/exp/new"

# retmin protects files, dry run removes nothing

mkdir -p "$T/ret"
touch -d '3 days ago' "$T/ret/a1" "$T/ret/a2" "$T/ret/a3"
f=$(ini retmin "[r]
dirname = $T/ret
pcrestr = ^a
expire = 1D
retmin = 3")
timeout 5 "$BIN/sentinal" -f "$f" >/dev/null 2>&1
check "retmin keeps files that would expire" test -e "$T/ret/a1" -a -e "$T/ret/a2" -a -e "$T/ret/a3"

touch -d '3 days ago' "$T/exp/dryfile"
f=$(ini dry "[r]
dirname = $T/exp
pcrestr = ^dryfile
expire = 1D")
timeout 5 "$BIN/sentinal" -f "$f" -D >/dev/null 2>&1
check "dry run removes nothing" test -e "$T/exp/dryfile"

# dfs thread must not rescan constantly when nothing can be removed
# (diskfree = 100 is always "low"; nothing matches pcrestr)

mkdir -p "$T/spin"
touch "$T/spin/keep"
f=$(ini spin "[s]
dirname = $T/spin
diskfree = 100
pcrestr = ^nomatch
retmin = 1")
n=$(timeout 8 "$BIN/sentinal" -f "$f" 2>&1 | grep -c 'low free blocks')
check "dfs backs off when nothing is removable ($n reports)" test "$n" -le 2

# pattern helpers

"$BIN/pcretest" '^ex-.*\.zst$' ex-1.log.zst 2>&1 | grep -q '^match:'
check "pcretest matches" test ${PIPESTATUS[1]} -eq 0
"$BIN/pcretest" '^ex-.*\.zst$' ex.log 2>&1 | grep -q '^no match:'
check "pcretest rejects" test ${PIPESTATUS[1]} -eq 0

exit $fail
