/*
	Possible Approaches:
	* Implicit Pathogen
		- focus only on the state of the host
		- SEIRD model - Susceptible/Exposed/Infected/Recovered/Deceased
	* Implicit Host
		- focus only on the presence/state of the pathogen
		- APEXR model - Absent/Present/Established/eXpired/Rejected
	* Explicit
		- consider host as a container for pathogen
		- Either SEIRD or blend of the two
*/
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

// Integer proxy representation - 16bits
// storage upper-bound: 2*N Bytes
// susceptible = 0
// exposed = [1,kE]
// infected = [kE+1,kE+kI]
// recovered = -2
// deceased = -1
unsigned long NCOMPS = 0;
unsigned long NRANDS = 0;

using Host = std::tuple<short,short,short,short>;

class Pathogen {
public:
	Pathogen(std::string name = "Ebola", double pE = 0.005, double pD = 0.5, short minE = 2, short kE = 9, short minI = 7, short kI = 9, short kT = 1)
		: name(name), pcatch(pE), pdie(pD), edist(1.0/(kE-minE+1)), idist(1.0/(kI-minI+1)), ndist(1.0/kT), minE(minE), minI(minI) {}
	bool susceptible (Host const& h) { return std::get<0>(h) ==  0; }
	bool deceased    (Host const& h) { return std::get<0>(h) == -1; }
	bool recovered   (Host const& h) { return std::get<0>(h) == -2; }
	bool exposed     (Host const& h) { return 0 < std::get<0>(h) && std::get<0>(h) <= std::get<1>(h); }
	bool infectious  (Host const& h) { return std::get<1>(h) < std::get<0>(h) && std::get<0>(h) <= (std::get<1>(h) + std::get<2>(h)); }

	bool  catches()   { return pcatch(rng); }
	bool  dies()      { return pdie(rng); }
	short incubates() { return minE + edist(rng); }
	short infects()   { return minI + idist(rng); }
	short numNeighbors() { return 1 + ndist(rng); }

private:
	std::string name;
	static std::mt19937 rng;
	//static std::default_random_engine rng;
	std::bernoulli_distribution pcatch;
	std::bernoulli_distribution pdie;
	std::geometric_distribution<> edist;
	std::geometric_distribution<> idist;
	std::geometric_distribution<> ndist;
	short minE;
	short minI;
};

std::mt19937 Pathogen::rng { std::random_device()() };
//std::default_random_engine Pathogen::rng { std::random_device()() };

using GridMap = std::vector<std::vector<Host>>;

int isExposed (int hi, int hj, GridMap const& m, Pathogen p)
{
	int count = 0;
	int N = m.size() - 1, M = m[0].size() - 1;
	for (auto i = std::max(0, hi - 1); i <= std::min(hi + 1, N); ++i) {
		for (auto j = std::max(0, hj - 1); j <= std::min(hj + 1, M); ++j) {
			if (p.infectious(m[i][j])) {
				++count;
			}
		}
	}
	return count;
}



GridMap computeNext (GridMap const& m, Pathogen p)
{
	GridMap m_next { m };
	int N = m.size();
	int M = m[0].size();
	for (auto i = 0; i < N; ++i) {
		auto& row = m[i];
		for (auto j = 0; j < M; ++j) {
			auto& cell = row[j];
			auto& cellnext = m_next[i][j];
			if (p.exposed(cell)) {
				++std::get<0>(cellnext);
			} else if (p.infectious(cell)) {
				++std::get<0>(cellnext);
				auto hk = std::get<3>(cellnext);
				for (auto hi = std::max(0, i - hk); hi <= std::min(i + hk, N - 1); ++hi) {
					for (auto hj = std::max(0, j - hk); hj <= std::min(j + hk, M - 1); ++hj) {
						auto& neighbor = m_next[hi][hj];
						if (p.susceptible(neighbor)) {
							if (p.catches()) {
								std::get<1>(neighbor) = p.incubates();
								std::get<2>(neighbor) = p.infects();
								++std::get<0>(neighbor);
							}
						}
					}
				}
				
				if (!p.infectious(cellnext)) {
					if (p.dies()) {
						std::get<0>(cellnext) = -1;
					} else {
						std::get<0>(cellnext) = -2;
					}
				}
			}
		}
	}
	return m_next;
}


void printMap (GridMap const& m, Pathogen p)
{
	for (auto row : m) {
		for (auto cell : row) {
			if (p.susceptible(cell)) {
				std::cout << 's';
			} else if (p.exposed(cell)) {
				std::cout << 'E';
			} else if (p.infectious(cell)) {
				std::cout << 'I';
			} else if (p.deceased(cell)) {
				std::cout << ' ';
			} else if (p.recovered(cell)) {
				std::cout << 'r';
			} else {
				std::cout << '!';
			}
		}
		std::cout << std::endl;
	}
}

void seedDisease (GridMap& m, Pathogen p, int count) {
	std::random_device rd;
	//std::mt19937 gen(rd());
	std::default_random_engine gen(rd());
	std::uniform_int_distribution<> d(0, (m.size() * m[0].size()) - 1);
	while (count--) {
		auto k = d(gen);
		auto i = k / m.size();
		auto j = k % m.size();
		auto& cell = m[i][j];
		std::get<1>(cell) = p.incubates();
		std::get<2>(cell) = p.infects();
		std::get<0>(cell) = 1;
	}
}

int countInfected (GridMap const& m, Pathogen p)
{
	int count = 0;
	for (auto row : m) {
		for (auto cell : row) {
			if (p.exposed(cell) || p.infectious(cell)) {
				++count;
			}
		}
	}
	return count;
}

int countRecovered (GridMap const& m, Pathogen p)
{
	int count = 0;
	for (auto row : m) {
		for (auto cell : row) {
			if (p.recovered(cell)) {
				++count;
			}
		}
	}
	return count;
}

int countDeceased (GridMap const& m, Pathogen p)
{
	int count = 0;
	for (auto row : m) {
		for (auto cell : row) {
			if (p.deceased(cell)) {
				++count;
			}
		}
	}
	return count;
}

int main(int argc, char** argv)
{
	unsigned const N = std::atoi(argv[1]);
	unsigned const T = std::atoi(argv[2]);
	double PT = std::atof(argv[3]);
	double PD = std::atof(argv[4]);
	short const ME = std::atoi(argv[5]);
	short const KE = std::atoi(argv[6]);
	short const MI = std::atoi(argv[7]);
	short const KI = std::atoi(argv[8]);
	short const KT = std::atoi(argv[9]);
	int const S = std::atoi(argv[10]);

	std::srand(std::time(nullptr));

	Pathogen ebola { "Ebola-like", PT, PD, ME, KE, MI, KI, KT };

	GridMap minit { N, std::vector<Host>( N, std::make_tuple(0, 0, 0, 0) ) };
	for (auto& row : minit) {
		for (auto& cell : row) {
			std::get<3>(cell) = ebola.numNeighbors();
		}
	}

	seedDisease(minit, ebola, S);

	GridMap m { minit };
	auto t = 0u;
	while (countInfected(m, ebola) > 0 && t++ < T) {
		m = computeNext(m, ebola);
	}
	printMap(m, ebola);
	std::cout << "After " << t << " days... (" << NCOMPS << ',' << NRANDS << ")\n";
	std::cout << countRecovered(m, ebola) << " recovered, " << countDeceased(m, ebola) << " died" << std::endl;

	return 0;
}

