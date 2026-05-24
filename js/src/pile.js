import { Card } from './card.js';

export class Pile {
	constructor() {
		this.down = new Array(24);
		this.up = new Array(24);
		for (let i = 0; i < 24; i++) {
			this.down[i] = new Card();
			this.up[i] = new Card();
		}
		this.size = 0;
		this.upSize = 0;
		this.downSize = 0;
	}

	addDown(card) {
		this.down[this.downSize++].copyFrom(card);
		this.size++;
	}

	addUp(card) {
		this.up[this.upSize++].copyFrom(card);
		this.size++;
	}

	flip() {
		if (this.upSize > 0) {
			this.down[this.downSize++].copyFrom(this.up[--this.upSize]);
		} else {
			this.up[this.upSize++].copyFrom(this.down[--this.downSize]);
		}
	}

	remove(to) {
		to.addUp(this.up[--this.upSize]);
		this.size--;
	}

	removeCount(to, count) {
		for (let i = this.upSize - count; i < this.upSize; ++i) {
			to.addUp(this.up[i]);
		}
		this.upSize -= count;
		this.size -= count;
	}

	removeTalon(to, count) {
		const target = this.size - count;
		do {
			to.addUp(this.up[--this.size]);
		} while (this.size > target);
		this.upSize = this.size;
	}

	reset() {
		this.size = 0;
		this.upSize = 0;
		this.downSize = 0;
	}

	initialize() {
		this.size = 0;
		this.upSize = 0;
		this.downSize = 0;
		for (let i = 0; i < 24; i++) {
			this.up[i].clear();
			this.down[i].clear();
		}
	}

	highValue() {
		return this.upSize > 0 ? this.up[0].Value : 99;
	}

	getUp(index) {
		return this.up[index];
	}

	getDown(index) {
		return this.down[index];
	}

	low() {
		return this.up[this.upSize - 1];
	}

	high() {
		return this.up[0];
	}

	copyFrom(other) {
		this.size = other.size;
		this.upSize = other.upSize;
		this.downSize = other.downSize;
		for (let i = 0; i < other.upSize; i++) {
			this.up[i].copyFrom(other.up[i]);
		}
		for (let i = 0; i < other.downSize; i++) {
			this.down[i].copyFrom(other.down[i]);
		}
	}
}
