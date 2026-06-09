#include<iostream>
#include<cstring>
#include<cstdlib>
#include<ctime>
#include<fstream>
#include<string>
#include<unordered_set>
#include"Solitaire.h"
#if !defined(_WIN32) && !defined(WIN32)
#define _stricmp strcasecmp
#endif

using namespace std;

//A variation of the Klondike solver that deals a game by its FreeCell (FC) game
//number and solves it. By default it runs the same full, unconstrained exhaustive
//search as the minimal solver. Four optional constraint sets are also available,
//forming a strict inheritance chain where each level adds one rule on top of the
//next-higher-numbered level (level1 is the most restrictive):
//
//  level4 (/CONSTRAINT level4) - never take a card back off a foundation onto the
//          tableau. Drawing from the stock is otherwise unrestricted.
//
//  level3 (/CONSTRAINT level3) - level4's rule, plus: never draw from the stock while
//          any other move is available (a tableau move, a move to a foundation, or a
//          play of the current waste card onto a tableau column).
//
//  level2 (/CONSTRAINT level2) - level3's rules, plus: when any move onto a foundation
//          OR any play of the top waste card is available, explore only those.
//
//  level1 (/CONSTRAINT level1) - level2's rules, plus: when any move onto a foundation
//          is available, explore only the foundation moves.
//
//In every case the remaining branches are still searched exhaustively, so the result is
//the minimal solution obeying the chosen constraints, or that the deal is impossible
//under them (which can happen even for deals the unconstrained solver can win).
//
//The constraint set is chosen with /CONSTRAINT <mode>, where <mode> is one of
//level1, level2, level3, level4, or stepup. With no /CONSTRAINT the unconstrained
//minimal solver is used. "stepup" escalates level1 -> level2 -> level3 -> level4 ->
//unconstrained, stopping at the first mode that solves the deal.

static bool IsSolved(SolveResult result) {
	return result == SolvedMinimal || result == SolvedMayNotBeMinimal;
}

//Reads a CSV of game numbers to skip (e.g. a failed_games.csv). The number is taken from the
//first comma-separated field of each line; a line whose first field is not a number (such as a
//"game" header or a blank line) is ignored. Returns false only if the file cannot be opened.
static bool LoadExcludeGames(const char * path, unordered_set<int> & excluded) {
	ifstream in(path);
	if (!in.is_open()) { return false; }
	string line;
	while (getline(in, line)) {
		size_t comma = line.find(',');
		string field = comma == string::npos ? line : line.substr(0, comma);
		size_t start = field.find_first_not_of(" \t\r\n");
		if (start == string::npos) { continue; }
		size_t end = field.find_last_not_of(" \t\r\n");
		field = field.substr(start, end - start + 1);
		char c = field[0];
		if (c != '-' && (c < '0' || c > '9')) { continue; }
		excluded.insert(atoi(field.c_str()));
	}
	return true;
}

//Resets the dealt game and runs the single solver matching mode. Anything other than
//level1/level2/level3/level4 runs the unconstrained minimal solver. numThreads > 1 uses
//the multithreaded minimal solver, which only exists for the unconstrained mode; the
//constrained solvers always run single-threaded.
static SolveResult RunSolve(Solitaire & s, int drawCount, int maxStates, const char * mode, int numThreads) {
	s.ResetGame(drawCount);
	if (_stricmp(mode, "level1") == 0) { return s.SolveLevel1(maxStates); }
	if (_stricmp(mode, "level2") == 0) { return s.SolveLevel2(maxStates); }
	if (_stricmp(mode, "level3") == 0) { return s.SolveLevel3(maxStates); }
	if (_stricmp(mode, "level4") == 0) { return s.SolveLevel4(maxStates); }
	return numThreads > 1 ? s.SolveMinimalMultithreaded(numThreads, maxStates) : s.SolveMinimal(maxStates);
}

//Maps a constraint mode name to the level number SolveCounting expects (0 = unconstrained).
static int LevelOf(const char * mode) {
	if (_stricmp(mode, "level1") == 0) { return 1; }
	if (_stricmp(mode, "level2") == 0) { return 2; }
	if (_stricmp(mode, "level3") == 0) { return 3; }
	if (_stricmp(mode, "level4") == 0) { return 4; }
	return 0;
}

