#!/usr/bin/env python3
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)

"""Writes oracle.txt, the cases numbers_oracle_test.cpp checks json_schema's numbers against.

Usage: oracle.py OUTPUT

The expected results come from Python's decimal and fractions, which compute exactly and share no
code with json_schema: a double is the Decimal of repr(), its shortest round-trip text; a
decimal64_t or decimal128_t literal is the Decimal its IEEE 754 context gives (16 or 34 digits,
ties to even, the format's exponents); a comparison compares Decimals, which is exact; and x is a
multiple of y when the Fraction x / y has the denominator 1. The cases are drawn from a seeded
random.Random, so the output is the same on every run.

Each line is one case, its words separated by one space; a line that starts with # is a comment.
An operand is a kind and a text: d, a double by the 16 hexadecimal digits of its bits; i, an
int64_t; u, a uint64_t; s, a decimal64_t literal; S, a decimal128_t literal.

    x KIND TEXT VALUE      the operand is VALUE exactly, or - when the Number refuses the literal
    c KIND X KIND Y ORDER  X compared with Y is ORDER, one of < = >
    m KIND X KIND Y BOOL   X is a multiple of Y (1) or not (0); Y is greater than zero
    n s|S BITS RESULT      what the loader makes of the double BITS for that Number: i VALUE, an
                           int64_t, u VALUE, a uint64_t, N VALUE, the Number, or -, nothing

A VALUE is [-]COEFFICIENT e EXPONENT without spaces, the coefficient without trailing zeros; zero
is 0e0, or -0e0 for a negative zero.

Every literal of a c or an m line is one its Number holds exactly, so those lines check the
comparison and the division alone; the x lines check the reading of literals: of any number of
digits, ties at the digit past the precision exact or decided far after it, runs of zeros before
and after the digits, zeros with exponents past any type's, and the ends of each type, where a
number rounds to zero or past the largest. An exponent of more than 18 digits, which Python's
decimal cannot hold, is left to numbers_test.cpp.
"""

from __future__ import annotations

import math
import random
import struct
import sys
from decimal import ROUND_HALF_EVEN, Context, Decimal
from fractions import Fraction

SEED = 20261009

D64 = Context(prec=16, Emax=384, Emin=-383, rounding=ROUND_HALF_EVEN, clamp=1, traps=[])
D128 = Context(prec=34, Emax=6144, Emin=-6143, rounding=ROUND_HALF_EVEN, clamp=1, traps=[])
# Arithmetic on the cases' values, which never rounds them.
EXACT = Context(prec=20000, Emax=100000, Emin=-100000, traps=[])
CONTEXTS = {'s': D64, 'S': D128}
DIGITS = {'s': 16, 'S': 34}

INT64_MIN = -(2 ** 63)
INT64_MAX = 2 ** 63 - 1
UINT64_MAX = 2 ** 64 - 1


def double_of(bits: int) -> float:
    """The double whose bits are bits."""
    return struct.unpack('<d', struct.pack('<Q', bits))[0]


def bits_of(d: float) -> int:
    """The bits of the double d."""
    return struct.unpack('<Q', struct.pack('<d', d))[0]


def hex_of(d: float) -> str:
    """The text of a d operand."""
    return f'{bits_of(d):016x}'


def text_of(value: Decimal) -> str:
    """A VALUE: [-]COEFFICIENT e EXPONENT, the coefficient without trailing zeros."""
    sign, digits, exponent = value.as_tuple()
    assert isinstance(exponent, int)
    coefficient = int(''.join(map(str, digits)) or '0')
    minus = '-' if sign else ''
    if coefficient == 0:
        return f'{minus}0e0'
    while coefficient % 10 == 0:
        coefficient //= 10
        exponent += 1
    return f'{minus}{coefficient}e{exponent}'


