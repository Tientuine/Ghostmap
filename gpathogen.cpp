#include "Angel.h"
#include "hostmap.hpp"

//-- Static functions and data for convenience -------------------------------

// Convenience container for data that must persist for the durection
// of the entire program and also be accessible to functions.
struct Scenario
{
    static HostMap* map;
    static Pathogen disease;
    static unsigned int T;
    static unsigned int M;
    static unsigned int S;

    static void runSim()
    {
        auto t = 0u;
        while (map->countInfected() > 0 && t++ < T) {
            map->computeNext();
        }
        map->print();
        std::cout << t << std::endl;
        std::cout << std::endl;
    }

    static void reset()
    {
        map->reset();
        map->seedDisease(S);
    }
};

HostMap* Scenario::map;
Pathogen Scenario::disease;
unsigned int Scenario::T;
unsigned int Scenario::M;
unsigned int Scenario::S;

// Convenience class containing functions and data for use with OpenGL/GLUT.
struct VisCallbacks
{
    static int N;
    static vec2* points;

    static void display(void)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, N);
        glutSwapBuffers();
    }

    static void keyboard(unsigned char key, int x, int y)
    {
        switch (key) {
        case 'R':  // fall-through!
        case 'r':
            Scenario::reset();
            glutTimerFunc(17, update, 0);
            break;
        case 033: exit(EXIT_SUCCESS); break;
        }
    }

    static void init(int h, int w)
    {
        N = h * w;
        for (auto i = 0; i < h; ++i) {
            for (auto j = 0; j < w; ++j) {
                points[i * w + j] =
                    vec2(static_cast<float>(j), static_cast<float>(h - i));
            }
        }

        // Load shaders and use the resulting shader program
        GLuint program = InitShader("vshader.glsl", "fshader.glsl");
        glUseProgram(program);

        // Display the grid using an orthographic projection
        mat4 projection = Ortho2D(0, static_cast<GLfloat>(w), 0, static_cast<GLfloat>(h));
        GLuint projection_loc = glGetUniformLocation(program, "projection");
        glUniformMatrix4fv(projection_loc, 1, GL_TRUE, projection);

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        GLuint pBuffer;
        glGenBuffers(1, &pBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, pBuffer);
        glBufferData(GL_ARRAY_BUFFER, N * sizeof(vec2), points, GL_STATIC_DRAW);

        GLuint posLoc = glGetAttribLocation(program, "vPosition");
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

        // State attribute determines the fragment color
        GLuint sBuffer;
        glGenBuffers(1, &sBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, sBuffer);
        glBufferData(GL_ARRAY_BUFFER, N * sizeof(Host), nullptr, GL_DYNAMIC_DRAW);

        GLuint stateLoc = glGetAttribLocation(program, "vState");
        glEnableVertexAttribArray(stateLoc);
        glVertexAttribIPointer(stateLoc, 3, GL_SHORT, 0, BUFFER_OFFSET(0));

        glClearColor(0.5, 0.5, 0.5, 1.0); /* gray background */

        render(*Scenario::map);
    }

    static void render(HostMap const& map)
    {
        size_t k = 0;
        for (auto& row : map) {
            auto n = row.size() * sizeof(Host);
            glBufferSubData(GL_ARRAY_BUFFER, k, n, row.data());
            k += n;
        }
        glutPostRedisplay();
    }

    static void update(int dt)
    {
        auto t = static_cast<size_t>(dt);
        std::cerr << ".";
        HostMap& map = *Scenario::map;
        if (map.countInfected() > 0 && t < Scenario::T) {
            glutTimerFunc(17, update, static_cast<int>(t + Scenario::M));
            for (size_t i = 0; i < Scenario::M; ++i) {
                map.computeNext();
            }
            render(map);
        }
        else if (t > 0) {
            std::cout << "After " << t << " days...\n";
            map.printSummary();
        }
    }
};

int VisCallbacks::N;
vec2* VisCallbacks::points = nullptr;

//-- USAGE INSTRUCTIONS ------------------------------------------------------

void printUsage(char* progName)
{
    std::cerr << "Usage:\n"
        << " " << progName << "\n"
        << "   <popn-size> [1000]\n"
        << "   <num-steps> [1000]\n"
        << "   <prob-transmit> [0.01-0.012]\n"
        << "   <prob-death> [0.5]\n"
        << "   <tmin-exposed> [2]\n"
        << "   <tavg-exposed> [9]\n"
        << "   <tmin-infected> [7]\n"
        << "   <tavg-infected> [9]\n"
        << "   <num-contacts> [17]\n"
        << "   <quarantine-delay> [0] (currently unused)\n"
        << "   <num-seeds> [1]\n"
        << "   <step-size> [1]\n";
}

//-- MAIN DRIVER ROUTINE -----------------------------------------------------

int main(int argc, char** argv)
{
    if (argc != 13) {
        printUsage(argv[0]);
        return 1;
    }

    unsigned const N = std::atoi(argv[1]);
    Scenario::T = std::atoi(argv[2]);
    double PT = std::atof(argv[3]);
    double PD = std::atof(argv[4]);
    int8_t const ME = (int8_t)std::atoi(argv[5]);
    int8_t const KE = (int8_t)std::atoi(argv[6]);
    int8_t const MI = (int8_t)std::atoi(argv[7]);
    int8_t const KI = (int8_t)std::atoi(argv[8]);
    int8_t const KT = (int8_t)std::atoi(argv[9]);
    int8_t const KQ = (int8_t)std::atoi(argv[10]);
    Scenario::S = std::atoi(argv[11]);
    Scenario::M = std::atoi(argv[12]);

    HostMap map({ "Ebola-like", PT, PD, ME, KE, MI, KI, KT, KQ }, N, N);
    Scenario::map = &map;
    map.seedDisease(Scenario::S);

    std::cerr << "Press 1 for Console (otherwise load GUI): ";
    if (std::cin.get() == '1') {
        auto t = 0u;
        while (map.countInfected() > 0 && t++ < Scenario::T) {
            map.computeNext();
            std::cerr << '.';
        }
        std::cerr << '\n';
        map.print();
        std::cout << "\nAfter " << t << " days...\n";
        map.printSummary();
    }
    else {
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
        glutInitWindowSize(static_cast<int>(map.row_count()), static_cast<int>(map.col_count()));
        glutInitContextVersion(3, 2);
        glutInitContextProfile(GLUT_CORE_PROFILE);
        glutCreateWindow("Ghostmap");

        glewExperimental = GL_TRUE;
        glewInit();

        vec2* points = new vec2[N * N];
        VisCallbacks::points = points;
        VisCallbacks::init(static_cast<int>(map.row_count()), static_cast<int>(map.col_count()));
        glutDisplayFunc(VisCallbacks::display);
        glutKeyboardFunc(VisCallbacks::keyboard);
        glutTimerFunc(17, VisCallbacks::update, 0);

        glutMainLoop();

        delete[] points;
    }
    return 0;
}