//Like SolveConstraint, but uses the single-threaded SolveCounting so it can also report, via
//winStates, the number of distinct explored states from which the win is reachable. For
//"stepup" it escalates level1 -> ... -> unconstrained, stopping at the first mode that solves;
//winStates reflects that solving mode (and is 0 for an unsolved deal).
static SolveResult SolveConstraintCounting(Solitaire & s, int drawCount, int maxStates, const char * constraint, const char * & usedMode, long long & winStates) {
	winStates = 0;
	if (_stricmp(constraint, "stepup") == 0) {
		static const char * ladder[] = { "level1", "level2", "level3", "level4", "unconstrained" };
		SolveResult result = Impossible;
		for (int i = 0; i < 5; i++) {
			s.ResetGame(drawCount);
			result = s.SolveCounting(maxStates, LevelOf(ladder[i]), winStates);
			usedMode = ladder[i];
			if (IsSolved(result)) { return result; }
		}
		return result;
	}
	usedMode = constraint;
	s.ResetGame(drawCount);
	return s.SolveCounting(maxStates, LevelOf(constraint), winStates);
}

//Solves the already-dealt game under the requested constraint. For "stepup" it escalates
//level1 -> level2 -> level3 -> level4 -> unconstrained, stopping at the first mode that
//solves. Sets usedMode to the mode that produced the returned result.
static SolveResult SolveConstraint(Solitaire & s, int drawCount, int maxStates, const char * constraint, int numThreads, const char * & usedMode) {
	if (_stricmp(constraint, "stepup") == 0) {
		static const char * ladder[] = { "level1", "level2", "level3", "level4", "unconstrained" };
		SolveResult result = Impossible;
		for (int i = 0; i < 5; i++) {
			result = RunSolve(s, drawCount, maxStates, ladder[i], numThreads);
			usedMode = ladder[i];
			if (IsSolved(result)) { return result; }
		}
		return result;
	}
	usedMode = constraint;
	return RunSolve(s, drawCount, maxStates, constraint, numThreads);
}

