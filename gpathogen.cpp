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
#include <cstdint>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "Angel.h"

// Integer proxy representation - 16bits
// storage upper-bound: 2*N Bytes
// susceptible = 0
// exposed = [1,kE]
// infected = [kE+1,kE+kI]
// recovered = -2
// deceased = -1
using Host = std::tuple<int8_t, int8_t, int8_t>;

class Pathogen
{
    public:
    Pathogen(std::string name = "Ebola", double pE = 0.005, double pD = 0.5,
             int8_t minE = 2, int8_t kE = 9, int8_t minI = 7, int8_t kI = 9,
             int8_t kT = 1, int8_t kQ = 1)
        : name(name), pcatch(pE), pdie(pD), edist(1.0 / (kE - minE + 1)),
          idist(1.0 / (kI - minI + 1)), ndist(1.0 / kT), minE(minE), minI(minI),
          timeQ(kQ)
    {
    }

    bool isSusceptible(Host const& h) { return std::get<0>(h) == 0; }
    bool isExposed(Host const& h) { return std::get<0>(h) == 1; }
    bool isInfectious(Host const& h) { return std::get<0>(h) == 2; }
    bool hasRunCourse(Host const& h) { return std::get<0>(h) == 3; }
    bool isRecovered(Host const& h) { return std::get<0>(h) == 4; }
    bool isDeceased(Host const& h) { return std::get<0>(h) == 5; }
    bool isDetected(Host const& h)
    {
        return isInfectious(h) && std::get<1>(h) < minI;
    }

    void infect(Host& h)
    {
        std::get<0>(h) = 1;
        std::get<1>(h) = incubationPeriod();
    }
    void worsen(Host& h)
    {
        if (--std::get<1>(h) == 0) {
            ++std::get<0>(h);
            if (hasRunCourse(h)) {
                if (dies()) {
                    kill(h);
                } else {
                    recover(h);
                }
            } else {
                std::get<1>(h) = infectionPeriod();
            }
        }
    }
    void recover(Host& h) { std::get<0>(h) = 4; }
    void kill(Host& h) { std::get<0>(h) = 5; }

    bool   catches() { return pcatch(rng); }
    bool   dies() { return pdie(rng); }
    int8_t incubationPeriod() { return minE + edist(rng); }
    int8_t infectionPeriod() { return minI + idist(rng); }
    int8_t numNeighbors() { return 1 + ndist(rng); }

    private:
    std::string         name;
    static std::mt19937 rng;
    // static std::default_random_engine rng;
    std::bernoulli_distribution   pcatch;
    std::bernoulli_distribution   pdie;
    std::geometric_distribution<> edist;
    std::geometric_distribution<> idist;
    std::geometric_distribution<> ndist;
    int8_t                        minE;
    int8_t                        minI;
    int8_t                        timeQ;
};

std::mt19937 Pathogen::rng{std::random_device()()};

using GridMap = std::vector<std::vector<Host>>;

GridMap computeNext(GridMap const& m, Pathogen p)
{
    GridMap m_next = m;
    int     N      = m.size();
    int     M      = m[0].size();
    for (auto i = 0; i < N; ++i) {
        auto& row = m[i];
        for (auto j = 0; j < M; ++j) {
            auto& cell     = row[j];
            auto& cellnext = m_next[i][j];
            if (p.isExposed(cell)) {
                p.worsen(cellnext);
            } else if (p.isInfectious(cell)) {
                p.worsen(cellnext);
                // if (p.isDetected(cell)) {
                //	std::get<2>(cellnext) = 0;
                //}
                auto hk = std::get<2>(cellnext);
                for (auto hi = i - hk; hi <= i + hk; ++hi) {
                    for (auto hj = j - hk; hj <= j + hk; ++hj) {
                        auto  realj = hj;
                        auto& neighbor =
                            m_next[hi < 0 ? (N + hi)
                                          : (hi >= N ? (hi - N) : hi)]
                                  [hj < 0 ? (M + hj)
                                          : (hj >= M ? (hj - M) : hj)];
                        if (p.isSusceptible(neighbor)) {
                            if (p.catches()) {
                                p.infect(neighbor);
                            }
                        }
                    }
                }
            }
        }
    }
    return m_next;
}

