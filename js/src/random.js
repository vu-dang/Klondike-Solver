// Ports the C++ Random class. Operates on 32-bit signed integers so that
// shuffles produced match the original solver bit-for-bit.
export class Random {
	constructor(seed = 101) {
		this.seed = 0;
		this.mix = 0;
		this.twist = 0;
		this.value = 0;
		this.setSeed(seed);
	}

	calculateNext() {
		// Operator precedence matches the C++ original: `-` binds tighter than `^`.
		// All ops kept inside int32 with `| 0`.
		let y = (this.value ^ ((this.twist - this.mix) | 0) ^ this.value) | 0;
		y = (y ^ this.twist ^ this.value ^ this.mix) | 0;
		this.mix = (this.mix ^ this.twist ^ this.value) | 0;
		this.value = (this.value ^ ((this.twist - this.mix) | 0)) | 0;
		this.twist = (this.twist ^ this.value ^ y) | 0;
		this.value =
			(this.value ^ ((this.twist << 7) ^ (this.mix >> 16) ^ (y << 8))) | 0;
	}

	setSeed(seed) {
		seed = seed | 0;
		this.seed = seed;
		this.mix = 51651237 | 0;
		this.twist = 895213268 | 0;
		this.value = seed;

		for (let i = 0; i < 50; ++i) {
			this.calculateNext();
		}

		seed = (seed ^ (seed >> 15)) | 0;
		this.value = (0x9417b3af ^ seed) | 0;

		for (let i = 0; i < 950; ++i) {
			this.calculateNext();
		}
	}

	next1() {
		this.calculateNext();
		return this.value & 0x7fffffff;
	}

	next2() {
		if (this.seed === 0) {
			this.seed = 0x12345987 | 0;
		}
		const k = (this.seed / 127773) | 0;
		this.seed = (Math.imul(16807, (this.seed - k * 127773) | 0) - 2836 * k) | 0;
		if (this.seed < 0) {
			this.seed = (this.seed + 2147483647) | 0;
		}
		return this.seed & 0x7fffffff;
	}
}
