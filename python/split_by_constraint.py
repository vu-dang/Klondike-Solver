#!/usr/bin/env python3
"""Combine solver CSVs and split them into one file per constraint value.

Reads every *.csv in the input directory (columns: game, moves, time_us,
num_recycles, max_states, constraint), combines all their rows, drops unsolved
deals (moves == 0), groups the rows by their `constraint` value, sorts each
group by num_recycles, then moves, then max_states, and writes one file per
constraint named `<constraint>.csv` into the output directory.

It also writes a JavaScript file per constraint (using output/level.js as the
template: `export const all_games = [ ... ];`) named after the constraint's
difficulty level: beginner -> level1.js, simple -> level2.js,
expert -> level3.js, unconstrained -> level4.js.

Usage:
    python split_by_constraint.py [input_dir] [output_dir]

Defaults to python/input and python/output next to this script.
"""
import csv
import glob
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_INPUT_DIR = os.path.join(SCRIPT_DIR, "input")
DEFAULT_OUTPUT_DIR = os.path.join(SCRIPT_DIR, "output")

SORT_KEYS = ("num_recycles", "moves", "max_states")

# Maps a constraint value to the JavaScript level file it should be written to.
CONSTRAINT_TO_LEVEL_JS = {
    "beginner": "level1.js",
    "simple": "level2.js",
    "expert": "level3.js",
    "unconstrained": "level4.js",
}


def to_int(value):
    """Best-effort int conversion so sorting is numeric, not lexical."""
    try:
        return int(value)
    except (TypeError, ValueError):
        return 0


def main(argv):
    input_dir = argv[1] if len(argv) > 1 else DEFAULT_INPUT_DIR
    output_dir = argv[2] if len(argv) > 2 else DEFAULT_OUTPUT_DIR

    if not os.path.isdir(input_dir):
        sys.exit(f"Input directory not found: {input_dir}")

    csv_paths = sorted(glob.glob(os.path.join(input_dir, "*.csv")))
    if not csv_paths:
        sys.exit(f"No .csv files found in {input_dir}")

    # Combine the rows from every input file, dropping unsolved deals (moves == 0).
    fieldnames = None
    groups = {}
    total_read = 0
    for path in csv_paths:
        with open(path, newline="") as f:
            reader = csv.DictReader(f)
            if reader.fieldnames is None or "constraint" not in reader.fieldnames:
                sys.exit(f"{path}: CSV must have a header row with a 'constraint' column.")
            if fieldnames is None:
                fieldnames = reader.fieldnames
            for row in reader:
                total_read += 1
                if to_int(row["moves"]) == 0:
                    continue
                groups.setdefault(row["constraint"], []).append(row)
        print(f"Read {os.path.basename(path)}")

    os.makedirs(output_dir, exist_ok=True)

    total_written = 0
    for constraint, group in sorted(groups.items()):
        group.sort(key=lambda r: tuple(to_int(r[k]) for k in SORT_KEYS))
        out_path = os.path.join(output_dir, f"{constraint}.csv")
        with open(out_path, "w", newline="") as out:
            writer = csv.DictWriter(out, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(group)
        total_written += len(group)
        print(f"Wrote {len(group):>5} rows -> {out_path}")

        # Write the matching level<N>.js file with the sorted game numbers.
        level_js = CONSTRAINT_TO_LEVEL_JS.get(constraint)
        if level_js is not None:
            js_path = os.path.join(output_dir, level_js)
            with open(js_path, "w") as js:
                js.write("export const all_games = [\n")
                for row in group:
                    js.write(f"\t{to_int(row['game'])},\n")
                js.write("];\n")
            print(f"Wrote {len(group):>5} games -> {js_path}")

    print(f"\nCombined {len(csv_paths)} file(s): {total_read} rows read, "
          f"{total_written} written, {total_read - total_written} dropped (0 moves).")


if __name__ == "__main__":
    main(sys.argv)
