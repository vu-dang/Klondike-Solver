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
    list) in groups of 3. Within each drawn group the third card lands on top and
    is accessible, while the earlier cards are buried beneath it:
      - the 1st-drawn card is blocked by the 2nd and 3rd,
      - the 2nd-drawn card is blocked by the 3rd.
    A card only counts as blocked when the card on top of it has a higher rank.
    Kings (rank 13) are ignored as blockers. A blocker of the same color as the
    card it blocks is weighted by SAME_COLOR_MULTIPLIER. Earlier groups are
    harder to clear, so each group's contribution is scaled by a position weight
    that decreases linearly from EARLY_GROUP_WEIGHT (first group) down to
    LATE_GROUP_WEIGHT (last group). Tally those blocked cards across every
    group of 3.
    """
    KING = 13
    SAME_COLOR_MULTIPLIER = 2.5
    EARLY_GROUP_WEIGHT = 2.0
    LATE_GROUP_WEIGHT = 1.0

    def is_red(card):
        return card.suit & 1  # suits: 0=C,1=D,2=S,3=H -> red are the odd ones

    def block_value(card, top):
        if top.rank > card.rank and top.rank != KING:
            return SAME_COLOR_MULTIPLIER if is_red(top) == is_red(card) else 1.0
        return 0.0

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
        # Position weight: EARLY for the first group, easing to LATE for the last.
        if len(groups) > 1:
            t = i / (len(groups) - 1)
            position_weight = EARLY_GROUP_WEIGHT + t * (LATE_GROUP_WEIGHT - EARLY_GROUP_WEIGHT)
        else:
            position_weight = EARLY_GROUP_WEIGHT
        # 1st blocked by 2nd and/or 3rd; 2nd blocked by 3rd (higher rank on top).
        group_value = (
            block_value(first, second)
            + block_value(first, third)
            + block_value(second, third)
        )
        total += position_weight * group_value
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


def estimate_difficulty(game_number):
    """Combined difficulty estimate: tableau blocking plus stock blocking."""
    board = generate_board(game_number)
    return tableau_difficulty(board) + stock_difficulty(board)


def sort_by_difficulty(games):
    """Score each game once and return (game_number, difficulty) pairs sorted
    ascending by difficulty (easiest first)."""
    scored = [(game, estimate_difficulty(game)) for game in games]
    scored.sort(key=lambda pair: pair[1])
    return scored


def write_levels_js(file_path, scored_games):
    """Overwrite a JavaScript module exporting the game numbers in ascending
    difficulty order as `all_games`."""
    with open(file_path, "w") as f:
        f.write("export const all_games = [\n")
        for game, _ in scored_games:
            f.write(f"\t{game},\n")
        f.write("];\n")


if __name__ == "__main__":
    games = get_games_from_csv("python/games.csv")
    scored_games = sort_by_difficulty(games)
    # Write sorted games to a new CSV file as `game_number,difficulty`.
    with open("python/sorted_games.csv", "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["game_number", "difficulty"])
        for game, difficulty in scored_games:
            writer.writerow([game, difficulty])
    # Also export the game numbers (ascending difficulty) as a JS module.
    write_levels_js("python/level.js", scored_games)
