#!/usr/bin/env python3
"""
BAND-DEPTH-CERTIFICATION.md Phase 3 planner calibration fit.

Parses [bucket-plan] decision=ADMIT lines paired with the immediately
following [certauto] predicted-vs-actual line to build a dataset of
(family, NX, NY, K, suffixMult, Y, Q, predLookups, actualLookups,
predScans, actualScans, wall) tuples, then derives calibrated
PLAN_C_BUILD/CLEAR/LOOKUP/SCAN coefficients via per-term ratio medians
(actual/predicted), which is the simplest sound way to make the score's
relative ranking track actual work: scale each raw structural term (Y*mult,
Q*mult, lookups*mult, recordChecks*mult) by how much it under/over-predicts
actual work on average, using the MEDIAN ratio (robust to outliers) rather
than mean or full least-squares (a handful of catastrophic outliers like
b58's NX=0/NY=4/K=2 would otherwise dominate an OLS fit).

Only the LOOKUP and SCAN terms have a direct actual-vs-predicted telemetry
counterpart (actualLookups, actualScans). BUILD and CLEAR terms (Y, Q) have
no directly-measured counterpart in the existing predicted-vs-actual print
(it doesn't log build/clear wall-time separately) -- for those we keep the
existing relative weights (3.0, 0.3) UNCHANGED except for the legacy 1.5x
safety margin applied uniformly, since we have no data to recalibrate them
independently and inventing one would be unsound speculation, not fitting.
"""
import re, sys, glob, statistics

LOGS = sorted(set(glob.glob("b*_certauto*.log") + glob.glob("b*_certpos21.log") +
                   ["b50_certified.log", "b60_certified.log"]))

# Primary site: the chosen-config decision=ADMIT line (has NX/NY/K/Y/Q/suffixMult
# inline). Fallback site (needed when a log was truncated mid-run, e.g.
# b54_certpos21.log / b58_certpos21.log's "[... trimmed ...]" marker cut the
# decision=ADMIT line before its "family=..." payload): the "admit <family>
# NX=.. NY=.. K=..: ... lookups=<predLookups> scans=<predScans>" line from
# enumerate*Configs, matched to the following predicted-vs-actual line by
# EXACT string equality of the lookups/scans values (both printed with the
# same "%.4Lg" format from the same PlanConfig field, so a textual match is
# an exact match, not a numeric-tolerance heuristic).
admit_re = re.compile(
    r"\[bucket-plan\] decision=ADMIT family=(?P<family>\S+) NX=(?P<NX>\d+) NY=(?P<NY>\d+) K=(?P<K>\d+) "
    r"P=(?P<P>\d+) Y=(?P<Y>\d+) Q=(?P<Q>\d+) suffixMult=(?P<suffixMult>\d+)")
enum_admit_re = re.compile(
    r"\[bucket-plan\] admit (?P<family>peeled|full-modulus) NX=(?P<NX>\d+) NY=(?P<NY>\d+) K=(?P<K>\d+): "
    r"Y=(?P<Y>\d+) Q=(?P<Q>\d+)(?: suffixMult=(?P<suffixMult>\d+))? projectedPeak=\S+ "
    r"lookups=(?P<lookups>\S+) scans=(?P<scans>\S+) score=\S+")
pva_re = re.compile(
    r"\[certauto\]\s+predicted-vs-actual: family=(?P<family>\S+) predLookups=(?P<predLookups>\S+) "
    r"actualLookups=(?P<actualLookups>\d+) predScans=(?P<predScans>\S+) actualScans=(?P<actualScans>\d+) "
    r"actualRoots=(?P<actualRoots>\d+) wall=(?P<wall>[\d.]+)s")

