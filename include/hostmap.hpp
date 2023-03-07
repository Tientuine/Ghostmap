#ifndef HPP_HOSTMAP
#define HPP_HOSTMAP

#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include "pathogen.hpp"

/// <summary>
/// Rectangular grid of host individuals along with a disease to model.
/// </summary>
class HostMap : public std::vector<std::vector<Host>>
{
    using super = std::vector<std::vector<Host>>;
    using row_type = std::vector<Host>;

    Pathogen disease;

public:
    /// <summary>
    /// Initialize this map with the specified dimensions and disease
    /// </summary>
    /// <param name="disease">representation of a communicable disease</param>
    /// <param name="r">number of rows in the grid</param>
    /// <param name="c">number of columns in the grid</param>
    HostMap(Pathogen const& disease, int r = 100, int c = 100)
        : super(r, row_type(c)), disease(disease)
    {
        for (auto& row : *this) {
            for (auto& cell : row) {
                std::get<2>(cell) = disease.numNeighbors();
            }
        }
    }

    /// <summary>Width of the the map.</summary>
    /// <returns>number of columns in the grid</returns>
    size_t col_count() const { return (*this)[0].size(); }

    /// <summary>Height of the the map.</summary>
    /// <returns>number of rows in the grid</returns>
    size_t row_count() const { return size(); }

    /// <summary>Resets the data for all hosts in the map.</summary>
    void reset()
    {
        for (auto& row : *this) {
            for (auto& cell : row) {
                cell = std::make_tuple<short,short,short>(0, 0, disease.numNeighbors());
            }
        }
    }

    /// <summary>
    /// Provides a view of the grid as a torus topology.
    /// </summary>
    /// <param name="m">a rectangular grid of hosts</param>
    /// <param name="i">a row index</param>
    /// <param name="j">a column index</param>
    /// <returns>a cell from the grid <c>m</c></returns>
    auto getNeighbor(int i, int j) -> decltype((*this)[0][0])
    {
        auto N = static_cast<int>(row_count());
        auto M = static_cast<int>(col_count());

        i = (i < 0) ? (N + i) : (i >= N ? (i - N) : i);
        j = (j < 0) ? (M + j) : (j >= M ? (j - M) : j);
        return (*this)[i][j];
    }

    /// <summary>
    /// Identify and potentially infect the close contacts of individual (i,j).
    /// </summary>
    /// <param name="i">row position of a host in the grid</param>
    /// <param name="j">column position of a host in the grid</param>
    void computeContacts(int i, int j)
    {
        auto t = std::get<2>((*this)[i][j]);
        auto k = std::lround((std::sqrt(t + 1) - 1) / 2);
        for (auto hi = i - k; hi <= i + k; ++hi) {
            for (auto hj = j - k; hj <= j + k; ++hj) {
                auto& x = getNeighbor(hi, hj);
                if (disease.isSusceptible(x)) disease.expose(x);
            }
        }
    }

    /// <summary>
    /// Advance the simulation one time step (i.e., day).
    /// </summary>
    void computeNext()
    {
        HostMap m_prev(*this);
        auto N = row_count();
        auto M = col_count();
        for (auto i = 0; i < N; ++i) {
            for (auto j = 0; j < M; ++j) {
                auto& cell_prev = m_prev[i][j];
                auto& cell = (*this)[i][j];
                if (disease.isExposed(cell_prev))
                    disease.worsen(cell);
                else if (disease.isInfectious(cell_prev)) {
                    disease.worsen(cell);
                    // if (p.isDetected(cell_prev)) {
                    //    std::get<2>(cell) = 0;
                    //}
                    computeContacts(i, j);
                }
            }
        }
    }

    /// <summary>
    /// Print a text representation of the map to standard output.
    /// </summary>
    void print() const
    {
        for (auto& row : *this) {
            for (auto& cell : row) {
                if (disease.isSusceptible(cell)) {
                    std::cout << 's';
                }
                else if (disease.isExposed(cell)) {
                    if (disease.isInfectious(cell)) {
                        std::cout << 'I';
                    }
                    else {
                        std::cout << 'e';
                    }
                }
                else if (disease.isDeceased(cell)) {
                    std::cout << ' ';
                }
                else if (disease.isRecovered(cell)) {
                    std::cout << 'R';
                }
                else {
                    std::cout << '!';
                }
            }
            std::cout << std::endl;
        }
    }

    /// <summary>
    /// Print aggregate totals for the map so far.
    /// </summary>
    void printSummary() const
    {
        std::cout
            << countDeceased() << " died, "
            << countRecovered() << " recovered, "
            << countInfected() << " still infected."
            << std::endl;
    }

    /// <summary>
    /// Count the number of active infections.
    /// </summary>
    /// <returns>total number of infected hosts in the map</returns>
    int countInfected() const
    {
        int count = 0;
        for (auto& row : *this) {
            for (auto& cell : row) {
                if (disease.isExposed(cell) || disease.isInfectious(cell)) {
                    ++count;
                }
            }
        }
        return count;
    }

    /// <summary>
    /// Count the number of recovered individuals.
    /// </summary>
    /// <returns>total number of recovered hosts in the map</returns>
    int countRecovered() const
    {
        int count = 0;
        for (auto& row : *this) {
            for (auto& cell : row) {
                if (disease.isRecovered(cell)) {
                    ++count;
                }
            }
        }
        return count;
    }

    /// <summary>
    /// Count the number of deceased individuals.
    /// </summary>
    /// <returns>total number of dead hosts in the map</returns>
    int countDeceased() const
    {
        int count = 0;
        for (auto& row : *this) {
            for (auto& cell : row) {
                if (disease.isDeceased(cell)) {
                    ++count;
                }
            }
        }
        return count;
    }

    /// <summary>
    /// Plant the disease in a given number of individuals (i.e., "patient zero" candidates).
    /// </summary>
    /// <param name="count">number of infected individuals at the start of the simulation</param>
    void seedDisease(int count)
    {
        std::random_device rd;
        std::default_random_engine gen(rd());
        std::uniform_int_distribution<> d(0, static_cast<int>(row_count() * col_count()) - 1);
        while (count--) {
            auto  k = d(gen);
            auto  i = k / size();
            auto  j = k % size();
            auto& cell = (*this)[i][j];
            disease.infect(cell);
        }
    }
};

#endif /*HPP_HOSTMAP*/
