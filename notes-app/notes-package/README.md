# Notes: Markdown + maths reader for the NumWorks calculator

A native external app (.nwa) for stock Epsilon that shows notes written in
Markdown, with LaTeX-style maths rendered the way the calculator draws it:
real fractions with a bar, roots with a radical and overbar, raised
exponents and lowered indices, brackets sized to their content, sums and
integrals with limits, Greek letters and the usual relation symbols.

## Two builds, pick one

- `notes.nwa`: reads notes from Workshop scripts only. The uploader asks for
  nothing else.
- `notes-data.nwa`: additionally takes a notes file right on the uploader
  page ("Select a data file"). Drop a plain text / Markdown file there; several
  notes in one file are separated by lines starting with `=== Title`
  (`my-notes.md` in this folder is a starter). To change the notes, install
  again with the new file. Workshop scripts are still read as well.

## Install the app

1. Plug the calculator in and leave it on "The calculator is connected".
2. Open https://my.numworks.com/apps in Chrome or Edge and add the `.nwa`
   (for `notes-data.nwa`, also select your notes file in the card).
3. "Notes" appears at the end of the Home screen. With no notes found it shows
   a built-in Guide that doubles as the manual.

Resets remove external apps (crashes do not); just install again.

## Write notes in the Workshop (no files needed)

Notes are ordinary Python scripts, so the NumWorks Workshop syncs them like
anything else:

1. On https://my.numworks.com, create a script named `note_<anything>.py`
   (the `note` prefix is how the app recognises it; `something_md.py` works too).
2. Paste your Markdown as the script's content. If you prefer the script to
   stay valid Python, wrap the whole text in triple quotes: the app strips them.
3. Sync to the calculator. Open Notes: the script shows up under its name.

Notes share the 42 kB script storage with your Python scripts. Up to 32 notes
are listed.

## Markdown that renders

    # Heading 1        ## Heading 2        ### Heading 3
    **bold**  *italic*  `code`
    - bullets (indent two spaces to nest)      1. numbered items
    > quote
    ---                 (horizontal rule)
    ```                 (code block, one line per row)
    $inline maths$      $$display maths$$

Paragraphs wrap to the screen width. A blank line separates paragraphs.

## Maths that renders

    \frac{a}{b}  \sqrt{x}  \sqrt[3]{x}  x^{2}  x_{i}  x_{1,2}^{n}
    ( ... ) [ ... ] \left( ... \right)  \abs{x}  \binom{n}{k}
    \sum_{k=1}^{n}  \prod  \int_0^1  \lim_{x \to 0}
    \vec{v} \bar{x} \hat{y} \dot{x} \tilde{x}  \text{words}
    \alpha \beta \gamma \delta \epsilon \zeta \eta \theta \lambda \mu \pi
    \rho \sigma \chi \omega \Gamma \Delta \Sigma \Omega
    \cdot \times \div \pm \infty \to \leq \geq \neq \approx \ldots \degree
    \quad \qquad (also split a long display formula onto several lines)
    \\ (line break inside display maths)

Letters the NumWorks font does not have (phi, tau, kappa, nu, xi, psi, ...)
are shown by name. A display formula wider than the screen is retried in
the small font.

## Tables

    | x | f(x) | f'(x) |
    |---|:----:|------:|
    | 1 | $e$  | $e$   |

Rows start with `|`. Rows above the `|---|` line form the header (bold, grey
background); `:---:` centres a column and `---:` right-aligns it. Cells may
contain inline maths, and `|` inside `$...$` does not split a cell. A table
wider than the screen is retried in the small font, then clipped with a `>`
marker. Up to 8 columns and 60 rows per table.

## Keys in the reader

- Up / Down: scroll one line
- Right / Left (or Shift+Down / Shift+Up): next / previous page
- OK or EXE: **Contents**, the list of headings; pick one to jump there
- Any letter or digit (Alpha + key, or Shift+Alpha to lock): **search** the
  note. Matches are highlighted; EXE or Down goes to the next, Up to the
  previous, Backspace edits, Back closes the search bar
- Toolbox: switch between small and large text
- Back: return to the list; the reading position of each note is remembered
  until you leave the app

## Building from source

    make PLATFORM=device build      # -> output/device/notes.nwa

The simulator build (`make PLATFORM=simulator SIMULATOR=path/to/epsilon.bin build`)
can be run with `--nwb output/linux/notes.nwb --nwb-external-data samples/test.md`
to preview notes without a calculator.