void printMap(GridMap const& m, Pathogen p)
{
    for (auto& row : m) {
        for (auto& cell : row) {
            if (p.isSusceptible(cell)) {
                std::cout << 's';
            } else if (p.isExposed(cell)) {
                if (p.isInfectious(cell)) {
                    std::cout << 'I';
                } else {
                    std::cout << 'e';
                }
            } else if (p.isDeceased(cell)) {
                std::cout << ' ';
            } else if (p.isRecovered(cell)) {
                std::cout << 'R';
            } else {
                std::cout << '!';
            }
        }
        std::cout << std::endl;
    }
}

void renderMap(GridMap const& map)
{
    auto k = 0;
    for (auto& row : map) {
        auto n = row.size() * sizeof(Host);
        glBufferSubData(GL_ARRAY_BUFFER, k, n, row.data());
        k += n;
    }
    glutPostRedisplay();
}

void seedDisease(GridMap& m, Pathogen p, int count)
{
    std::random_device rd;
    // std::mt19937 gen(rd());
    std::default_random_engine      gen(rd());
    std::uniform_int_distribution<> d(0, (m.size() * m[0].size()) - 1);
    while (count--) {
        auto  k    = d(gen);
        auto  i    = k / m.size();
        auto  j    = k % m.size();
        auto& cell = m[i][j];
        p.infect(cell);
    }
}

int countInfected(GridMap const& m, Pathogen p)
{
    int count = 0;
    for (auto& row : m) {
        for (auto& cell : row) {
            if (p.isExposed(cell) || p.isInfectious(cell)) {
                ++count;
            }
        }
    }
    return count;
}

int countRecovered(GridMap const& m, Pathogen p)
{
    int count = 0;
    for (auto row : m) {
        for (auto cell : row) {
            if (p.isRecovered(cell)) {
                ++count;
            }
        }
    }
    return count;
}

int countDeceased(GridMap const& m, Pathogen p)
{
    int count = 0;
    for (auto row : m) {
        for (auto cell : row) {
            if (p.isDeceased(cell)) {
                ++count;
            }
        }
    }
    return count;
}

GridMap      m;
Pathogen     ebola;
unsigned int T;
unsigned int M;
unsigned int S;

void runSim()
{
    // printMap(m, ebola);
    // std::cout << std::endl;
    auto t = 0u;
    while (countInfected(m, ebola) > 0 && t++ < T) {
        m = computeNext(m, ebola);
        // std::cout << i << std::endl;
        // printMap(m, ebola);
        // std::cerr <<
        // "******************************************************\n";
    }
    printMap(m, ebola);
    std::cout << t << std::endl;
    std::cout << std::endl;

    // std::cin.get();
    // return 0;
}

// 0000000000000000
// 0000000000000001
// 0000000000011111
// 0000001111111111
// 1111111111111110
// 1111111111111111

// 0000111110000000 - mask for kI?
// 0000000001111100 - mask for kE?

//----------------------------------------------------------------------------

vec2* points;
int   N;

