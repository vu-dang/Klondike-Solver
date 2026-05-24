export class Move {
	constructor(from = 0, to = 0, count = 0, extra = 0) {
		this.From = from;
		this.To = to;
		this.Count = count;
		this.Extra = extra;
	}

	set(from, to, count, extra) {
		this.From = from;
		this.To = to;
		this.Count = count;
		this.Extra = extra;
	}

	copyFrom(other) {
		this.From = other.From;
		this.To = other.To;
		this.Count = other.Count;
		this.Extra = other.Extra;
	}

	clone() {
		return new Move(this.From, this.To, this.Count, this.Extra);
	}
}

export class MoveNode {
	constructor(move, parent = null) {
		this.Value = move;
		this.Parent = parent;
	}
}
