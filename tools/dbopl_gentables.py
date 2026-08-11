#!/usr/bin/env python3
#
# Generate src/dosbox/dbopl_tables.h -- the OPL wave and volume-multiplier
# tables that DBOPL::InitTables() used to build at runtime with pow()/sin()
# from libm.
#
# Why this exists
# ---------------
# The tables are a fixed set of integers fully determined at compile time; the
# only thing the runtime float code did was recompute the same 1152 constants
# on every startup.  Doing that through libm made the AdLib output depend on
# the host's pow()/sin() implementation and on the compiler's floating point
# settings.  The rounding margins are thin enough to matter: the tightest one
# is MulTable[341] = 40.49982446652..., only 1.755e-4 below the round-half-up
# tie point, i.e. a relative error of 4.3e-6 is enough to flip that entry.  A
# conforming libm is ~1e-16 and therefore safe, but -ffast-math, an approximate
# pow() on a minimal platform, or x87 excess-precision spills are not
# guaranteed to be.
#
# This generator computes the same integers with *exact* arithmetic and emits
# them as literals, so the tables are byte-identical everywhere by construction
# and no arithmetic remains at runtime to be perturbed.
#
# Exactness
# ---------
# The 2^x tables are resolved by exact big-integer comparison -- no floating
# point and no arbitrary-precision float library is involved, so the result is
# the provably correctly-rounded integer.  The sine table uses mpmath at 60
# decimal digits and every entry is certified: the script asserts that each
# value sits at least 1e-30 away from its rounding boundary, which is 27 orders
# of magnitude more slack than the working precision.
#
# Regenerate with:
#     python3 tools/dbopl_gentables.py > src/dosbox/dbopl_tables.h
# The output must be identical to the committed file; CI diffs the two.

import sys

try:
    from mpmath import mp, mpf, sin, pi, floor as mpfloor
except ImportError:
    sys.stderr.write("dbopl_gentables.py requires mpmath (pip install mpmath)\n")
    raise SystemExit(1)

mp.dps = 60

# Certification margin.  Every generated value must be at least this far from
# the boundary its rounding mode would tie on.  mp.dps = 60 gives ~1e-60
# relative working precision, so a 1e-30 margin is an enormous safety factor.
MARGIN = mpf('1e-30')


# --------------------------------------------------------------------------
# Exact 2^x rounding via big integers.
#
# round_half_up(V * 2^(-k/256)) is the unique integer n satisfying
#
#     (2n-1)^256 * 2^k  <=  (2V)^256  <  (2n+1)^256 * 2^k
#
# (raise both sides of  n - 1/2 <= V*2^(-k/256) < n + 1/2  to the 256th power
# and clear the negative exponent).  All quantities are Python ints, so the
# comparison is exact and the answer is the correctly-rounded value with no
# floating point anywhere.
# --------------------------------------------------------------------------
def exact_round_pow2(V, k):
    """Correctly-rounded round-half-up(V * 2**(-k/256)) using integers only."""
    lhs = (2 * V) ** 256

    def too_small(n):
        # True when V * 2^(-k/256) >= n + 1/2
        return lhs >= (2 * n + 1) ** 256 * 2 ** k

    lo, hi = 0, V + 1
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if too_small(mid - 1):
            lo = mid
        else:
            hi = mid - 1
    n = lo
    # Verify the bracket rather than trusting the search.
    assert (2 * n - 1) ** 256 * 2 ** k <= lhs, (V, k, n)
    assert lhs < (2 * n + 1) ** 256 * 2 ** k, (V, k, n)
    return n


def certify_round_half_up(value, result, what):
    """Assert `value` is far from the .5 tie point and rounds to `result`."""
    frac = value - mpfloor(value)
    assert abs(frac - mpf('0.5')) > MARGIN, "%s too close to tie: %s" % (what, value)
    assert int(mpfloor(value + mpf('0.5'))) == result, what


def certify_trunc(value, result, what):
    """Assert `value` is far from an integer boundary and truncates to `result`."""
    assert value >= 0, what
    frac = value - mpfloor(value)
    assert min(frac, 1 - frac) > MARGIN, "%s too close to boundary: %s" % (what, value)
    assert int(mpfloor(value)) == result, what


def emit(name, ctype, values, per_line, out):
    out.append("static const %s %s = {" % (ctype, name))
    for i in range(0, len(values), per_line):
        row = values[i:i + per_line]
        out.append("\t" + " ".join("%6d," % v for v in row))
    out.append("};")
    out.append("")


def main():
    out = []
    mul, wave_sin, wave_exp = [], [], []

    # MulTable[i] = round_half_up(2^(-1 + (255 - 8i)/256) * 2^MUL_SH)
    #             = round_half_up(65536 * 2^(-(1 + 8i)/256))
    for i in range(384):
        k = 1 + 8 * i
        n = exact_round_pow2(65536, k)
        certify_round_half_up(mpf(65536) * mp.power(2, mpf(-k) / 256), n,
                              "MulTable[%d]" % i)
        mul.append(n)

    # WaveTable[0x200 + i] = trunc(sin((i + 0.5) * pi / 512) * 4084)
    # The argument is (2i+1)/2048 of a full turn; the value is positive over
    # the whole range, so the C cast's truncate-toward-zero is a floor.
    for i in range(512):
        v = sin((mpf(i) + mpf('0.5')) * pi / 512) * 4084
        n = int(mpfloor(v))
        certify_trunc(v, n, "WaveTable[0x200+%d]" % i)
        wave_sin.append(n)

    # WaveTable[0x700 + i] = round_half_up(4085 * 2^(-(1 + 8i)/256))
    for i in range(256):
        k = 1 + 8 * i
        n = exact_round_pow2(4085, k)
        certify_round_half_up(mpf(4085) * mp.power(2, mpf(-k) / 256), n,
                              "WaveTable[0x700+%d]" % i)
        wave_exp.append(n)

    out.append("/* Generated by tools/dbopl_gentables.py -- do not edit by hand. */")
    out.append("/*")
    out.append(" * Fixed OPL wave and volume-multiplier tables.  These replace the")
    out.append(" * pow()/sin() loops that DBOPL::InitTables() used to run at startup, so")
    out.append(" * the AdLib output no longer depends on the host libm or on the")
    out.append(" * compiler's floating point settings.  Values are the exact")
    out.append(" * correctly-rounded integers; see the generator for the derivation and")
    out.append(" * the certification of every entry.")
    out.append(" */")
    out.append("")
    out.append("#ifndef DBOPL_TABLES_H")
    out.append("#define DBOPL_TABLES_H")
    out.append("")
    out.append("/* MulTable[i] = round(65536 * 2^(-(1 + 8i)/256)) */")
    emit("MulTableInit[384]", "Bit16u", mul, 8, out)
    out.append("/* WaveTable[0x200 + i] = trunc(4084 * sin((i + 0.5) * pi / 512)) */")
    emit("WaveSineInit[512]", "Bit16s", wave_sin, 8, out)
    out.append("/* WaveTable[0x700 + i] = round(4085 * 2^(-(1 + 8i)/256)) */")
    emit("WaveExpInit[256]", "Bit16s", wave_exp, 8, out)
    out.append("#endif /* DBOPL_TABLES_H */")

    sys.stdout.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
