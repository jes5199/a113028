#!/usr/bin/env python3
"""Independent validator for the A113028 b-file (n = 2..64).

Run from the repo root:  python3 scripts/validate_bfile.py

Inputs (all relative to the repo root, i.e. the current working directory):
  - b113028.txt                       the b-file under test
  - README.md                         the "Known values" table (base-B digit
                                      strings in the extended alphabet)
  - docs/results/FRONTIER-STATUS.md   decimal values for bases 50..64

Checks, per base n = 2..64:
  1. The README table's base-n digit string decodes to exactly the b-file
     value (round-trip through an independent alphabet decoder). The
     corrected a(46) is taken from the README table itself.
  2. For n in 50..64 the b-file value equals the decimal in the
     FRONTIER-STATUS status table (| base | value | status | ... rows).
  3. The base-n digits of a(n) (recomputed from the decimal, not the
     string) are all distinct, all nonzero, all < n.
  4. lcm(digits) divides a(n).
  5. n = 64 only: the digit set is exactly {1..63} and the units digit
     is 32.

Prints one PASS/FAIL line per base and exits 0 only if everything passes.
Standard library only.
"""

import math
import os
import re
import sys

BFILE = "b113028.txt"
README = "README.md"
FRONTIER = os.path.join("docs", "results", "FRONTIER-STATUS.md")

LO, HI = 2, 64

# --------------------------------------------------------------------------
# Extended alphabet: '0'-'9' = 0-9; 'A'-'Z' = 10-35; Greek lowercase
# alpha..nu = 36-48; Hebrew alef..tav (plain consonants, no finals) = 49-70.
# Strings may carry U+200E LEFT-TO-RIGHT MARK after Hebrew letters.
# --------------------------------------------------------------------------
GREEK = "αβγδεζηθικλμν"
HEBREW = ("אבגדהוזחטי"
          "כלמנסעפצקר"
          "שת")
LRM = "‎"

CHARVAL = {}
for _i in range(10):
    CHARVAL[str(_i)] = _i
for _i, _c in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
    CHARVAL[_c] = 10 + _i
for _i, _c in enumerate(GREEK):
    CHARVAL[_c] = 36 + _i
for _i, _c in enumerate(HEBREW):
    CHARVAL[_c] = 49 + _i


def decode(s, base):
    """Decode a most-significant-digit-first extended-alphabet string."""
    n = 0
    for ch in s.replace(LRM, ""):
        if ch not in CHARVAL:
            raise ValueError("unknown character U+%04X" % ord(ch))
        d = CHARVAL[ch]
        if d >= base:
            raise ValueError("digit %d out of range for base %d" % (d, base))
        n = n * base + d
    return n


def to_digits(n, base):
    """Base-`base` digits of n, most significant first."""
    ds = []
    while n:
        n, r = divmod(n, base)
        ds.append(r)
    return ds[::-1]


# --------------------------------------------------------------------------
# Parsers
# --------------------------------------------------------------------------
def parse_bfile(path):
    values = {}
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            line = line.rstrip("\n")
            m = re.fullmatch(r"(\d+) (\d+)", line)
            if not m:
                raise ValueError("%s:%d: malformed line %r" % (path, lineno, line))
            n = int(m.group(1))
            if n in values:
                raise ValueError("%s: duplicate n=%d" % (path, n))
            values[n] = int(m.group(2))
    if sorted(values) != list(range(LO, HI + 1)):
        raise ValueError("%s: expected exactly n=%d..%d" % (path, LO, HI))
    return values


def parse_readme_table(path):
    """Base -> digit string from the README Results table.

    Header: | base | a(base), base-B digits | status |
    Only the FIRST backtick code span of the value cell is the digit string.
    """
    strings = {}
    in_table = False
    with open(path, encoding="utf-8") as f:
        for line in f:
            if re.match(r"\|\s*base\s*\|\s*a\(base\), base-B digits\s*\|\s*status\s*\|", line):
                in_table = True
                continue
            if not in_table:
                continue
            m = re.match(r"\|\s*(\d+)\s*\|([^|]*)\|", line)
            if m:
                span = re.search(r"`([^`]+)`", m.group(2))
                if not span:
                    raise ValueError("README row for base %s has no code span"
                                     % m.group(1))
                strings[int(m.group(1))] = span.group(1)
            elif not line.lstrip().startswith("|"):
                in_table = False  # table ended
    if sorted(strings) != list(range(LO, HI + 1)):
        raise ValueError("README Known-values table: expected bases %d..%d, got %s"
                         % (LO, HI, sorted(strings)))
    return strings


def parse_frontier_table(path):
    """Base -> decimal value from the | base | value | status | ... table."""
    values = {}
    in_table = False
    with open(path, encoding="utf-8") as f:
        for line in f:
            if re.match(r"\|\s*base\s*\|\s*value\s*\|\s*status\s*\|", line):
                in_table = True
                continue
            if not in_table:
                continue
            m = re.match(r"\|\s*(\d+)\s*\|\s*(\d+)\s*\|", line)
            if m:
                values[int(m.group(1))] = int(m.group(2))
            elif not line.lstrip().startswith("|"):
                in_table = False
    if sorted(values) != list(range(50, HI + 1)):
        raise ValueError("FRONTIER-STATUS table: expected bases 50..%d, got %s"
                         % (HI, sorted(values)))
    return values


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------
def main():
    for path in (BFILE, README, FRONTIER):
        if not os.path.exists(path):
            print("FATAL: %s not found (run from the repo root)" % path)
            return 2

    bfile = parse_bfile(BFILE)
    readme = parse_readme_table(README)
    frontier = parse_frontier_table(FRONTIER)

    all_ok = True
    for n in range(LO, HI + 1):
        v = bfile[n]
        problems = []

        # 1. README round-trip (independent decode).
        try:
            dec = decode(readme[n], n)
            if dec != v:
                problems.append("README decode %d != b-file value" % dec)
        except ValueError as e:
            problems.append("README string undecodable: %s" % e)

        # 2. FRONTIER-STATUS decimal, bases 50..64.
        if n >= 50 and frontier[n] != v:
            problems.append("FRONTIER-STATUS value %d != b-file value" % frontier[n])

        # 3./4. Digit validity and lcm divisibility (recomputed from decimal).
        digits = to_digits(v, n)
        dset = set(digits)
        if len(digits) != len(dset):
            problems.append("repeated digit")
        if 0 in dset:
            problems.append("zero digit")
        nonzero = dset - {0}
        if nonzero and v % math.lcm(*nonzero) != 0:
            problems.append("lcm(digits) does not divide a(n)")
        missing = sorted(set(range(1, n)) - dset)

        # 5. Base-64 arithmetic obstruction checks.
        if n == 64:
            if dset != set(range(1, 64)):
                problems.append("digit set is not {1..63}")
            if digits[-1] != 32:
                problems.append("units digit is %d, not 32" % digits[-1])

        if problems:
            all_ok = False
            print("FAIL n=%2d: %s" % (n, "; ".join(problems)))
        else:
            print("PASS n=%2d  |D|=%2d  missing=%s" % (n, len(digits), missing))

    if all_ok:
        print("ALL CHECKS PASSED (%d terms, n=%d..%d)" % (HI - LO + 1, LO, HI))
        return 0
    print("VALIDATION FAILED")
    return 1


if __name__ == "__main__":
    sys.exit(main())
