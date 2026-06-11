import csv
import os
from collections import namedtuple

# A single playing card. Values mirror Card.set() in js/src/card.js:
#   rank = value % 13 + 1   (1=Ace .. 13=King)
#   suit = value // 13      (0=Clubs, 1=Diamonds, 2=Spades, 3=Hearts)
Card = namedtuple("Card", ["value", "rank", "suit"])

SUIT_NAMES = "CDSH"
RANK_NAMES = "0A23456789TJQK"


def make_card(value):
    """Build a Card from a 0..51 deck value (matches js/src/card.js set())."""
    return Card(value=value, rank=(value % 13) + 1, suit=value // 13)


def generate_board(game_number):
    """Deal a Klondike board for the given game number.

    Ports the FreeCell-style shuffle (shuffleFC) and the deal (resetGame) from
    js/src/solitaire.js so a game number produces the identical board.

    Returns a dict:
        {
            "tableau": [col0, ... col6],  # 7 columns, each bottom->top
                                          # (last element is the top/face-up card)
            "stock":   [Card, ...],       # 24 cards, in deal order
        }
    """
    # --- shuffleFC port (js/src/solitaire.js shuffleFC) ---
    deck = list(range(52))
    seed = game_number & 0xFFFFFFFF
    for i in range(52):
        cards_left = 52 - i
        # 32-bit LCG; masking reproduces the JS `| 0` int32 wrap.
        seed = (seed * 214013 + 2531011) & 0xFFFFFFFF
        rand = (seed >> 16) & 0x7FFF
        # Positive game numbers take the `dealNumber >= 0` branch in the JS.
        rect = rand % cards_left
        deck[rect], deck[cards_left - 1] = deck[cards_left - 1], deck[rect]

    # Map shuffled deck indices to Card values. suits maps FreeCell suit order
    # (C, D, H, S) to this codebase's suit ids (C=0, D=1, S=2, H=3).
    suits = [0, 1, 3, 2]
    cards = [None] * 52
    for i in range(52):
        rank = deck[i] >> 2
        suit = suits[deck[i] & 3]
        value = suit * 13 + rank
        cards[51 - i] = make_card(value)

    # --- deal port (js/src/solitaire.js resetGame) ---
    tableau = [[] for _ in range(7)]
    i = 0
    for j in range(7):
        # The up (face-up) card for column j is dealt first in the JS loop, but
        # column j already holds its face-down cards from earlier iterations, so
        # the up card ends up on top. Append it last to keep that order.
        up_card = cards[i]
        i += 1
        for k in range(j + 1, 7):
            tableau[k].append(cards[i])
            i += 1
        tableau[j].append(up_card)

    # Stock: JS pushes cards[51] down to cards[28] onto STOCK.
    stock = [cards[idx] for idx in range(51, 27, -1)]

    return {"tableau": tableau, "stock": stock}


def tableau_difficulty(board):
    """Estimate board difficulty from the blocking heuristic.

    For each card in each tableau column, count how many cards stacked on top of
    it have a higher rank (each must be moved to free this card). A King is
    treated as an Ace: it never blocks another card, but any card except an Ace
    can block it. A blocker of the same color as the card it blocks is weighted
    by SAME_COLOR_MULTIPLIER. Sum across all tableau columns.
    """
    KING = 13
    SAME_COLOR_MULTIPLIER = 1.5

    def effective_rank(card):
        return 1 if card.rank == KING else card.rank

    def is_red(card):
        return card.suit & 1  # suits: 0=C,1=D,2=S,3=H -> red are the odd ones

    total = 0.0
    for column in board["tableau"]:
        for idx, card in enumerate(column):
            card_rank = effective_rank(card)
            for b in column[idx + 1:]:
                if effective_rank(b) > card_rank:
                    total += SAME_COLOR_MULTIPLIER if is_red(b) == is_red(card) else 1.0
    return total

def stock_difficulty(board):
    """Estimate stock difficulty for draw-3 play.

    The stock is drawn from the top of the pile (the high-index end of the stock
    list) in groups of 3. Within each drawn group the third-drawn card lands on
    top and is accessible, with the earlier-drawn cards buried beneath it, so
    ordered from the top the group sits at positions 0 (top), 1, 2 (bottom).

    Aces and kings are the cards worth digging for. Each group scores a single
    value:
      - if the group holds no ace (rank 1) or king (rank 13), the value is 1;
      - otherwise scan from the top and take the position of the first ace/king
        that is buried under an ordinary (non-ace/non-king) card. An ace/king
        that sits on top with no ordinary card above it is accessible and does
        not count, so a group whose only aces/kings are all accessible scores 0.

    Examples written top -> bottom: AAA -> 0, KAA -> 0, K3A -> 2, 33K -> 2,
    3AK -> 1. Earlier groups are harder to clear, so each group's value is
    scaled by a position weight that eases linearly from EARLY_GROUP_WEIGHT
    (first group) down to LATE_GROUP_WEIGHT (last group). Sum the weighted
    values across every full group of 3.
    """
    ACE = 1
    KING = 13
    group_weight = 3.0
    iteration_multiplier = 0.7
    position_multiplier = 2.0


    def is_key(card):
        return card.rank == ACE or card.rank == KING

    # Draw order: top of the pile first (reverse of the bottom->top stock list).
    draw_order = list(reversed(board["stock"]))
    # Only full groups of 3 are scored.
    groups = [
        draw_order[g:g + 3]
        for g in range(0, len(draw_order), 3)
        if len(draw_order[g:g + 3]) == 3
    ]

    total = 0.0
    for i, (first, second, third) in enumerate(groups):


        # Ordered from the top (accessible) down to the bottom (buried).
        top_to_bottom = (third, second, first)
        if not any(is_key(card) for card in top_to_bottom):
            value = 1  # no ace/king worth digging for
        else:
            # Depth of the topmost ace/king that an ordinary card sits on; an
            # ace/king reached before any ordinary card is accessible (0).
            value = 0
            passed_ordinary = False
            for position, card in enumerate(top_to_bottom):
                if is_key(card):
                    if passed_ordinary:
                        value = position * position_multiplier
                        break
                else:
                    passed_ordinary = True

        total += value * group_weight 
        group_weight *= iteration_multiplier  # later groups are easier to clear
    return total

def get_games_from_csv(file_path):
    """Read game numbers from a CSV file (first column of each row).

    Skips blank rows and any non-integer cell, so an optional header is handled
    gracefully. Returns an empty list if the file does not exist.
    """
    if not os.path.exists(file_path):
        return []

    games = []
    with open(file_path, newline="") as f:
        for row in csv.reader(f):
            if not row:
                continue
            cell = row[0].strip()
            try:
                games.append(int(cell))
            except ValueError:
                continue  # header or non-numeric cell
    return games


def score_game(game_number):
    """Return a game's (tableau_diff, stock_diff) difficulty components."""
    board = generate_board(game_number)
    return tableau_difficulty(board), stock_difficulty(board)


def estimate_difficulty(game_number):
    """Combined difficulty estimate: tableau blocking plus stock blocking."""
    tableau_diff, stock_diff = score_game(game_number)
    return tableau_diff + stock_diff


def sort_by_difficulty(games):
    """Score each game once and return (game, tableau_diff, stock_diff) rows
    sorted ascending by combined difficulty (easiest first)."""
    scored = [(game, *score_game(game)) for game in games]
    scored.sort(key=lambda row: row[1] + row[2])
    return scored


def write_levels_js(file_path, scored_games):
    """Overwrite a JavaScript module exporting the game numbers in ascending
    difficulty order as `all_games`."""
    with open(file_path, "w") as f:
        f.write("export const all_games = [\n")
        for game, *_ in scored_games:
            f.write(f"\t{game},\n")
        f.write("];\n")


def annotate_csv(in_path, out_path):
    """Copy a results CSV, appending `tableau_diff` and `stock_diff` columns
    computed from the game number in the first column.

    All original columns are preserved. Rows are sorted ascending by
    `constraint`, then `tableau_diff`, then `stock_diff`. The first non-blank row
    is treated as the header; difficulty values are rounded to 4 decimal places.
    """
    with open(in_path, newline="") as src:
        rows = [row for row in csv.reader(src) if row]  # drop blank lines
    if not rows:
        return  # nothing to write

    header, *data = rows
    constraint_idx = header.index("constraint") if "constraint" in header else None

    scored = []
    for row in data:
        tableau_diff, stock_diff = score_game(int(row[0]))
        constraint = row[constraint_idx] if constraint_idx is not None else ""
        scored.append((constraint, tableau_diff, stock_diff, row))
    # Sort by constraint, then tableau_diff, then stock_diff (all ascending).
    scored.sort(key=lambda r: (r[0], r[1], r[2]))

    out_rows = [header + ["tableau_diff", "stock_diff"]]
    out_rows += [
        row + [round(tableau_diff, 4), round(stock_diff, 4)]
        for _, tableau_diff, stock_diff, row in scored
    ]

    with open(out_path, "w", newline="") as dst:
        csv.writer(dst).writerows(out_rows)


def process_directory(input_dir, output_dir):
    """Annotate every .csv in input_dir with difficulty columns and write the
    result to output_dir under the same filename. Returns the filenames done."""
    os.makedirs(output_dir, exist_ok=True)
    names = sorted(n for n in os.listdir(input_dir) if n.lower().endswith(".csv"))
    for name in names:
        annotate_csv(os.path.join(input_dir, name), os.path.join(output_dir, name))
        print(f"wrote {os.path.join(output_dir, name)}")
    return names


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    input_dir = os.path.join(here, "input")
    output_dir = os.path.join(here, "output")
    done = process_directory(input_dir, output_dir)
    if not done:
        print(f"no CSV files found in {input_dir}")
