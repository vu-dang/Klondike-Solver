import {
	Card,
	ACE,
	KING,
	CLUBS,
	DIAMONDS,
	SPADES,
	HEARTS,
	SUIT_NONE,
} from './card.js';
import { Pile } from './pile.js';
import { Move, MoveNode } from './move.js';
import { Random } from './random.js';

export const WASTE = 0;
export const TABLEAU1 = 1;
export const TABLEAU2 = 2;
export const TABLEAU3 = 3;
export const TABLEAU4 = 4;
export const TABLEAU5 = 5;
export const TABLEAU6 = 6;
export const TABLEAU7 = 7;
export const STOCK = 8;
export const FOUNDATION1C = 9;
export const FOUNDATION2D = 10;
export const FOUNDATION3S = 11;
export const FOUNDATION4H = 12;

export const SolveResult = Object.freeze({
	CouldNotComplete: -2,
	SolvedMayNotBeMinimal: -1,
	Impossible: 0,
	SolvedMinimal: 1,
});

const PILES = 'W1234567GCDSH';
const RANKS = '0A23456789TJQK';
const SUITS = 'CDSH';

export class Solitaire {
	constructor() {
		this.piles = new Array(13);
		for (let i = 0; i < 13; i++) this.piles[i] = new Pile();
		this.cards = new Array(52);
		for (let i = 0; i < 52; i++) this.cards[i] = new Card();
		this.movesMade = new Array(512);
		for (let i = 0; i < 512; i++) this.movesMade[i] = new Move();
		this.movesAvailable = new Array(32);
		for (let i = 0; i < 32; i++) this.movesAvailable[i] = new Move();
		this.random = new Random();
		this.drawCount = 1;
		this.maxRounds = -1;
		this.roundCount = 0;
		this.foundationCount = 0;
		this.movesAvailableCount = 0;
		this.movesMadeCount = 0;
	}

	initialize() {
		this.drawCount = 1;
		this.maxRounds = -1;
		for (let i = 0; i < 52; i++) this.cards[i].set(i);
		for (let i = 0; i < 13; i++) this.piles[i].initialize();
	}

	setDrawCount(drawCount) {
		this.drawCount = drawCount;
	}

	drawCountValue() {
		return this.drawCount;
	}

	setMaxRounds(maxRounds) {
		this.maxRounds = maxRounds;
	}

	maxRoundsValue() {
		return this.maxRounds;
	}

	movesAvailableCountValue() {
		return this.movesAvailableCount;
	}

	movesMadeCountValue() {
		return this.movesMadeCount;
	}

	foundationCountValue() {
		return this.foundationCount;
	}

	roundCountValue() {
		return this.roundCount;
	}

	getMoveMade(index) {
		return this.movesMade[index];
	}

	getMoveAvailable(index) {
		return this.movesAvailable[index];
	}

	resetGame(drawCount = this.drawCount) {
		this.drawCount = drawCount;
		this.roundCount = 0;
		this.foundationCount = 0;
		this.movesMadeCount = 0;
		this.movesAvailableCount = 0;

		for (let i = 0; i < 13; ++i) this.piles[i].reset();

		for (let j = TABLEAU1, i = 0; j <= TABLEAU7; ++j) {
			this.piles[j].addUp(this.cards[i++]);
			for (let k = j + 1; k <= TABLEAU7; ++k, ++i) {
				this.piles[k].addDown(this.cards[i]);
			}
		}

		for (let i = 51; i >= 28; --i) {
			this.piles[STOCK].addUp(this.cards[i]);
		}
	}

	shuffle1(dealNumber = -1) {
		if (dealNumber !== -1) {
			this.random.setSeed(dealNumber);
		} else {
			dealNumber = this.random.next1();
			this.random.setSeed(dealNumber);
		}

		for (let i = 0; i < 52; i++) this.cards[i].set(i);

		for (let x = 0; x < 269; ++x) {
			const k = this.random.next1() % 52;
			const j = this.random.next1() % 52;
			const temp = this.cards[k];
			this.cards[k] = this.cards[j];
			this.cards[j] = temp;
		}
		return dealNumber;
	}

	shuffleFC(dealNumber) {
		const deck = new Int32Array(52);
		for (let i = 0; i < 52; i++) deck[i] = i;

		let seed = dealNumber | 0;
		for (let i = 0; i < 52; ++i) {
			const cardsLeft = 52 - i;
			seed = (Math.imul(seed, 214013) + 2531011) | 0;
			const rand = (seed >> 16) & 0x7fff;
			// Match C++ unsigned compare semantics: negative dealNumber goes to else branch.
			const rect =
				dealNumber >= 0 && dealNumber < 0x80000000
					? rand % cardsLeft
					: (rand | 0x8000) % cardsLeft;

			const temp = deck[rect];
			deck[rect] = deck[cardsLeft - 1];
			deck[cardsLeft - 1] = temp;
		}

		const suits = [CLUBS, DIAMONDS, HEARTS, SPADES];
		for (let i = 0; i < 52; i++) {
			const rank = deck[i] >> 2;
			const suitFC = deck[i] & 3;
			const suit = suits[suitFC];
			const value = suit * 13 + rank;
			this.cards[51 - i].set(value);
		}

		return dealNumber;
	}

