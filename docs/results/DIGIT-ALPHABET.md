# The extended digit alphabet, and right-to-left rendering

Values in this repository are written in an extended digit alphabet (digit
values map to symbols):

| values | symbols |
|--------|---------|
| 0–9 | `0`–`9` |
| 10–35 | `A`–`Z` |
| 36–48 | Greek lowercase `α β γ δ ε ζ η θ ι κ λ μ ν` (α=36 … ν=48) |
| 49–70 | Hebrew `א‎ ב‎ ג‎ ד‎ ה‎ ו‎ ז‎ ח‎ ט‎ י‎ כ‎ ל‎ מ‎ נ‎ ס‎ ע‎ פ‎ צ‎ ק‎ ר‎ ש‎ ת‎` (א=49 … ת=70; plain consonants — no niqqud, no final forms) |

Most significant digit first, as usual.

## Right-to-left digits (bases > 49)

Hebrew is a right-to-left script, so a Hebrew digit dropped naively into a
value string triggers Unicode bidirectional reordering — adjacent Hebrew
letters display reversed and the number falls out of place-value order.
The notation therefore mandates: **every Hebrew digit is immediately
followed by U+200E LEFT-TO-RIGHT MARK** in rendered values. Each letter
then forms a singleton bidi run, so logical order = display order, in code
spans and plain text alike, with no HTML wrappers needed.

Demonstration (digit values 53 down to 46, crossing the Hebrew/Greek
boundary — both lines contain the same logical digit sequence `ה ד ג ב א
ν μ λ`):

- naive (renders out of order): `הדגבאνμλ`
- with LRM after each Hebrew letter (correct left-to-right place order): `ה‎ד‎ג‎ב‎א‎νμλ`

Machine consumers must **strip U+200E before decoding** (the b-file
validator, `scripts/validate_bfile.py`, does exactly this).

The alphabet is total through digit value 70 (base 71); values above base 71
would extend it. Digits 49+ first occur at base 50.
