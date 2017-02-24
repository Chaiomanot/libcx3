#include "bag.hpp"

define_test(bag, "")
{
	bag_t<nat8_t> bag;

	insert(bag, 1ULL);
	insert(bag, 99ULL);
	insert(bag, 99ULL);
	insert(bag, 80085ULL);
	prove_eq(bag.cells.len, 4);

	nat8_t count = 0;
	for (auto& el : bag) {
		if (count < 2) {
			remove(bag, el);
		}
		++count;
	}
	prove_eq(count, 4);

	prove_eq(bag.cells[0], 0ULL);
	prove_eq(bag.cells[1], 0ULL);
	prove_eq(bag.cells[2], 99ULL);
	prove_eq(bag.cells[3], 80085ULL);

	count = 0;
	for (const auto& el : bag) {
		unused(el);
		++count;
	}
	prove_eq(count, 2);
	prove_eq(bag.cells.len, 4);

	return {};
}