	shuffle2(dealNumber) {
		for (let i = 0; i < 26; i++) this.cards[i].set(i);
		for (let i = 39; i < 52; i++) this.cards[i].set(i - 13);
		for (let i = 26; i < 39; i++) this.cards[i].set(i + 13);

		this.random.setSeed(dealNumber);
		for (let i = 0; i < 7; i++) {
			for (let j = 0; j < 52; j++) {
				const r = this.random.next2() % 52;
				const temp = this.cards[j];
				this.cards[j] = this.cards[r];
				this.cards[r] = temp;
			}
		}
		for (let i = 0, j = 51; i < 26; i++, j--) {
			const temp = this.cards[j];
			this.cards[j] = this.cards[i];
			this.cards[i] = temp;
		}
	}

	foundationMin() {
		const one = this.piles[FOUNDATION2D].size;
		const two = this.piles[FOUNDATION4H].size;
		const redFoundationMin = one <= two ? one : two;
		const onec = this.piles[FOUNDATION1C].size;
		const twoc = this.piles[FOUNDATION3S].size;
		const blackFoundationMin = onec <= twoc ? onec : twoc;
		return (
			2 +
			(blackFoundationMin <= redFoundationMin
				? blackFoundationMin
				: redFoundationMin)
		);
	}

	getTalonCards(talon, talonMoves) {
		let index = 0;

		const waste = this.piles[WASTE];
		const wasteSize = waste.size;
		if (wasteSize > 0) {
			talon[index] = waste.low();
			talonMoves[index++] = 0;
		}

		const stock = this.piles[STOCK];
		const stockSize = stock.size;
		const startJ =
			stockSize > 0 && stockSize - this.drawCount <= 0
				? 0
				: stockSize - this.drawCount;
		for (let j = startJ; j >= 0; j -= this.drawCount) {
			talon[index] = stock.getUp(j);
			talonMoves[index++] = stockSize - j;

			if (j > 0 && j < this.drawCount) {
				j = this.drawCount;
			}
		}

		// Cards reached by recycling the waste back into the stock require an additional round.
		// Only generate these moves when we are still allowed to recycle (maxRounds < 0 means unlimited).
		if (this.maxRounds < 0 || this.roundCount < this.maxRounds) {
			let amountToDraw = stockSize + stockSize + wasteSize + 1;
			const wasteEnd = wasteSize - 1;

			let lastIndex = this.drawCount - 1;
			while (lastIndex < wasteEnd) {
				talon[index] = waste.getUp(lastIndex);
				talonMoves[index++] = amountToDraw + lastIndex;
				lastIndex += this.drawCount;
			}

			if (lastIndex > wasteEnd && wasteEnd > -1) {
				amountToDraw += wasteEnd + stockSize;
				const startJ2 =
					stockSize > 0 && stockSize - lastIndex + wasteEnd <= 0
						? 0
						: stockSize - lastIndex + wasteEnd;
				for (let j = startJ2; j > 0; j -= this.drawCount) {
					talon[index] = stock.getUp(j);
					talonMoves[index++] = amountToDraw - j;
				}
			}
		}

		return index;
	}