def literal(kind: str, text: str) -> Decimal | None:
    """The Number the literal text gives, or None when the Number cannot hold it: too large, or
    not zero and rounded to zero."""
    context = CONTEXTS[kind]
    context.clear_flags()
    value = context.create_decimal(text)
    if not value.is_finite():
        return None
    if value.is_zero() and not Decimal(text).is_zero():
        return None
    return value


def is_exact(kind: str, text: str) -> bool:
    """Whether the Number holds the literal text without rounding."""
    value = literal(kind, text)
    return value is not None and value == Decimal(text)


def long_literals() -> list[str]:
    """Literals past each type's digits and exponents, drawn from a random.Random of their own."""
    draw = random.Random(SEED + 1)
    texts: list[str] = []
    # Digits past each type's precision, at the exponents of either type.
    for _ in range(300):
        count = draw.randint(18, 70)
        coefficient = draw.randrange(10 ** (count - 1), 10 ** count)
        low, high = (-420, 400) if draw.random() < 0.7 else (-6250, 6170)
        texts.append(scientific(coefficient, draw.randint(low, high), draw.random() < 0.2))
    # A tie at the digit past the precision, exact, or decided by a digit far after it.
    for digits, low, high in ((16, -400, 380), (34, -6200, 6100)):
        for _ in range(60):
            head = draw.randrange(10 ** (digits - 1), 10 ** digits)
            run = draw.randint(1, 40)
            tail = draw.choice(['5', '5' + '0' * run + '1', '4' + '9' * run, '5' + '0' * run])
            texts.append(f'{head}{tail}e{draw.randint(low, high)}')
    # Runs of zeros before the digits and after them, the exponent making up for them.
    for _ in range(60):
        digits = str(draw.randrange(1, 10 ** draw.randint(1, 40)))
        zeros = draw.randint(20, 1000)
        exponent = draw.randint(-300, 300)
        texts.append(f'0.{"0" * zeros}{digits}e{exponent + zeros}')
        texts.append(f'{digits}{"0" * zeros}e{exponent - zeros}')
    # Zeros, with exponents past any type's.
    texts += ['0e-100000', '-0e99999999999', '0e2147483648', '0.000e+999999999999999',
              '-0.0e-999999999999999', f'0.{"0" * 500}e100']
    # The ends: half the smallest subnormal, which rounds to zero, and rounding past the largest.
    texts += [
        '5e-399', '-5e-399', '4.9999999999999999999999e-399', '5.0000000000000000000001e-399',
        '5e-6177', f'5.{"0" * 40}1e-6177', f'4.{"9" * 40}e-6177', '9.9999999999999995e384',
        '9.99999999999999949999999999e384', '-9.9999999999999995000000000001e384',
        f'9.{"9" * 33}5e6144', f'9.{"9" * 33}49999e6144',
    ]
    return texts


class Operand:
    """An operand of a line: its kind, its text, and its exact value."""

    def __init__(self, kind: str, text: str, value: Decimal) -> None:
        self.kind = kind
        self.text = text
        self.value = value

    def __str__(self) -> str:
        return f'{self.kind} {self.text}'


def double(d: float) -> Operand:
    """A d operand: the Decimal of the double's shortest round-trip text."""
    return Operand('d', hex_of(d), Decimal(repr(d)))


def integer(value: int) -> Operand:
    """An i operand when value fits int64_t, else a u operand."""
    kind = 'i' if INT64_MIN <= value <= INT64_MAX else 'u'
    assert value <= UINT64_MAX
    return Operand(kind, str(value), Decimal(value))


def number(kind: str, text: str) -> Operand:
    """An s or S operand, which its Number holds exactly."""
    assert is_exact(kind, text), text
    return Operand(kind, text, Decimal(text))


def order(x: Decimal, y: Decimal) -> str:
    """ORDER: x compared with y."""
    if x < y:
        return '<'
    return '=' if x == y else '>'


def is_multiple(x: Decimal, y: Decimal) -> bool:
    """Whether x is an integer times y."""
    return (Fraction(x) / Fraction(y)).denominator == 1