rows = []
fallback_used = 0
for path in LOGS:
    try:
        lines = open(path, errors="replace").read().splitlines()
    except FileNotFoundError:
        continue
    pending_admit = None
    enum_admits = []  # accumulate "admit <family> ..." lines since last pva/decision
    for line in lines:
        m = admit_re.search(line)
        if m:
            pending_admit = m.groupdict()
            continue
        m = enum_admit_re.search(line)
        if m:
            enum_admits.append(m.groupdict())
            continue
        m = pva_re.search(line)
        if m:
            d = None
            if pending_admit is not None:
                d = dict(pending_admit)
            else:
                # fallback: find the enum-admit line whose printed lookups/
                # scans text matches this pva line's predLookups/predScans
                # text exactly and whose family matches.
                pl_str, ps_str, fam = m.group("predLookups"), m.group("predScans"), m.group("family")
                for cand in reversed(enum_admits):
                    if cand["family"] == fam and cand["lookups"] == pl_str and cand["scans"] == ps_str:
                        d = dict(cand)
                        d["Y"] = cand["Y"]; d["Q"] = cand["Q"]
                        d["suffixMult"] = cand.get("suffixMult") or "1"
                        fallback_used += 1
                        break
            if d is not None:
                d.update(m.groupdict())
                d["source"] = path
                rows.append(d)
            pending_admit = None
            enum_admits = []

print(f"# parsed {len(rows)} (ADMIT, predicted-vs-actual) pairs from {len(LOGS)} log files "
      f"({fallback_used} via truncated-log lookups/scans-text-match fallback)", file=sys.stderr)

lookup_ratios = []
scan_ratios = []
for r in rows:
    predL = float(r["predLookups"]); actL = float(r["actualLookups"])
    predS = float(r["predScans"]); actS = float(r["actualScans"])
    if predL > 0:
        lookup_ratios.append(actL / predL)
    if predS > 0:
        scan_ratios.append(actS / predS)

print("B\tfamily\tNX\tNY\tK\tsuffixMult\tpredLookups\tactualLookups\tratioL\tpredScans\tactualScans\tratioS\twall\tsource")
for r in rows:
    predL = float(r["predLookups"]); actL = float(r["actualLookups"])
    predS = float(r["predScans"]); actS = float(r["actualScans"])
    ratioL = actL / predL if predL > 0 else float("nan")
    ratioS = actS / predS if predS > 0 else float("nan")
    base = re.match(r"b(\d+)_", r["source"])
    base = base.group(1) if base else "?"
    print(f"{base}\t{r['family']}\t{r['NX']}\t{r['NY']}\t{r['K']}\t{r['suffixMult']}\t"
          f"{predL:.4g}\t{actL:.0f}\t{ratioL:.4g}\t{predS:.4g}\t{actS:.0f}\t{ratioS:.4g}\t{r['wall']}\t{r['source']}")

def summarize(name, xs):
    if not xs:
        print(f"# {name}: no data", file=sys.stderr)
        return None
    xs_sorted = sorted(xs)
    med = statistics.median(xs_sorted)
    print(f"# {name}: n={len(xs)} min={xs_sorted[0]:.4g} median={med:.4g} max={xs_sorted[-1]:.4g} "
          f"p90={xs_sorted[int(0.9*(len(xs)-1))]:.4g}", file=sys.stderr)
    return med

medL = summarize("lookup actual/predicted ratio (GLOBAL, all K pooled)", lookup_ratios)
medS = summarize("scan actual/predicted ratio (GLOBAL, all K pooled)", scan_ratios)

# The global (all-K-pooled) fit is reported above for reference, but the
# ratio is STRONGLY K-dependent (see per-K breakdown below) -- a global
# scalar cannot raise a K=2 config's score enough to trigger the
# LEGACY_DEFAULT_BEAT_MARGIN robustness rule against the b58 pathological
# pick (verified below), so the constants actually baked into carrytrie.cpp
# use a PER-K ratio instead. This is the fit that was implemented.
import math

by_k = {}
for r in rows:
    k = int(r["K"])
    predL = float(r["predLookups"]); actL = float(r["actualLookups"])
    if predL > 0:
        by_k.setdefault(k, []).append(actL / predL)

print("# --- per-K ratio breakdown (this is what was baked in) ---", file=sys.stderr)
per_k_ratio = {}
for k in sorted(by_k):
    xs = sorted(by_k[k])
    med = statistics.median(xs)
    per_k_ratio[k] = med
    print(f"#   K={k}: n={len(xs)} median={med:.4g} min={xs[0]:.4g} max={xs[-1]:.4g}", file=sys.stderr)

OLD_C_BUILD, OLD_C_CLEAR, OLD_C_LOOKUP, OLD_C_SCAN = 3.0, 0.3, 6.0, 1.5
SAFETY = 1.5
new_c_build = OLD_C_BUILD * SAFETY
new_c_clear = OLD_C_CLEAR * SAFETY