	updateAvailableMoves() {
		this.movesAvailableCount = 0;
		const foundationMin = this.foundationMin();
		const talon = new Array(24);
		const talonMoves = new Array(24);
		const talonCount = this.getTalonCards(talon, talonMoves);

		for (let i = TABLEAU1; i <= TABLEAU7; ++i) {
			const pile1 = this.piles[i];
			const pile1Size = pile1.size;
			if (pile1Size === 0) continue;

			const pile1UpSize = pile1.upSize;
			const card1 = pile1.low();
			const cardFoundation = card1.Foundation;

			if (card1.Rank - this.piles[cardFoundation].size === 1) {
				if (card1.Rank < foundationMin) {
					this.movesAvailable[0].set(
						i,
						cardFoundation,
						1,
						pile1UpSize === 1 && pile1Size > 1 ? 1 : 0,
					);
					this.movesAvailableCount = 1;
					return;
				}
				this.movesAvailable[this.movesAvailableCount++].set(
					i,
					cardFoundation,
					1,
					pile1UpSize === 1 && pile1Size > 1 ? 1 : 0,
				);
			}

			const card2 = pile1.high();
			const pile1Length = card2.Rank - card1.Rank + 1;
			let kingMoved = false;

			for (let j = TABLEAU1; j <= TABLEAU7; ++j) {
				if (i === j) continue;
				const pile2 = this.piles[j];

				if (pile2.size === 0) {
					if (card2.Rank === KING && pile1Size !== pile1Length && !kingMoved) {
						this.movesAvailable[this.movesAvailableCount++].set(
							i,
							j,
							pile1Length,
							1,
						);
						kingMoved = true;
					}
					continue;
				}

				const card3 = pile2.low();
				if (
					card1.Rank >= card3.Rank ||
					card2.Rank + 1 < card3.Rank ||
					((card3.IsRed ^ card1.IsRed) ^ (card3.IsOdd ^ card1.IsOdd)) !== 0
				) {
					continue;
				}

				const pile1Moved = card3.Rank - card1.Rank;
				if (pile1Moved === pile1Length) {
					this.movesAvailable[this.movesAvailableCount++].set(
						i,
						j,
						pile1Moved,
						pile1Size > pile1Moved ? 1 : 0,
					);
					continue;
				}

				const card4 = pile1.getUp(pile1UpSize - pile1Moved - 1);
				if (card4.Rank - this.piles[card4.Foundation].size === 1) {
					this.movesAvailable[this.movesAvailableCount++].set(
						i,
						j,
						pile1Moved,
						0,
					);
				}
			}
		}

		for (let j = 0; j < talonCount; j++) {
			const talonCard = talon[j];
			const foundation = talonCard.Foundation;
			const cardsToDraw = talonMoves[j];

			if (talonCard.Rank - this.piles[foundation].size === 1) {
				if (talonCard.Rank <= foundationMin) {
					if (this.drawCount === 1) {
						if (cardsToDraw === 0 || this.movesAvailableCount === 0) {
							this.movesAvailable[0].set(WASTE, foundation, 1, cardsToDraw);
							this.movesAvailableCount = 1;
							return;
						} else {
							this.movesAvailable[this.movesAvailableCount++].set(
								WASTE,
								foundation,
								1,
								cardsToDraw,
							);
							break;
						}
					} else {
						this.movesAvailable[this.movesAvailableCount++].set(
							WASTE,
							foundation,
							1,
							cardsToDraw,
						);
						continue;
					}
				}
				this.movesAvailable[this.movesAvailableCount++].set(
					WASTE,
					foundation,
					1,
					cardsToDraw,
				);
			}

			for (let i = TABLEAU1; i <= TABLEAU7; ++i) {
				const pile = this.piles[i];
				if (pile.size !== 0) {
					const tableauCard = pile.low();
					if (
						tableauCard.Rank - talonCard.Rank !== 1 ||
						tableauCard.IsRed === talonCard.IsRed
					) {
						continue;
					}
					this.movesAvailable[this.movesAvailableCount++].set(
						WASTE,
						i,
						1,
						cardsToDraw,
					);
				} else if (talonCard.Rank === KING) {
					this.movesAvailable[this.movesAvailableCount++].set(
						WASTE,
						i,
						1,
						cardsToDraw,
					);
					break;
				}
			}
		}

		if (this.foundationCount === 0) return;

		const lastMove = this.movesMade[this.movesMadeCount - 1];
		for (let i = FOUNDATION1C; i <= FOUNDATION4H; ++i) {
			const pile1 = this.piles[i];
			const foundationRank = pile1.size;
			if (foundationRank === 0 || foundationRank <= foundationMin) continue;

			for (let j = TABLEAU1; j <= TABLEAU7; ++j) {
				const pile2 = this.piles[j];
				if (pile2.size !== 0) {
					const card = pile2.low();
					if ((card.Foundation & 1) === (i & 1) || card.Rank - foundationRank !== 1) {
						continue;
					}
					if (lastMove.From !== j && lastMove.To !== i) {
						this.movesAvailable[this.movesAvailableCount++].set(i, j, 1, 0);
					}
				} else if (foundationRank === KING) {
					if (lastMove.From !== j && lastMove.To !== i) {
						this.movesAvailable[this.movesAvailableCount++].set(i, j, 1, 0);
					}
					break;
				}
			}
		}
	}

	makeAutoMoves() {
		this.updateAvailableMoves();
		while (this.movesAvailableCount === 1) {
			this.makeMove(this.movesAvailable[0]);
			this.updateAvailableMoves();
		}
	}

	makeMoveByIndex(index) {
		this.makeMove(this.movesAvailable[index]);
	}

	makeMove(move) {
		this.movesMade[this.movesMadeCount++].copyFrom(move);

		if (move.Count === 1) {
			if (move.From === WASTE && move.Extra > 0) {
				let stockSize = this.piles[STOCK].size;
				if (move.Extra <= stockSize) {
					this.piles[STOCK].removeTalon(this.piles[WASTE], move.Extra);
				} else {
					this.roundCount++;
					stockSize += stockSize;
					const wasteSize = this.piles[WASTE].size;
					stockSize += wasteSize;
					stockSize += wasteSize;
					stockSize -= move.Extra;
					if (stockSize > 0) {
						this.piles[WASTE].removeTalon(this.piles[STOCK], stockSize);
					} else {
						this.piles[STOCK].removeTalon(this.piles[WASTE], -stockSize);
					}
				}
			}
			this.piles[move.From].remove(this.piles[move.To]);

			if (move.To >= FOUNDATION1C) ++this.foundationCount;
			else if (move.From >= FOUNDATION1C) --this.foundationCount;
		} else {
			this.piles[move.From].removeCount(this.piles[move.To], move.Count);
		}

		if (move.From !== WASTE && move.Extra > 0) {
			this.piles[move.From].flip();
		}
	}