void init(int h, int w)
{
    N = h * w;

    points = new vec2[N];
    for (auto i = 0; i < h; ++i) {
        for (auto j = 0; j < w; ++j) {
            points[i * w + j] =
                vec2(static_cast<float>(j), static_cast<float>(h - i));
        }
    }

    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create and initialize a buffer object
    GLuint vbuffer;
    glGenBuffers(1, &vbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(vec2), points, GL_STATIC_DRAW);

    // Initialize the vertex position attribute from the vertex shader
    GLuint loc = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint sbuffer;
    glGenBuffers(1, &sbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, sbuffer);
    glBufferData(GL_ARRAY_BUFFER, N * sizeof(Host), nullptr, GL_DYNAMIC_DRAW);

    // Initialize the vertex position attribute from the vertex shader
    GLuint sloc = glGetAttribLocation(program, "vState");
    glEnableVertexAttribArray(sloc);
    glVertexAttribIPointer(sloc, 3, GL_BYTE, 0, BUFFER_OFFSET(0));

    mat4   projection     = Ortho2D(0, w, 0, h);
    GLuint projection_loc = glGetUniformLocation(program, "projection");
    glUniformMatrix4fv(projection_loc, 1, GL_TRUE, projection);

    renderMap(m);

    glClearColor(0.5, 0.5, 0.5, 1.0); /* gray background */

    // delete points;
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_POINTS, 0, N);

    glutSwapBuffers();
}

//----------------------------------------------------------------------------

void update(int t)
{
    if (countInfected(m, ebola) > 0 && t < T) {
        glutTimerFunc(17, update, t + M);
        for (auto i = 0; i < M; ++i) {
            m = computeNext(m, ebola);
        }
        // printMap(m, ebola);
        renderMap(m);
    } else if (t > 0) {
        std::cout << "After " << t << " days...\n";
        std::cout << countRecovered(m, ebola) << " recovered, "
                  << countDeceased(m, ebola) << " died" << std::endl;
    }
}

void resetMap()
{
    m = {m.size(), std::vector<Host>(m[0].size(), std::make_tuple(0, 0, 0))};
    for (auto& row : m) {
        for (auto& cell : row) {
            std::get<2>(cell) = ebola.numNeighbors();
        }
    }
    seedDisease(m, ebola, S);
    glutTimerFunc(17, update, 0);
}

//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'R':  // fall-through!
    case 'r': resetMap(); break;
    case 033: exit(EXIT_SUCCESS); break;
    }
}

//----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    unsigned const N = std::atoi(argv[1]);
    T                = std::atoi(argv[2]);
    double       PT  = std::atof(argv[3]);
    double       PD  = std::atof(argv[4]);
    int8_t const ME  = std::atoi(argv[5]);
    int8_t const KE  = std::atoi(argv[6]);
    int8_t const MI  = std::atoi(argv[7]);
    int8_t const KI  = std::atoi(argv[8]);
    int8_t const KQ  = std::atoi(argv[9]);
    int8_t const KT  = std::atoi(argv[10]);
    S                = std::atoi(argv[11]);
    M                = std::atoi(argv[12]);

    std::srand(std::time(nullptr));

    ebola = {"Ebola-like", PT, PD, ME, KE, MI, KI, KT, KQ};

    m = {N, std::vector<Host>(N, std::make_tuple(0, 0, 0))};
    for (auto& row : m) {
        for (auto& cell : row) {
            std::get<2>(cell) = ebola.numNeighbors();
        }
    }

    seedDisease(m, ebola, S);

    std::cerr << "Press 1 for Console (otherwise load GUI): ";
    if (std::cin.get() == '1') {
        auto t = 0u;
        while (countInfected(m, ebola) > 0 && t++ < T) {
            m = computeNext(m, ebola);
            std::cerr << '.';
        }
        std::cerr << '\n';
        printMap(m, ebola);
        std::cout << "After " << t
                  << " days...\n";  //(" << NCOMPS << ',' << NRANDS << ")\n";
        std::cout << countRecovered(m, ebola) << " recovered, "
                  << countDeceased(m, ebola) << " died" << std::endl;
    } else {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
        glutInitWindowSize(N, N);
        glutInitContextVersion(3, 2);
        glutInitContextProfile(GLUT_CORE_PROFILE);
        glutCreateWindow("Ghostmap");

        glewExperimental = GL_TRUE;
        glewInit();

        init(N, N);

        glutDisplayFunc(display);
        glutKeyboardFunc(keyboard);
        glutTimerFunc(17, update, 0);

        glutMainLoop();
    }
    return 0;
}