int main(int argc, char * argv[]) {
	Solitaire s;
	s.Initialize();

	int gameNumber = 1;
	int toGame = 0;
	int drawCount = 1;
	int maxStates = 5000000;
	bool showMoves = false;
	bool replay = false;
	int multiThreaded = 1;
	const char * constraint = "unconstrained";
	const char * outputFile = NULL;
	const char * excludeFile = NULL;

	for (int i = 1; i < argc; i++) {
		if (_stricmp(argv[i], "-game") == 0 || _stricmp(argv[i], "/game") == 0 || _stricmp(argv[i], "-g") == 0 || _stricmp(argv[i], "/g") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an FC game number to load.\n"; return 0; }
			gameNumber = atoi(argv[++i]);
		} else if (_stricmp(argv[i], "-togame") == 0 || _stricmp(argv[i], "/togame") == 0 || _stricmp(argv[i], "-tg") == 0 || _stricmp(argv[i], "/tg") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an FC game number to solve up to.\n"; return 0; }
			toGame = atoi(argv[++i]);
		} else if (_stricmp(argv[i], "-draw") == 0 || _stricmp(argv[i], "/draw") == 0 || _stricmp(argv[i], "-dc") == 0 || _stricmp(argv[i], "/dc") == 0) {
			if (i + 1 >= argc) { cout << "You must specify draw count.\n"; return 0; }
			drawCount = atoi(argv[++i]);
			if (drawCount < 1 || drawCount > 12) { cout << "Please specify a valid draw count from 1 to 12.\n"; return 0; }
		} else if (_stricmp(argv[i], "-states") == 0 || _stricmp(argv[i], "/states") == 0 || _stricmp(argv[i], "-s") == 0 || _stricmp(argv[i], "/s") == 0) {
			if (i + 1 >= argc) { cout << "You must specify max states.\n"; return 0; }
			maxStates = atoi(argv[++i]);
			if (maxStates < 1) { cout << "You must specify a valid max number of states.\n"; return 0; }
		} else if (_stricmp(argv[i], "-constraint") == 0 || _stricmp(argv[i], "/constraint") == 0 || _stricmp(argv[i], "-c") == 0 || _stricmp(argv[i], "/c") == 0) {
			if (i + 1 >= argc) { cout << "You must specify a constraint: level1, level2, level3, level4, or stepup.\n"; return 0; }
			constraint = argv[++i];
			if (_stricmp(constraint, "level1") != 0 && _stricmp(constraint, "level2") != 0 && _stricmp(constraint, "level3") != 0 && _stricmp(constraint, "level4") != 0 && _stricmp(constraint, "stepup") != 0) {
				cout << "Invalid constraint '" << constraint << "'. Use level1, level2, level3, level4, or stepup.\n"; return 0;
			}
		} else if (_stricmp(argv[i], "-m") == 0 || _stricmp(argv[i], "/m") == 0 || _stricmp(argv[i], "-multi") == 0 || _stricmp(argv[i], "/multi") == 0) {
			if (i + 1 >= argc) { cout << "You must specify number of threads.\n"; return 0; }
			multiThreaded = atoi(argv[++i]);
			if (multiThreaded < 2 || multiThreaded > 99) { cout << "Please specify a valid number of threads from 2 to 99.\n"; return 0; }
		} else if (_stricmp(argv[i], "-output") == 0 || _stricmp(argv[i], "/output") == 0 || _stricmp(argv[i], "-o") == 0 || _stricmp(argv[i], "/o") == 0) {
			if (i + 1 >= argc) { cout << "You must specify an output filename.\n"; return 0; }
			outputFile = argv[++i];
		} else if (_stricmp(argv[i], "-excludegames") == 0 || _stricmp(argv[i], "/excludegames") == 0 || _stricmp(argv[i], "-eg") == 0 || _stricmp(argv[i], "/eg") == 0) {
			if (i + 1 >= argc) { cout << "You must specify a path to an exclude CSV.\n"; return 0; }
			excludeFile = argv[++i];
		} else if (_stricmp(argv[i], "-mvs") == 0 || _stricmp(argv[i], "/mvs") == 0 || _stricmp(argv[i], "-moves") == 0 || _stricmp(argv[i], "/moves") == 0) {
			showMoves = true;
		} else if (_stricmp(argv[i], "-r") == 0 || _stricmp(argv[i], "/r") == 0) {
			replay = true;
		} else if (_stricmp(argv[i], "-?") == 0 || _stricmp(argv[i], "/?") == 0 || _stricmp(argv[i], "?") == 0 || _stricmp(argv[i], "/help") == 0 || _stricmp(argv[i], "-help") == 0) {
			cout << "SimpleSolver\n";
			cout << "Deals a Klondike game by FreeCell (FC) game number and solves it. By default it\n";
			cout << "runs a full exhaustive (unconstrained) search; a constraint set can be selected\n";
			cout << "with /CONSTRAINT.\n\n";
			cout << "SimpleSolver [/G #] [/DC #] [/S #] [/CONSTRAINT mode] [/M #] [/OUTPUT file] [/MVS] [/R] [#]\n\n";
			cout << "  /GAME # [/G #]    FC game number to deal and solve. Defaults to 1.\n";
			cout << "  /TOGAME # [/TG #] Solve a range of games from /GAME to this number,\n";
			cout << "                    printing CSV lines per deal.\n";
			cout << "  /DRAW # [/DC #]   Draw count to use. Defaults to 1.\n";
			cout << "  /STATES # [/S #]  Max game states to evaluate. Defaults to 5,000,000.\n";
			cout << "  /CONSTRAINT mode [/C mode]  Constraint set to impose. Each level adds a rule on\n";
			cout << "                    top of the next (level1 is the most restrictive):\n";
			cout << "                      level4 - never un-play a foundation card.\n";
			cout << "                      level3 - level4, plus never draw from stock while another move exists.\n";
			cout << "                      level2 - level3, plus explore only foundation/waste moves when any exist.\n";
			cout << "                      level1 - level2, plus explore only foundation moves when any exist.\n";
			cout << "                      stepup - try level1, then level2, level3, level4, then unconstrained.\n";
			cout << "                    Omit /CONSTRAINT to run the unconstrained minimal solver.\n";
			cout << "  /MULTI # [/M #]   Use # threads (2-99) for the unconstrained minimal solve.\n";
			cout << "                    Ignored by the constrained solvers, which are single-threaded.\n";
			cout << "  /OUTPUT file [/O file]  Write the CSV output to file (created in the current\n";
			cout << "                    directory) instead of stdout. Only applies in /TOGAME range mode.\n";
			cout << "  /EXCLUDEGAMES file [/EG file]  Skip any deal whose number appears in the first\n";
			cout << "                    column of this CSV (e.g. a failed_games.csv). /TOGAME range mode only.\n";
			cout << "  /MOVES [/MVS]     Output the compact list of moves made when solved.\n";
			cout << "  /R                Replay the solution step by step to output.\n";
			cout << "  #                 A bare number is treated as the FC game number.\n";
			return 0;
		} else if (argv[i][0] == '-' || argv[i][0] == '/') {
			cout << "Unknown option '" << argv[i] << "'. Use /? for usage.\n"; return 0;
		} else {
			//A bare token is accepted only if it is a plain number (the FC game number).
			//Reject anything else (e.g. a misspelled flag or stray path) so it can't be
			//silently turned into game 0.
			for (const char * p = argv[i]; *p != '\0'; p++) {
				if (*p < '0' || *p > '9') {
					cout << "Unknown argument '" << argv[i] << "'. Use /? for usage.\n"; return 0;
				}
			}
			gameNumber = atoi(argv[i]);
		}
	}

	s.SetDrawCount(drawCount);

	//Range mode: when /TOGAME is given, solve every deal from gameNumber to toGame
	//and emit one CSV line (game,moves,ms) per solved deal, like KlondikeSolver.
	if (toGame > 0) {
		int last = toGame < gameNumber ? gameNumber : toGame;
		//Write CSV to the file given by /OUTPUT, or to stdout when none was specified.
		FILE * out = stdout;
		if (outputFile != NULL) {
			out = fopen(outputFile, "w");
			if (out == NULL) { cout << "Could not open output file '" << outputFile << "' for writing.\n"; return 0; }
			cout << "Writing CSV output to " << outputFile << "\n";
		}
		//Optionally load a set of game numbers to skip (e.g. previously failed games).
		unordered_set<int> excluded;
		if (excludeFile != NULL) {
			if (!LoadExcludeGames(excludeFile, excluded)) {
				cout << "Could not open exclude file '" << excludeFile << "' for reading.\n";
				if (out != stdout) { fclose(out); }
				return 0;
			}
			//Only report to stdout when the CSV itself is going to a file, so stdout output stays clean.
			if (out != stdout) { cout << "Excluding " << excluded.size() << " games listed in " << excludeFile << "\n"; }
		}

		fprintf(out, "game,moves,time_us,num_recycles,max_states,constraint,num_wins\n");
		for (int deal = gameNumber; deal <= last; deal++) {
			//Skip any deal in the exclude set without attempting to solve it.
			if (!excluded.empty() && excluded.count(deal) > 0) { continue; }
			s.ShuffleFC(deal);
			const char * usedMode;
			long long winStates = 0;
			clock_t start = clock();
			//SolveConstraintCounting also counts the winning states; it is single-threaded,
			//so /MULTI does not apply in range mode.
			SolveResult result = SolveConstraintCounting(s, drawCount, maxStates, constraint, usedMode, winStates);
			clock_t elapsed = clock() - start;
			//Print a row for every deal; unsolved deals report 0 moves and 0 winning states.
			int moves = IsSolved(result) ? s.MovesMadeNormalizedCount() : 0;
			fprintf(out, "%d,%d,%lu,%d,%d,%s,%lld\n", deal, moves, (unsigned long)elapsed, s.RoundCount(), s.StatesUsed(), usedMode, winStates);
			fflush(out);
		}
		if (out != stdout) { fclose(out); }
		return 0;
	}

	s.ShuffleFC(gameNumber);
	s.ResetGame(drawCount);

	cout << "FC Game #" << gameNumber << " (draw " << drawCount << ", " << constraint << " constraint)\n";
	cout << s.GameDiagram() << "\n\n";

	const char * method;
	clock_t start = clock();
	SolveResult result = SolveConstraint(s, drawCount, maxStates, constraint, multiThreaded, method);
	clock_t elapsed = clock() - start;

	bool solved = IsSolved(result);
	if (result == SolvedMinimal) {
		cout << "Solved with the " << method << " method in " << s.MovesMadeNormalizedCount() << " moves (minimal under its constraints).";
	} else if (result == SolvedMayNotBeMinimal) {
		cout << "Solved with the " << method << " method in " << s.MovesMadeNormalizedCount() << " moves (state limit hit, may not be minimal).";
	} else if (result == Impossible) {
		cout << "Impossible under the " << method << " method. Best foundation count " << s.FoundationCount() << "/52.";
	} else {
		cout << "Could not complete the search (state limit hit). Best foundation count " << s.FoundationCount() << "/52.";
	}
	cout << " Took " << elapsed << " ms.\n";

	if (solved && showMoves) {
		cout << "\n" << s.MovesMade() << "\n";
	}

	if (solved && replay) {
		int movesToMake = s.MovesMadeCount();
		s.ResetGame(drawCount);
		for (int i = 0; i < movesToMake; i++) {
			cout << "----------------------------------------\n";
			cout << s.GetMoveInfo(s[i]) << "\n\n";
			s.MakeMove(s[i]);
			cout << s.GameDiagram() << "\n\n";
		}
		cout << "----------------------------------------\n";
	}

	return 0;
}