	undoMove() {
		const move = this.movesMade[--this.movesMadeCount];

		if (move.From !== WASTE && move.Extra > 0) {
			this.piles[move.From].flip();
		}

		if (move.Count === 1) {
			this.piles[move.To].remove(this.piles[move.From]);

			if (move.To >= FOUNDATION1C) --this.foundationCount;
			else if (move.From >= FOUNDATION1C) ++this.foundationCount;

			if (move.From === WASTE && move.Extra > 0) {
				let wasteSize = this.piles[WASTE].size;
				if (move.Extra <= wasteSize) {
					this.piles[WASTE].removeTalon(this.piles[STOCK], move.Extra);
				} else {
					this.roundCount--;
					wasteSize += wasteSize;
					const stockSize = this.piles[STOCK].size;
					wasteSize += stockSize;
					wasteSize += stockSize;
					wasteSize -= move.Extra;
					if (wasteSize > 0) {
						this.piles[STOCK].removeTalon(this.piles[WASTE], wasteSize);
					} else {
						this.piles[WASTE].removeTalon(this.piles[STOCK], -wasteSize);
					}
				}
			}
		} else {
			this.piles[move.To].removeCount(this.piles[move.From], move.Count);
		}
	}

	minimumMovesLeft() {
		const waste = this.piles[WASTE];
		const wasteSize = waste.size;
		let win = this.piles[STOCK].size;
		const stockCount =
			((win / this.drawCount) | 0) + (win % this.drawCount === 0 ? 0 : 1);
		win += stockCount;
		win += wasteSize;

		for (let i = wasteSize - 1; i > 0; --i) {
			const card1 = waste.getUp(i);
			for (let j = i - 1; j >= 0; --j) {
				const card2 = waste.getUp(j);
				if (card1.Suit === card2.Suit && card1.Rank > card2.Rank) {
					++win;
					break;
				}
			}
		}

		for (let i = TABLEAU1; i <= TABLEAU7; ++i) {
			const pile = this.piles[i];
			let pileSize = pile.size;
			const downSize = pile.downSize;
			win += pileSize;
			win += downSize;
			if (downSize === 0) continue;

			pileSize -= downSize;
			// The C++ trick: walks `mins[suit]` then `mins[suit + 4]` etc.
			// We emulate the same 8-slot layout: indices 0..3 are the current min ranks
			// per suit, indices 4..7 are slots used to bump previously seen ranks.
			const mins = new Uint8Array(8);

			for (let j = pileSize - 1; j >= 0; j--) {
				const card1 = pile.getUp(j);
				mins[card1.Suit] = card1.Rank;
			}

			for (let j = downSize - 1; j >= 0; j--) {
				const card1 = pile.getDown(j);
				const suit = card1.Suit;
				let rankSlot = suit;
				let rank = mins[rankSlot];
				let cardRank = card1.Rank;

				if (mins[suit + 4] === 0) {
					if (rank > cardRank) {
						win++;
					}
					mins[rankSlot] = cardRank;
					continue;
				} else if (rank > cardRank) {
					do {
						win++;
						rankSlot += 4;
						rank = mins[rankSlot];
					} while (rank > cardRank);
				}

				do {
					const temp = mins[rankSlot];
					mins[rankSlot] = cardRank;
					cardRank = temp;
					rankSlot += 4;
					rank = mins[rankSlot];
				} while (rank < cardRank);
			}
		}

		return win;
	}

	movesAdded(move) {
		let movesAdded = 1;
		const wasteSize = this.piles[WASTE].size;
		const stockSize = this.piles[STOCK].size;
		if (move.Extra > 0) {
			if (move.From === WASTE) {
				if (move.Extra <= stockSize) {
					movesAdded += (move.Extra / this.drawCount) | 0;
					movesAdded += move.Extra % this.drawCount === 0 ? 0 : 1;
				} else {
					movesAdded += (stockSize / this.drawCount) | 0;
					movesAdded += stockSize % this.drawCount === 0 ? 0 : 1;
					let temp = move.Extra;
					temp -= wasteSize;
					temp -= stockSize;
					temp -= stockSize;
					movesAdded += (temp / this.drawCount) | 0;
					movesAdded += temp % this.drawCount === 0 ? 0 : 1;
				}
			} else {
				movesAdded++;
			}
		}
		return movesAdded;
	}