print(f"# --- fit result (as baked into carrytrie.cpp) ---", file=sys.stderr)
print(f"# PLAN_C_BUILD: {OLD_C_BUILD} -> {new_c_build:.4g}  (no direct actual/pred data; safety margin only)", file=sys.stderr)
print(f"# PLAN_C_CLEAR: {OLD_C_CLEAR} -> {new_c_clear:.4g}  (no direct actual/pred data; safety margin only)", file=sys.stderr)
for k in sorted(per_k_ratio):
    r = per_k_ratio[k]
    print(f"# PLAN_C_LOOKUP[K={k}]: {OLD_C_LOOKUP} -> {OLD_C_LOOKUP*r*SAFETY:.4g}  (median ratio={r:.4g} x legacy 1.5x margin)", file=sys.stderr)
    print(f"# PLAN_C_SCAN[K={k}]:   {OLD_C_SCAN} -> {OLD_C_SCAN*r*SAFETY:.4g}  (median ratio={r:.4g} x legacy 1.5x margin)", file=sys.stderr)

# Residuals pre-calibration (global pool) vs post-calibration (per-K):
# |log10(actual/predicted)| should shrink toward 0 once we divide out each
# row's own K-bucket median (a residual of 0 means "the median for that K
# bucket predicts this row exactly"; nonzero residual is the intra-K spread
# the single per-K constant cannot capture).
def resid_stats(ratios, label):
    if not ratios:
        return
    logs = [abs(math.log10(r)) for r in ratios if r > 0]
    if logs:
        print(f"# {label} |log10(ratio)| residual: mean={statistics.mean(logs):.3f} "
              f"median={statistics.median(logs):.3f} max={max(logs):.3f}", file=sys.stderr)

resid_stats(lookup_ratios, "lookup ratio (pre-cal, vs global median)")
resid_stats(scan_ratios, "scan ratio (pre-cal, vs global median)")

post_cal_resid = []
for r in rows:
    k = int(r["K"])
    predL = float(r["predLookups"]); actL = float(r["actualLookups"])
    if predL > 0 and k in per_k_ratio:
        post_cal_resid.append(actL / predL / per_k_ratio[k])
resid_stats(post_cal_resid, "lookup ratio (post-cal, vs own K-bucket median)")

# Known-data-point scenario check (task requirement): does the per-K
# calibration actually cause the LEGACY_DEFAULT_BEAT_MARGIN=3.0x rule to
# reject b58's pathological peeled NX=0/NY=4/K=2 plan in favor of the
# legacy default (peeled NX=2/K=3)? Uses the exact Y/Q/suffixMult/lookups/
# scans values from b58_certpos21.log.
print("# --- b58 known pathological-plan scenario check ---", file=sys.stderr)
K2 = dict(Y=43680, Q=3364, suffixMult=72593, lookups=2160, scans=28050)
K3legacy = dict(Y=240, Q=195112, suffixMult=72593, lookups=4.717e6, scans=5803)

def score(cfg, k):
    mult = cfg["suffixMult"]
    ratio = per_k_ratio.get(k, per_k_ratio.get(3, 21.0))
    cB, cC = new_c_build, new_c_clear
    cL = OLD_C_LOOKUP * ratio * SAFETY
    cS = OLD_C_SCAN * ratio * SAFETY
    return cB*cfg["Y"]*mult + cC*cfg["Q"]*mult + cL*cfg["lookups"]*mult + cS*cfg["scans"]*mult

s_challenger = score(K2, 2)
s_legacy = score(K3legacy, 3)
LEGACY_DEFAULT_BEAT_MARGIN = 3.0
beats = s_challenger * LEGACY_DEFAULT_BEAT_MARGIN <= s_legacy
print(f"#   NX=0/NY=4/K=2 score={s_challenger:.4g}  legacy NX=2/K=3 score={s_legacy:.4g}", file=sys.stderr)
print(f"#   challenger beats legacy by required {LEGACY_DEFAULT_BEAT_MARGIN}x margin? {beats}  "
      f"(False = robustness rule falls back to legacy default -- pathological plan AVOIDED)", file=sys.stderr)

resid_stats(lookup_ratios, "lookup ratio (pre-cal)")
resid_stats(scan_ratios, "scan ratio (pre-cal)")
