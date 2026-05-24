// Quick verification: shuffle deals should match the C++ reference,
// and a small solve should produce a valid solution.

import { Solitaire } from '../src/solitaire.js';
import { solveGame } from '../src/index.js';

let passed = 0;
let failed = 0;

function test(name, fn) {
	try {
		fn();
		console.log(`  ok  - ${name}`);
		passed++;
	} catch (e) {
		console.log(`  FAIL - ${name}`);
		console.log(`         ${e.message}`);
		failed++;
	}
}

function assertEq(actual, expected, msg = '') {
	if (actual !== expected) {
		throw new Error(`${msg}\n   expected: ${expected}\n   actual:   ${actual}`);
	}
}

// ---- shuffleFC test ----
test('shuffleFC(1) produces the C++ reference deal', () => {
	const s = new Solitaire();
	s.initialize();
	s.shuffleFC(1);
	s.resetGame();

	const diagram = s.gameDiagram();
	const expected = [
		' 0: ',
		' 1: JD ',
		' 2: 5H -2D',
		' 3: QC -KD-9H',
		' 4: 9D -KH-KC-JC',
		' 5: AH -QD-3H-9S-5D',
		' 6: 5C -3C-JS-2S-5S-7H',
		' 7: QH -TS-4C-AS-KS-AD-7C',
		' 8: 4H AC 4D 7S 3S TD 4S TH 8H 2C JH 7D 6D 8S 8D QS 6C 3D 8C TC 6S 9C 2H 6H ',
		' 9: ',
		'10: ',
		'11: ',
		'12: ',
		'Minimum Moves Needed: 102',
	].join('\n');

	assertEq(diagram, expected, 'shuffleFC(1) deal mismatch');
});

// ---- Solver tests vs C++ reference ----
const REFERENCE = [
	{
		game: 1,
		drawCount: 1,
		moves: 148,
		compact:
			'5H F5 DR2 WC W6 DR3 W6 DR5 WC DR1 W3 DR3 W4 DR6 W1 41-2 F4 34-2 F3 DR3 WH DR1 NEW DR7 W1 DR3 W3 13-5 41-3 F4 74 F7 71 F7 72 F7 7S F7 57 F5 5H F5 DR1 W3 23-2 F2 26 72-2 F7 7D F7 6D DR1 WD W5 W7 67-3 F6 6C F6 3C 62 F6 6S F6 7S 7H 7C 3H 3C DR2 W3 DR2 WH NEW DR1 WD DR3 WS 6S F6 6H 3S 46-2 F4 W4 WS DR2 WH 54-2 F5 5D 7D 3D 7C 3S 4D DR1 WC 3D 4S DR1 WC 4D 3C 4C 3D 1S 2S 3S 2D 3D F3 3H WH 1H 1C 6H 1H 2S 6C',
	},
	{
		game: 2,
		drawCount: 3,
		moves: 116,
		compact:
			'DR1 WC W2 DR2 W2 DR1 W2 DR1 W4 WD 37 F3 27-4 F2 DR2 WS DR1 W2 NEW DR1 WH 7H DR1 WS 7S W2 DR1 WH 7H W2 42-2 F4 4D F4 W4 64 F6 37 F3 63 F6 64 F6 W6 DR1 W6 WH WD 7D 5D F5 52 F5 W5 75-3 F7 17 71-2 F7 76 F7 7S F7 DR1 WS DR1 W7 57-5 F5 65-4 F6 6H F6 6C 73-7 F7 47-4 F4 4C 51-5 F5 5C 3C 2C 3D WD 3C 2H 3H 2C 3C 2H 3H 2C 3C 2H 3H WH WS 1S 1D 7S 1S 7D 1D 7S 1S 7D 1D 2C 7S F7 1S 3C 7D',
	},
	{
		game: 5,
		drawCount: 1,
		moves: 133,
		compact:
			'1H DR6 W4 DR3 W1 DR3 WC DR1 WH 3C F3 57 F5 5H F5 WH W1 25 F2 72-2 F7 DR1 W7 W7 W7 DR3 W6 36 F3 DR5 WD 3D 5D 43-2 F4 4C F4 4S F4 74-4 F7 W7 W3 73-2 F7 7H F7 72 F7 7D F7 7S 2D DR2 NEW DR1 W3 DR3 W4 W1 DR3 WD 4D 63-3 F6 6D F6 67 F6 6S F6 5S F5 W1 WS 2S 51 F5 5C 3C 3H DR1 WC 2H 3S DR2 WC 2S 3H 4C 1C 3S 4D DR1 WS 4C 3D 4D 3S 4S 62 F6 6H 1H 1C WC WH 1D 3H 1S 2H 3C 7D',
	},
];

for (const ref of REFERENCE) {
	test(
		`solveGame(${ref.game}, draw=${ref.drawCount}) matches C++ reference (${ref.moves} moves)`,
		() => {
			const r = solveGame(ref.game, {
				drawCount: ref.drawCount,
				maxStates: 5_000_000,
			});
			assertEq(r.result, 'minimal', 'expected minimal solution');
			assertEq(r.foundationCount, 52, 'foundation count');
			assertEq(r.moveCount, ref.moves, 'normalized move count');
			assertEq(r.compactMoves, ref.compact, 'compact move string');
			if (!Array.isArray(r.moves) || r.moves.length === 0) {
				throw new Error('expected non-empty expanded moves array');
			}
		},
	);
}

console.log(`\n${passed} passed, ${failed} failed`);
process.exit(failed === 0 ? 0 : 1);