	movesMadeNormalizedCount() {
		let movesTotal = 0;
		let stockSize = 24;
		let wasteSize = 0;
		for (let i = 0; i < this.movesMadeCount; i++) {
			const m = this.movesMade[i];
			movesTotal++;
			if (m.Extra > 0) {
				if (m.From === WASTE) {
					let temp = stockSize;
					if (m.Extra <= stockSize) {
						temp = m.Extra;
						stockSize -= temp;
						wasteSize += temp - 1;
					} else {
						movesTotal += (temp / this.drawCount) | 0;
						movesTotal += temp % this.drawCount === 0 ? 0 : 1;
						temp = m.Extra;
						temp -= wasteSize;
						temp -= stockSize;
						temp -= stockSize;
						stockSize += wasteSize - temp;
						wasteSize = temp - 1;
					}
					movesTotal += (temp / this.drawCount) | 0;
					movesTotal += temp % this.drawCount === 0 ? 0 : 1;
				} else {
					movesTotal++;
				}
			} else if (m.From === WASTE) {
				wasteSize--;
			}
		}
		return movesTotal;
	}

	gameStateKey() {
		const order = [
			TABLEAU1,
			TABLEAU2,
			TABLEAU3,
			TABLEAU4,
			TABLEAU5,
			TABLEAU6,
			TABLEAU7,
		];
		let current = 1;
		while (current < 7) {
			let search = current;
			do {
				if (
					this.piles[order[search - 1]].highValue() <=
					this.piles[order[search]].highValue()
				) {
					break;
				}
				const temp = order[--search];
				order[search] = order[search + 1];
				order[search + 1] = temp;
			} while (search > 0);
			++current;
		}

		// We assemble exactly the same 21-byte key the C++ generates, then
		// stringify it for use as a Map key.
		const key = new Uint8Array(21);
		let z = 0;
		key[z++] =
			((this.piles[FOUNDATION1C].size << 4) |
				(this.piles[FOUNDATION2D].size + 1)) &
			0xff;
		key[z++] =
			((this.piles[FOUNDATION3S].size << 4) | this.piles[FOUNDATION4H].size) &
			0xff;

		let bits = 5;
		let mask = this.roundCount | 0;

		for (let i = 0; i < 7; ++i) {
			const pile = this.piles[order[i]];
			const upSize = pile.upSize;

			let added = 10;
			mask = (mask << 6) | 0;
			if (upSize > 0) {
				added += upSize - 1;
				mask = (mask | (pile.getUp(0).Value + 1)) | 0;
			}
			bits += added;
			mask = (mask << 4) | 0;
			mask = (mask | upSize) | 0;
			for (let j = 1; j < upSize; ++j) {
				mask = (mask << 1) | 0;
				mask = (mask | (pile.getUp(j).Suit >> 1)) | 0;
			}

			bits += 21 - added;
			mask = (mask << (21 - added)) | 0;
			do {
				bits -= 8;
				key[z++] = (mask >> bits) & 255;
			} while (bits >= 8);
		}
		if (bits > 0) {
			key[z] = mask & 255;
		}

		// Convert to string for use as Map key. Using fromCharCode is safe for bytes.
		return String.fromCharCode.apply(null, key);
	}

	// -------- solver --------

	solveMinimal(maxClosedCount) {
		this.makeAutoMoves();
		if (this.movesAvailableCount === 0) {
			return this.foundationCount === 52
				? SolveResult.SolvedMinimal
				: SolveResult.Impossible;
		}

		let maxFoundationCount = this.foundationCount;
		let bestSolutionMoveCount = 512;

		const closed = new Map();
		const open = new Array(512);
		for (let i = 0; i < 512; i++) open[i] = [];

		const movesToMake = new Array(512);
		const bestSolution = new Array(512);
		for (let i = 0; i < 512; i++) bestSolution[i] = new Move();
		bestSolution[0].Count = 255;

		const startMoves = this.minimumMovesLeft() + this.movesMadeNormalizedCount();

		let firstNode =
			this.movesMadeCount > 0
				? new MoveNode(this.movesMade[this.movesMadeCount - 1].clone())
				: null;
		let node = firstNode;
		for (let i = this.movesMadeCount - 2; i >= 0; i--) {
			node.Parent = new MoveNode(this.movesMade[i].clone());
			node = node.Parent;
		}
		open[startMoves].push(firstNode);

		while (closed.size < maxClosedCount) {
			let index = startMoves;
			while (index < 512 && open[index].length === 0) index++;
			if (index >= 512) break;

			firstNode = open[index].pop();

			this.resetGame(this.drawCount);
			let movesTotal = 0;
			node = firstNode;
			while (node !== null) {
				movesToMake[movesTotal++] = node.Value;
				node = node.Parent;
			}
			while (movesTotal > 0) {
				this.makeMove(movesToMake[--movesTotal]);
			}

			this.updateAvailableMoves();
			while (this.movesAvailableCount === 1) {
				const move = this.movesAvailable[0].clone();
				this.makeMove(move);
				firstNode = new MoveNode(move, firstNode);
				this.updateAvailableMoves();
			}
			movesTotal = this.movesMadeNormalizedCount();

			if (
				this.foundationCount > maxFoundationCount ||
				(this.foundationCount === maxFoundationCount &&
					bestSolutionMoveCount > movesTotal)
			) {
				bestSolutionMoveCount = movesTotal;
				maxFoundationCount = this.foundationCount;

				for (let i = 0; i < this.movesMadeCount; i++) {
					bestSolution[i].copyFrom(this.movesMade[i]);
				}
				bestSolution[this.movesMadeCount].Count = 255;
			} else if (maxFoundationCount === 52) {
				let helper = this.minimumMovesLeft();
				helper += movesTotal;
				if (helper >= bestSolutionMoveCount) continue;
			}

			const movesAvailableCount = this.movesAvailableCount;
			for (let i = 0; i < movesAvailableCount; i++) {
				const move = this.movesAvailable[i].clone();
				let added = this.movesAdded(move);

				this.makeMove(move);

				added += movesTotal;
				added += this.minimumMovesLeft();
				if (maxFoundationCount < 52 || added < bestSolutionMoveCount) {
					let helper = added;
					helper += 52 - this.foundationCount + this.roundCount;
					const key = this.gameStateKey();
					const existing = closed.get(key);
					if (existing === undefined || existing > added) {
						const newNode = new MoveNode(move, firstNode);
						closed.set(key, added);
						open[helper].push(newNode);
					}
				}

				this.undoMove();
			}
		}

		this.resetGame(this.drawCount);
		for (let i = 0; bestSolution[i].Count < 255; i++) {
			this.makeMove(bestSolution[i]);
		}

		return closed.size >= maxClosedCount
			? maxFoundationCount === 52
				? SolveResult.SolvedMayNotBeMinimal
				: SolveResult.CouldNotComplete
			: maxFoundationCount === 52
				? SolveResult.SolvedMinimal
				: SolveResult.Impossible;
	}