def scientific(coefficient: int, exponent: int, negative: bool = False) -> str:
    """The literal coefficient e exponent, written as JSON writes a number."""
    return f"{'-' if negative else ''}{coefficient}e{exponent}"


class Generator:
    """The cases, drawn from one seeded random.Random."""

    def __init__(self) -> None:
        self.random = random.Random(SEED)
        self.lines: list[str] = []

    def emit(self, line: str) -> None:
        """Adds one line."""
        self.lines.append(line)

    def finite_double(self) -> float:
        """A double drawn uniformly over the bits of the finite ones."""
        while True:
            d = double_of(self.random.getrandbits(64))
            if math.isfinite(d):
                return d

    def decimal_double(self) -> float:
        """A double near a short decimal: up to 17 digits, any exponent of a double."""
        while True:
            digits = self.random.randint(1, 17)
            coefficient = self.random.randrange(10 ** (digits - 1), 10 ** digits)
            exponent = self.random.randint(-340, 308 - digits + 1)
            d = float(scientific(coefficient, exponent, self.random.random() < 0.3))
            if math.isfinite(d) and d != 0.0:
                return d

    def large_integral_double(self) -> float:
        """An integral double between 2^52 and 2^65, where int64_t and uint64_t end."""
        sign = -1.0 if self.random.random() < 0.3 else 1.0
        return sign * math.ldexp(float(self.random.getrandbits(53) | (1 << 52)),
                                 self.random.randint(0, 12))

    def short_literal(self, kind: str, low: int, high: int, digits: int) -> Operand:
        """A literal its Number holds: up to digits digits, the exponent in [low, high]."""
        while True:
            count = self.random.randint(1, digits)
            coefficient = self.random.randrange(10 ** (count - 1), 10 ** count)
            text = scientific(coefficient, self.random.randint(low, high),
                              self.random.random() < 0.2)
            if is_exact(kind, text):
                return number(kind, text)

    def doubles(self) -> list[float]:
        """The doubles every kind of line draws from: the edges, then random ones."""
        edges = [
            0.0, -0.0, 5e-324, -5e-324, math.nextafter(2.2250738585072014e-308, 0.0),
            2.2250738585072014e-308, sys.float_info.max, -sys.float_info.max, 1.0, -1.0, 0.1, 0.2,
            0.3, 0.1 + 0.2, 1.1, 0.0075, 0.00751, 4.5, 1e308, 1e-300, 1e-7, 1e15, 1e16, 1e17,
            1e21, 1e22, 1e23, 123456789012345680000.0, 2.0 ** 53, 2.0 ** 53 + 2.0, 2.0 ** 60,
            2.0 ** 63, -(2.0 ** 63), 2.0 ** 64, math.nextafter(2.0 ** 64, 0.0),
            math.nextafter(2.0 ** 63, 0.0), math.nextafter(-(2.0 ** 63), 0.0),
            9007199254740993.0, 0.30000000000000004, 1.7976931348623157e308,
        ]
        drawn = [self.finite_double() for _ in range(200)]
        drawn += [self.decimal_double() for _ in range(200)]
        drawn += [self.large_integral_double() for _ in range(60)]
        return edges + drawn

    def exact_values(self, doubles: list[float]) -> None:
        """x lines: each double, and literals that round, are refused or are held whole."""
        self.emit('# The exact value of a double, the decimal of its shortest text.')
        for d in doubles:
            operand = double(d)
            self.emit(f'x {operand} {text_of(operand.value)}')
        self.emit('# The value of a literal, rounded to its Number, or refused.')
        texts = [
            '0', '-0', '0.0', '-0e5', '0e-1000', '1e384', '9.999999999999999e384', '1e385',
            '-1e385', '9.9999999999999995e384', '9.9999999999999994e384', '1e-398', '1e-399',
            '-4e-400', '6e-399', '1.5e-398', '2.5e-398', '1.25e-397', '1e6144', '1e6145',
            '1e-6176', '1e-6177', '9.999999999999999999999999999999999e6144',
            '9.9999999999999999999999999999999995e6144', '0.30000000000000004',
            '0.30000000000000005', '0.30000000000000015', '12345678901234565', '12345678901234575',
            '18446744073709551615', '18446744073709551616', '1.7976931348623157e308',
        ]
        texts += [repr(d) for d in doubles]
        for _ in range(200):
            count = self.random.randint(1, 35)
            coefficient = self.random.randrange(10 ** (count - 1), 10 ** count)
            texts.append(scientific(coefficient, self.random.randint(-420, 400)))
        for _ in range(60):
            count = self.random.randint(1, 35)
            coefficient = self.random.randrange(10 ** (count - 1), 10 ** count)
            texts.append(scientific(coefficient, self.random.randint(-6220, 6150)))
        texts += long_literals()
        for text in texts:
            for kind in ('s', 'S'):
                value = literal(kind, text)
                self.emit(f'x {kind} {text} {"-" if value is None else text_of(value)}')

    def comparisons(self, doubles: list[float]) -> None:
        """c lines: doubles, integers and literals against each other and their neighbours."""
        self.emit('# Comparisons.')
        cases: list[tuple[Operand, Operand]] = []
        for d in doubles:
            x = double(d)
            cases.append((x, double(self.random.choice(doubles))))
            shortest = Decimal(repr(d))
            # The double's own text, and the same less or more by one in its last digit.
            for kind in ('s', 'S'):
                sign, digits, exponent = shortest.as_tuple()
                assert isinstance(exponent, int)
                coefficient = int(''.join(map(str, digits)))
                for delta in (-1, 0, 1):
                    text = scientific(coefficient + delta, exponent, sign == 1)
                    if coefficient + delta >= 0 and is_exact(kind, text):
                        cases.append((x, number(kind, text)))
            # The double's binary value, which decimal128_t holds when it has 34 digits at most:
            # the double is its shortest text, never that.
            binary = Decimal(d)
            if len(binary.as_tuple().digits) <= 34 and is_exact('S', str(binary)):
                cases.append((x, number('S', str(binary))))
            truncated = D128.create_decimal(binary)
            if truncated.is_finite() and is_exact('S', str(truncated)):
                cases.append((x, number('S', str(truncated))))
        boundaries = [INT64_MIN, INT64_MIN + 1, -1, 0, 1, 2 ** 53, 2 ** 53 + 1, INT64_MAX,
                      2 ** 63, 2 ** 63 + 1, 2 ** 64 - 2049, 2 ** 64 - 2048, UINT64_MAX]
        for value in boundaries:
            for d in (float(value), math.nextafter(float(value), math.inf),
                      math.nextafter(float(value), -math.inf)):
                cases.append((integer(value), double(d)))
                cases.append((double(d), integer(value)))
        for _ in range(200):
            value = self.random.randint(INT64_MIN, UINT64_MAX)
            cases.append((integer(value), double(float(value))))
            cases.append((integer(value), integer(self.random.randint(INT64_MIN, UINT64_MAX))))
            kind = self.random.choice('sS')
            cases.append((integer(value), self.short_literal(kind, -5, 5, DIGITS[kind])))
        for _ in range(200):
            kind = self.random.choice('sS')
            other = self.random.choice('sS')
            x = self.short_literal(kind, -420, 400, DIGITS[kind])
            cases.append((x, self.short_literal(other, -420, 400, DIGITS[other])))
            sign, digits, exponent = x.value.as_tuple()
            assert isinstance(exponent, int)
            shifted = scientific(int(''.join(map(str, digits))) * 10, exponent - 1, sign == 1)
            if is_exact('S', shifted):
                cases.append((x, number('S', shifted)))
        for x, y in cases:
            self.emit(f'c {x} {y} {order(x.value, y.value)}')

    def multiples(self, doubles: list[float]) -> None:
        """m lines: multiples and near misses, large divisors and large quotients."""
        self.emit('# Multiples.')
        cases: list[tuple[Operand, Operand]] = []
        for _ in range(300):
            kind = self.random.choice('sS')
            y = self.short_literal(kind, -12, 3, 6)
            y.value = abs(y.value)
            y.text = y.text.lstrip('-')
            k = self.random.randint(0, 10 ** self.random.randint(1, 9))
            product = EXACT.multiply(y.value, k)
            near = EXACT.add(product, EXACT.scaleb(y.value, -self.random.randint(1, 4)))
            for product in (product, near):
                d = float(product)
                if math.isfinite(d) and Decimal(repr(d)) == product:
                    cases.append((double(d), y))
                if -2 ** 63 <= product <= UINT64_MAX and product == product.to_integral_value():
                    cases.append((integer(int(product)), y))
                if is_exact('S', str(product)):
                    cases.append((number('S', str(product)), y))
        for _ in range(250):
            # A large divisor, of up to 34 digits, and its multiples, of up to 34 digits too.
            count = self.random.randint(17, 30)
            coefficient = self.random.randrange(10 ** (count - 1), 10 ** count)
            exponent = self.random.randint(-400, 300)
            y = number('S', scientific(coefficient, exponent))
            k = self.random.randint(1, 10 ** (34 - count))
            shift = self.random.randint(0, 3000)
            cases.append((number('S', scientific(coefficient * k, exponent)), y))
            cases.append((number('S', scientific(coefficient * k + 1, exponent)), y))
            if is_exact('S', scientific(coefficient * k, exponent + shift)):
                cases.append((number('S', scientific(coefficient * k, exponent + shift)), y))
            cases.append((double(self.random.choice(doubles)), y))
        for _ in range(300):
            # Large quotients: any double, or a literal of a large exponent, by a small divisor.
            kind = self.random.choice('sS')
            y = self.short_literal(kind, -400, 20, 4)
            y.value = abs(y.value)
            y.text = y.text.lstrip('-')
            cases.append((double(self.random.choice(doubles)), y))
            x_kind = self.random.choice('sS')
            cases.append((self.short_literal(x_kind, -400, 380, DIGITS[x_kind]), y))
        for x in (0, 1, 2, 3, 7, 10, 12391239123, INT64_MIN, INT64_MAX, 2 ** 63, UINT64_MAX):
            for y in (1, 2, 3, 7, 6700417, 65537, 2 ** 32, INT64_MAX, 2 ** 63, UINT64_MAX):
                cases.append((integer(x), integer(y)))
        for x, y in cases:
            if y.value > 0:
                self.emit(f'm {x} {y} {int(is_multiple(x.value, y.value))}')

    def loaded_doubles(self, doubles: list[float]) -> None:
        """n lines: the loader's number for a double, for each Number."""
        self.emit("# A schema's double, as the loader keeps it.")
        for d in doubles:
            shortest = Decimal(repr(d))
            for kind in ('s', 'S'):
                if shortest == shortest.to_integral_value() and INT64_MIN <= shortest <= INT64_MAX:
                    result = f'i {text_of(Decimal(int(shortest)))}'
                elif shortest == shortest.to_integral_value() and 0 <= shortest <= UINT64_MAX:
                    result = f'u {text_of(Decimal(int(shortest)))}'
                else:
                    value = literal(kind, repr(d))
                    result = '-' if value is None else f'N {text_of(value)}'
                self.emit(f'n {kind} {hex_of(d)} {result}')

    def run(self) -> list[str]:
        """Every line, in order."""
        doubles = self.doubles()
        self.emit('# Written by oracle.py; never edited by hand.')
        self.exact_values(doubles)
        self.comparisons(doubles)
        self.multiples(doubles)
        self.loaded_doubles(doubles)
        return self.lines


def main() -> int:
    """Writes the cases at the path given."""
    if len(sys.argv) != 2:
        print('usage: oracle.py OUTPUT', file=sys.stderr)
        return 2
    with open(sys.argv[1], 'w', encoding='ascii', newline='\n') as output:
        for line in Generator().run():
            output.write(line + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
