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
#include <vector>

// Integer proxy representation - 16bits
// storage upper-bound: 2*N Bytes
// susceptible = 0
// exposed = [1,kE]
// infected = [kE+1,kE+kI]
// recovered = -2
// deceased = -1
using Host = std::pair<short,short>;

unsigned long NCOMPS = 0;
unsigned long NRANDS = 0;

class Pathogen {
public:
	Pathogen(std::string name = "Ebola", double pE = 0.005, double pD = 0.5, short minE = 2, short kE = 9, short kI = 9)
		: name(name), pcatch(pE), pdie(pD), dist(1.0/(kE-minE+1)), minE(minE), kI(kI) {}
	bool susceptible (Host const& h) { ++NCOMPS; return h.first ==  0; }
	bool deceased    (Host const& h) { ++NCOMPS; return h.first == -1; }
	bool recovered   (Host const& h) { ++NCOMPS; return h.first == -2; }
	bool exposed     (Host const& h) { ++NCOMPS; return 0 < h.first && h.first <= h.second; }
	bool infectious  (Host const& h) { ++NCOMPS; return h.second < h.first && h.first <= (h.second + kI); }

	bool catches() { ++NRANDS; return pcatch(rng); }
	bool dies() { ++NRANDS; return pdie(rng); }
	int incubation() { ++NRANDS; return minE + dist(rng); }

private:
	std::string name;
	static std::mt19937 rng;
	//static std::default_random_engine rng;
	std::bernoulli_distribution pcatch;
	std::bernoulli_distribution pdie;
	std::geometric_distribution<> dist;
	short minE;
	short kE;
	short kI;
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
				++(cellnext.first);
			} else if (p.infectious(cell)) {
				++(cellnext.first);
				for (auto hi = std::max(0, i - 1); hi <= std::min(i + 1, N - 1); ++hi) {
					for (auto hj = std::max(0, j - 1); hj <= std::min(j + 1, M - 1); ++hj) {
						auto& neighbor = m_next[hi][hj];
						if (p.susceptible(neighbor)) {
							if (p.catches()) {
								neighbor.second = p.incubation();
								++(neighbor.first);
							}
						}
					}
				}
				
				if (!p.infectious(cellnext)) {
					if (p.dies()) {
						cellnext.first = -1;
					} else {
						cellnext.first = -2;
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
		cell.second = p.incubation();
		cell.first = 1;
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

int main(int argc, char** argv)
{
	unsigned const N = std::atoi(argv[1]);
	unsigned const T = std::atoi(argv[2]);
	double PT = std::atof(argv[3]);
	double PD = std::atof(argv[4]);
	short const ME = std::atoi(argv[5]);
	short const KE = std::atoi(argv[6]);
	short const KI = std::atoi(argv[7]);
	int const S = std::atoi(argv[8]);

	std::srand(std::time(nullptr));

	GridMap minit { N, std::vector<Host>( N, {0,0} ) };

	Pathogen ebola { "Ebola-like", PT, PD, ME, KE, KI };

	seedDisease(minit, ebola, S);

	GridMap m { minit };
	printMap(m, ebola);
	std::cout << std::endl;
	auto t = 0u;
	while (countInfected(m, ebola) > 0 && t++ < T) {
		m = computeNext(m, ebola);
	}
	printMap(m, ebola);
	std::cout << t << ' ' << NCOMPS << ' ' << NRANDS << std::endl;

	return 0;
}