	solveFast(maxClosedCount, twoShift, threeShift) {
		this.makeAutoMoves();
		if (this.movesAvailableCount === 0) {
			return this.foundationCount === 52
				? SolveResult.SolvedMinimal
				: SolveResult.Impossible;
		}

		let maxFoundationCount = this.foundationCount;
		let bestSolutionMoveCount = 512;

		const closed = new Map();
		const open = new Array(512);
		for (let i = 0; i < 512; i++) open[i] = [];

		const movesToMake = new Array(512);
		const bestSolution = new Array(512);
		for (let i = 0; i < 512; i++) bestSolution[i] = new Move();
		bestSolution[0].Count = 255;

		const startMoves = this.movesMadeNormalizedCount() + this.minimumMovesLeft();
		const threeClosed = maxClosedCount >> threeShift;
		const twoClosed = maxClosedCount >> twoShift;

		let firstNode =
			this.movesMadeCount > 0
				? new MoveNode(this.movesMade[this.movesMadeCount - 1].clone())
				: null;
		let node = firstNode;
		for (let i = this.movesMadeCount - 2; i >= 0; i--) {
			node.Parent = new MoveNode(this.movesMade[i].clone());
			node = node.Parent;
		}
		open[startMoves].push(firstNode);

		while (closed.size < maxClosedCount) {
			let index = startMoves;
			while (index < 512 && open[index].length === 0) index++;
			if (index >= 512) break;

			firstNode = open[index].pop();

			this.resetGame(this.drawCount);
			let movesTotal = 0;
			node = firstNode;
			while (node !== null) {
				movesToMake[movesTotal++] = node.Value;
				node = node.Parent;
			}
			while (movesTotal > 0) {
				this.makeMove(movesToMake[--movesTotal]);
			}

			this.updateAvailableMoves();
			while (this.movesAvailableCount === 1) {
				const move = this.movesAvailable[0].clone();
				this.makeMove(move);
				firstNode = new MoveNode(move, firstNode);
				this.updateAvailableMoves();
			}
			movesTotal = this.movesMadeNormalizedCount();

			if (
				this.foundationCount > maxFoundationCount ||
				(this.foundationCount === maxFoundationCount &&
					bestSolutionMoveCount > movesTotal)
			) {
				bestSolutionMoveCount = movesTotal;
				maxFoundationCount = this.foundationCount;

				for (let i = 0; i < this.movesMadeCount; i++) {
					bestSolution[i].copyFrom(this.movesMade[i]);
				}
				bestSolution[this.movesMadeCount].Count = 255;
			} else if (maxFoundationCount === 52) {
				let helper = this.minimumMovesLeft();
				helper += movesTotal;
				if (helper >= bestSolutionMoveCount) continue;
			}

			let bestMove1 = null;
			let bestMove2 = null;
			let bestMove3 = null;
			let bestMoveAdded1 = 512,
				bestMoveHelper1 = 512;
			let bestMoveAdded2 = 512,
				bestMoveHelper2 = 512;
			let bestMoveAdded3 = 512,
				bestMoveHelper3 = 512;

			for (let i = 0; i < this.movesAvailableCount; i++) {
				const move = this.movesAvailable[i].clone();
				let added = this.movesAdded(move);

				this.makeMove(move);

				added += movesTotal;
				added += this.minimumMovesLeft();
				if (maxFoundationCount < 52 || added < bestSolutionMoveCount) {
					let helper = added;
					helper += 52 - this.foundationCount + this.roundCount;

					if (helper < bestMoveHelper1) {
						if (bestMoveHelper1 <= bestMoveHelper2) {
							if (bestMoveHelper2 <= bestMoveHelper3) {
								bestMove3 = bestMove2;
								bestMoveAdded3 = bestMoveAdded2;
								bestMoveHelper3 = bestMoveHelper2;
							}
							bestMove2 = bestMove1;
							bestMoveAdded2 = bestMoveAdded1;
							bestMoveHelper2 = bestMoveHelper1;
						} else if (bestMoveHelper1 <= bestMoveHelper3) {
							bestMove3 = bestMove1;
							bestMoveAdded3 = bestMoveAdded1;
							bestMoveHelper3 = bestMoveHelper1;
						}
						bestMove1 = move;
						bestMoveAdded1 = added;
						bestMoveHelper1 = helper;
					} else if (helper < bestMoveHelper2) {
						if (bestMoveHelper2 <= bestMoveHelper3) {
							bestMove3 = bestMove2;
							bestMoveAdded3 = bestMoveAdded2;
							bestMoveHelper3 = bestMoveHelper2;
						}
						bestMove2 = move;
						bestMoveAdded2 = added;
						bestMoveHelper2 = helper;
					} else if (helper < bestMoveHelper3) {
						bestMove3 = move;
						bestMoveAdded3 = added;
						bestMoveHelper3 = helper;
					}
				}

				this.undoMove();
			}

			const tryPush = (move, added, helper) => {
				this.makeMove(move);
				const key = this.gameStateKey();
				const existing = closed.get(key);
				if (existing === undefined || existing > added) {
					closed.set(key, added);
					const newNode = new MoveNode(move, firstNode);
					open[helper].push(newNode);
				}
				this.undoMove();
			};

			if (bestMoveHelper1 < 512) tryPush(bestMove1, bestMoveAdded1, bestMoveHelper1);
			if (closed.size < twoClosed && bestMoveHelper2 < 512)
				tryPush(bestMove2, bestMoveAdded2, bestMoveHelper2);
			if (closed.size < threeClosed && bestMoveHelper3 < 512)
				tryPush(bestMove3, bestMoveAdded3, bestMoveHelper3);
		}

		this.resetGame(this.drawCount);
		for (let i = 0; bestSolution[i].Count < 255; i++) {
			this.makeMove(bestSolution[i]);
		}
		return maxFoundationCount === 52
			? SolveResult.SolvedMayNotBeMinimal
			: SolveResult.CouldNotComplete;
	}

