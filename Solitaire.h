#ifndef Solitaire_h
#define Solitaire_h
#include<string>
#include<stack>
#include<memory>
#include<mutex>
#include"Random.h"
#include"Move.h"
#include"Card.h"
#include"Pile.h"
#include"HashMap.h"
using namespace std;

enum SolveResult {
	CouldNotComplete = -2,
	SolvedMayNotBeMinimal = -1,
	Impossible = 0,
	SolvedMinimal = 1
};

class Solitaire {
	friend class SolitaireWorker;
private:
	Move movesMade[512];
	Pile piles[13];
	Card cards[52];
	Move movesAvailable[32];
	Random random;
	int drawCount, roundCount, foundationCount, movesAvailableCount, movesMadeCount, maxRounds;
	int statesUsed;

	int FoundationMin();
	int GetTalonCards(Card talon[], int talonMoves[]);
	void FilterLevel1Moves();
	void FilterLevel2Moves();
	void FilterLevel3Moves();
	void FilterLevel4Moves();
public:
	void Initialize();
	int Shuffle1(int dealNumber = -1);
	int ShuffleFC(int deal_number);
	void Shuffle2(int dealNumber);
	void ResetGame();
	void ResetGame(int drawCount);
	void UpdateAvailableMoves();
	void MakeAutoMoves();
	void MakeMove(Move move);
	void MakeMove(int index);
	void UndoMove();
	SolveResult SolveMinimalMultithreaded(int numThreads, int maxClosedCount);
	SolveResult SolveMinimal(int maxClosedCount);
	SolveResult SolveFast(int maxClosedCount, int twoShift, int threeShift);
	SolveResult SolveLevel1(int maxClosedCount);
	SolveResult SolveLevel2(int maxClosedCount);
	SolveResult SolveLevel3(int maxClosedCount);
	SolveResult SolveLevel4(int maxClosedCount);
	SolveResult SolveRandom(int numberOfTimesToPlay, int solutionsToFind);
	int MovesAvailableCount();
	int MovesMadeCount();
	int MovesMadeNormalizedCount();
	int FoundationCount();
	int RoundCount();
	int StatesUsed();
	int DrawCount();
	int MovesAdded(Move const& move);
	int MinimumMovesLeft();
	void SetDrawCount(int drawCount);
	void SetMaxRounds(int maxRounds);
	int MaxRounds();
	HashKey GameState();
	string GetMoveInfo(Move move);
	bool LoadSolitaire(string const& cardSet);
	string GetSolitaire();
	bool LoadPysol(string const& cardSet);
	string GetPysol();
	Move GetMoveAvailable(int index);
	Move GetMoveMade(int index);
	string GameDiagram();
	string GameDiagramPysol();
	string MovesMade();
	string MovesAvailable();
	Move operator[](int index);
};

class SolitaireWorker {
private:
	stack<shared_ptr<MoveNode>> open[512];
	Move bestSolution[512];
	Solitaire * solitaire;
	mutex mtx;
	int openCount, maxFoundationCount, bestSolutionMoveCount, startMoves, maxClosedCount;

	void RunMinimalWorker(void * closed);
	void RunFastWorker();
public:
	SolitaireWorker(Solitaire & solitaire, int maxClosedCount);

	SolveResult Run(int numThreads);
};
#endif