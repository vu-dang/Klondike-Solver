import { Solitaire, SolveResult, pileName } from './solitaire.js';

export { Solitaire, SolveResult, pileName };

/**
 * Solve a Klondike Solitaire game by its FreeCell-style game number.
 *
 * @param {number} gameNumber - The game seed/number (matches Microsoft FreeCell game numbering).
 * @param {object} [options]
 * @param {number} [options.drawCount=1] - Cards drawn from stock per draw (1 or 3 typical).
 * @param {number} [options.maxStates=5_000_000] - Maximum game states to explore before giving up.
 * @param {boolean} [options.fast=false] - If true, use the faster (non-minimal) solver.
 * @param {'fc'|'ms'|'pysol'} [options.shuffle='fc'] - Shuffle algorithm:
 *   'fc' - Microsoft FreeCell-style deal numbering (default, matches CLI behavior)
 *   'ms' - Microsoft Klondike PRNG-based shuffle
 *   'pysol' - PySol-compatible shuffle
 * @returns {{
 *   result: 'minimal'|'maybeMinimal'|'impossible'|'incomplete',
 *   moves: Array<object>,
 *   compactMoves: string,
 *   moveCount: number,
 *   foundationCount: number,
 * }}
 */
export function solveGame(gameNumber, options = {}) {
	const {
		drawCount = 1,
		maxStates = 5_000_000,
		fast = false,
		shuffle = 'fc',
	} = options;

	const s = new Solitaire();
	s.initialize();
	s.setDrawCount(drawCount);

	switch (shuffle) {
		case 'ms':
			s.shuffle1(gameNumber);
			break;
		case 'pysol':
			s.shuffle2(gameNumber);
			break;
		case 'fc':
		default:
			s.shuffleFC(gameNumber);
			break;
	}

	s.resetGame();

	let result;
	if (fast) {
		// Try a few fast variants and keep the best (mirrors the C++ CLI).
		result = s.solveFast(maxStates, 0, 0);
		let bestCount =
			result === SolveResult.SolvedMinimal ||
			result === SolveResult.SolvedMayNotBeMinimal
				? s.movesMadeNormalizedCount()
				: 512;
		let bestFoundation = s.foundationCountValue();
		let best = s.clone();

		s.resetGame();
		const r2 = s.solveFast(maxStates, 0, 4);
		if (
			(r2 === SolveResult.SolvedMinimal ||
				r2 === SolveResult.SolvedMayNotBeMinimal) &&
			s.movesMadeNormalizedCount() < bestCount
		) {
			best = s.clone();
			bestCount = s.movesMadeNormalizedCount();
			bestFoundation = s.foundationCountValue();
		}
		if (s.foundationCountValue() > bestFoundation) {
			best = s.clone();
			bestFoundation = s.foundationCountValue();
		}

		s.resetGame();
		const r3 = s.solveFast(maxStates, 1, 4);
		if (
			(r3 === SolveResult.SolvedMinimal ||
				r3 === SolveResult.SolvedMayNotBeMinimal) &&
			s.movesMadeNormalizedCount() < bestCount
		) {
			best = s.clone();
			bestCount = s.movesMadeNormalizedCount();
			bestFoundation = s.foundationCountValue();
		}
		if (s.foundationCountValue() > bestFoundation) {
			best = s.clone();
			bestFoundation = s.foundationCountValue();
		}

		s.restoreFrom(best);

		result =
			bestFoundation === 52
				? SolveResult.SolvedMayNotBeMinimal
				: SolveResult.CouldNotComplete;
	} else {
		result = s.solveMinimal(maxStates);
	}

	const resultName = {
		[SolveResult.SolvedMinimal]: 'minimal',
		[SolveResult.SolvedMayNotBeMinimal]: 'maybeMinimal',
		[SolveResult.Impossible]: 'impossible',
		[SolveResult.CouldNotComplete]: 'incomplete',
	}[result];

	const compactMoves = s.movesMadeString();
	const moves = s.expandedMoves();

	return {
		result: resultName,
		moves,
		compactMoves,
		moveCount: s.movesMadeNormalizedCount(),
		foundationCount: s.foundationCountValue(),
	};
}