	// -------- string output (matching C++) --------

	_appendMove(parts, m, stockSize, wasteSize, combine) {
		if (m.Extra > 0) {
			if (m.From !== WASTE) {
				if (m.Count > 1) {
					if (!combine) {
						parts.push(`${PILES[m.From]}${PILES[m.To]}-${m.Count} F${m.From}`);
					} else {
						parts.push(`[${PILES[m.From]}${PILES[m.To]}-${m.Count} F${m.From}]`);
					}
				} else if (!combine) {
					parts.push(`${PILES[m.From]}${PILES[m.To]} F${m.From}`);
				} else {
					parts.push(`[${PILES[m.From]}${PILES[m.To]} F${m.From}]`);
				}
			} else if (!combine) {
				if (m.Extra <= stockSize) {
					const draws =
						((m.Extra / this.drawCount) | 0) +
						(m.Extra % this.drawCount === 0 ? 0 : 1);
					parts.push(`DR${draws}`);
					parts.push(`${PILES[m.From]}${PILES[m.To]}`);
				} else {
					const temp1 =
						((stockSize / this.drawCount) | 0) +
						(stockSize % this.drawCount === 0 ? 0 : 1);
					if (temp1 !== 0) parts.push(`DR${temp1}`);
					parts.push('NEW');
					let temp2 = m.Extra - stockSize - stockSize - wasteSize;
					temp2 =
						((temp2 / this.drawCount) | 0) + (temp2 % this.drawCount === 0 ? 0 : 1);
					parts.push(`DR${temp2}`);
					parts.push(`${PILES[m.From]}${PILES[m.To]}`);
				}
			} else if (m.Extra <= stockSize) {
				const draws =
					((m.Extra / this.drawCount) | 0) +
					(m.Extra % this.drawCount === 0 ? 0 : 1);
				parts.push(`[DR${draws} ${PILES[m.From]}${PILES[m.To]}]`);
			} else {
				let temp = m.Extra - stockSize - stockSize - wasteSize;
				temp =
					((temp / this.drawCount) | 0) + (temp % this.drawCount === 0 ? 0 : 1);
				temp +=
					((stockSize / this.drawCount) | 0) +
					(stockSize % this.drawCount === 0 ? 0 : 1);
				parts.push(`[DR${temp} ${PILES[m.From]}${PILES[m.To]}]`);
			}
		} else if (m.Count > 1) {
			parts.push(`${PILES[m.From]}${PILES[m.To]}-${m.Count}`);
		} else {
			parts.push(`${PILES[m.From]}${PILES[m.To]}`);
		}
	}

