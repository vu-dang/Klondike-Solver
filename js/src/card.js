export const EMPTY = 0;
export const ACE = 1;
export const TWO = 2;
export const THREE = 3;
export const FOUR = 4;
export const FIVE = 5;
export const SIX = 6;
export const SEVEN = 7;
export const EIGHT = 8;
export const NINE = 9;
export const TEN = 10;
export const JACK = 11;
export const QUEEN = 12;
export const KING = 13;

export const CLUBS = 0;
export const DIAMONDS = 1;
export const SPADES = 2;
export const HEARTS = 3;
export const SUIT_NONE = 255;

export class Card {
	constructor() {
		this.Rank = EMPTY;
		this.Suit = SUIT_NONE;
		this.IsRed = 0;
		this.IsOdd = 0;
		this.Foundation = 0;
		this.Value = 0;
	}

	clear() {
		this.Rank = EMPTY;
		this.Suit = SUIT_NONE;
	}

	set(value) {
		this.Value = value;
		this.Rank = (value % 13) + 1;
		this.Suit = (value / 13) | 0;
		this.IsRed = this.Suit & 1;
		this.IsOdd = this.Rank & 1;
		this.Foundation = this.Suit + 9;
	}

	static empty() {
		const c = new Card();
		c.clear();
		return c;
	}

	copyFrom(other) {
		this.Rank = other.Rank;
		this.Suit = other.Suit;
		this.IsRed = other.IsRed;
		this.IsOdd = other.IsOdd;
		this.Foundation = other.Foundation;
		this.Value = other.Value;
	}

	clone() {
		const c = new Card();
		c.copyFrom(this);
		return c;
	}
}
