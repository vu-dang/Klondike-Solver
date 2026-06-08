#include"Move.h"

Move::Move() {}
Move::Move(unsigned char from, unsigned char to, unsigned char count, unsigned char extra) {
	From = from; To = to; Count = count; Extra = extra;
}
void Move::Set(unsigned char from, unsigned char to, unsigned char count, unsigned char extra) {
	From = from; To = to; Count = count; Extra = extra;
}

MoveNode::MoveNode(Move move) {
	Value = move;
	Parent = NULL;
}
MoveNode::MoveNode(Move move, shared_ptr<MoveNode> const& parent) {
	Value = move;
	Parent = parent;
}
//MoveNodes form a singly-linked list through Parent. The default (recursive) destructor
//would tear down a long chain one stack frame per node, overflowing the stack on deep
//solution paths (e.g. draw-3 deals under the beginner constraint). Unlink the chain
//iteratively instead: only follow a Parent that this node solely owns, so each node is
//destroyed with an already-empty Parent and no recursion occurs.
MoveNode::~MoveNode() {
	shared_ptr<MoveNode> parent = std::move(Parent);
	while (parent != NULL && parent.use_count() == 1) {
		shared_ptr<MoveNode> grandparent = std::move(parent->Parent);
		parent.reset();
		parent = std::move(grandparent);
	}
}