	movesMadeString() {
		const totalMoves = this.movesMadeCount;
		const savedMoves = [];
		for (let i = 0; i < totalMoves; i++) savedMoves.push(this.movesMade[i].clone());

		this.resetGame(this.drawCount);
		const parts = [];
		for (let i = 0; i < totalMoves; i++) {
			const m = savedMoves[i];
			this._appendMove(
				parts,
				m,
				this.piles[STOCK].size,
				this.piles[WASTE].size,
				false,
			);
			this.makeMove(m);
		}
		return parts.join(' ');
	}

	gameDiagram() {
		const lines = [];
		for (let i = 0; i < 13; i++) {
			let line = (i < 10 ? ' ' : '') + i + ': ';
			const p = this.piles[i];
			for (let j = p.upSize - 1; j >= 0; j--) {
				const c = p.getUp(j);
				const rank = RANKS[c.Rank];
				const suit = c.Suit !== SUIT_NONE ? SUITS[c.Suit] : 'X';
				line += `${rank}${suit} `;
			}
			for (let j = p.downSize - 1; j >= 0; j--) {
				const c = p.getDown(j);
				const rank = RANKS[c.Rank];
				const suit = c.Suit !== SUIT_NONE ? SUITS[c.Suit] : 'X';
				line += `-${rank}${suit}`;
			}
			lines.push(line);
		}
		lines.push('Minimum Moves Needed: ' + this.minimumMovesLeft());
		return lines.join('\n');
	}

	// Returns an array of structured move objects representing the current
	// movesMade list expanded into atomic actions a UI can replay step-by-step.
	expandedMoves() {
		const totalMoves = this.movesMadeCount;
		const savedMoves = [];
		for (let i = 0; i < totalMoves; i++) savedMoves.push(this.movesMade[i].clone());

		this.resetGame(this.drawCount);
		const out = [];
		for (let i = 0; i < totalMoves; i++) {
			const m = savedMoves[i];
			this._expandOne(out, m);
			this.makeMove(m);
		}
		return out;
	}

	_expandOne(out, m) {
		const drawCount = this.drawCount;
		const stockSize = this.piles[STOCK].size;
		const wasteSize = this.piles[WASTE].size;

		if (m.From === WASTE && m.Extra > 0) {
			// First, optional draws / recycle.
			if (m.Extra <= stockSize) {
				const draws =
					((m.Extra / drawCount) | 0) + (m.Extra % drawCount === 0 ? 0 : 1);
				for (let k = 0; k < draws; k++) {
					out.push({ type: 'draw', count: drawCount });
				}
			} else {
				const drawsBefore =
					((stockSize / drawCount) | 0) + (stockSize % drawCount === 0 ? 0 : 1);
				for (let k = 0; k < drawsBefore; k++) {
					out.push({ type: 'draw', count: drawCount });
				}
				out.push({ type: 'recycle' });
				let temp = m.Extra - stockSize - stockSize - wasteSize;
				const drawsAfter =
					((temp / drawCount) | 0) + (temp % drawCount === 0 ? 0 : 1);
				for (let k = 0; k < drawsAfter; k++) {
					out.push({ type: 'draw', count: drawCount });
				}
			}
		}

		out.push({
			type: 'move',
			from: pileName(m.From),
			to: pileName(m.To),
			count: m.Count,
		});

		if (m.From !== WASTE && m.Extra > 0) {
			out.push({ type: 'flip', pile: pileName(m.From) });
		}
	}

	clone() {
		const s = new Solitaire();
		s.restoreFrom(this);
		return s;
	}

	restoreFrom(other) {
		this.drawCount = other.drawCount;
		this.roundCount = other.roundCount;
		this.foundationCount = other.foundationCount;
		this.movesAvailableCount = other.movesAvailableCount;
		this.movesMadeCount = other.movesMadeCount;
		for (let i = 0; i < 52; i++) this.cards[i].copyFrom(other.cards[i]);
		for (let i = 0; i < 13; i++) this.piles[i].copyFrom(other.piles[i]);
		for (let i = 0; i < other.movesMadeCount; i++) {
			this.movesMade[i].copyFrom(other.movesMade[i]);
		}
		for (let i = 0; i < other.movesAvailableCount; i++) {
			this.movesAvailable[i].copyFrom(other.movesAvailable[i]);
		}
	}
}

export function pileName(index) {
	if (index === WASTE) return 'waste';
	if (index === STOCK) return 'stock';
	if (index >= TABLEAU1 && index <= TABLEAU7) return 't' + index;
	if (index === FOUNDATION1C) return 'foundationC';
	if (index === FOUNDATION2D) return 'foundationD';
	if (index === FOUNDATION3S) return 'foundationS';
	if (index === FOUNDATION4H) return 'foundationH';
	return 'unknown';
}
