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

#include "Angel.h"

// Integer proxy representation - 16bits
// storage upper-bound: 2*N Bytes
// susceptible = 0
// exposed = [1,kE]
// infected = [kE+1,kE+kI]
// recovered = -2
// deceased = -1
using Host = short;

class Pathogen {
public:
	Pathogen(std::string name = "", double pE = 0.005, double pD = 0.5, short kE = 7, short kI = 7)
		: name(name), pcatch(pE), pdie(pD), kE(kE), kI(kE+kI) {}
	bool susceptible (Host h) { return h ==  0; }
	bool deceased    (Host h) { return h == -1; }
	bool recovered   (Host h) { return h == -2; }
	bool exposed     (Host h) { return 0  < h && h <= kE; }
	bool infectious  (Host h) { return kE < h && h <= kI; }

	bool catches() { return pcatch(rng); }
	bool dies() { return pdie(rng); }

private:
	std::string name;
	static std::mt19937 rng;
	std::bernoulli_distribution pcatch;
	std::bernoulli_distribution pdie;
	short kE;
	short kI;
};

std::mt19937 Pathogen::rng { std::random_device()() };

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

	for (auto i = 0u; i < m.size(); ++i) {
		auto& row = m[i];
		for (auto j = 0u; j < row.size(); ++j) {
			auto& cell = row[j];
			auto& cellnext = m_next[i][j];
			if (p.susceptible(cell)) {
				int exposure = isExposed(i, j, m, p);
				for (auto k = 0; k < exposure; ++k) {
					if (p.catches()) {
						++cellnext;
						break;
					}
				}
			} else if (p.exposed(cell)) {
				++cellnext;
			} else if (p.infectious(cell)) {
				++cellnext;
				if (!p.infectious(cellnext)) {
					if (p.dies()) {
						cellnext = -1;
					} else {
						cellnext = -2;
					}
				}
			}
		}
	}
	return m_next;
}


void printMap (GridMap const& m, Pathogen p)
{
	for (auto& row : m) {
		for (auto& cell : row) {
			if (p.susceptible(cell)) {
				std::cout << 's';
			} else if (p.exposed(cell)) {
				std::cout << 'e';
			} else if (p.infectious(cell)) {
				std::cout << 'I';
			} else if (p.deceased(cell)) {
				std::cout << ' ';
			} else if (p.recovered(cell)) {
				std::cout << 'R';
			} else {
				std::cout << '!';
			}
		}
		std::cout << std::endl;
	}
}

void renderMap(GridMap const& m)
{
	auto k = 0;
	for (auto& row : m) {
		auto n = row.size() * sizeof(short);
		glBufferSubData(GL_ARRAY_BUFFER, k, n, row.data());
		k += n;
	}
	glutPostRedisplay();
}

void seedDisease (GridMap& m, Pathogen p, int count) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> d(0, (m.size() * m[0].size()) - 1);
	while (count--) {
		auto k = d(gen);
		auto i = k / m.size();
		auto j = k % m.size();
		m[i][j] = 1;
	}
}

int countInfected (GridMap const& m, Pathogen p)
{
	int count = 0;
	for (auto& row : m) {
		for (auto& cell : row) {
			if (p.exposed(cell) || p.infectious(cell)) {
				++count;
			}
		}
	}
	return count;
}

GridMap m;
Pathogen ebola;
unsigned int T;
unsigned int M;

void runSim()
{
	//printMap(m, ebola);
	//std::cout << std::endl;
	auto t = 0u;
	while (countInfected(m, ebola) > 0 && t++ < T) {
		m = computeNext(m, ebola);
		//std::cout << i << std::endl;
		//printMap(m, ebola);
		//std::cerr << "******************************************************\n";
	}
	printMap(m, ebola);
	std::cout << t << std::endl;
	std::cout << std::endl;

	//std::cin.get();
	//return 0;
}

//0000000000000000
//0000000000000001
//0000000000011111
//0000001111111111
//1111111111111110
//1111111111111111

// 0000111110000000 - mask for kI?
// 0000000001111100 - mask for kE?


//----------------------------------------------------------------------------

vec2* points;
int N;

void
init( int h, int w, int kE, int kI )
{
    N = h * w;

    points = new vec2[N];
    for (auto i = 0; i < h; ++i) {
        for (auto j = 0; j < w; ++j) {
            points[i*w+j] = vec2(static_cast<float>(i), static_cast<float>(j));
        }
    }

    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram( program );

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Create and initialize a buffer object
    GLuint vbuffer;
    glGenBuffers( 1, &vbuffer );
    glBindBuffer( GL_ARRAY_BUFFER, vbuffer );
    glBufferData( GL_ARRAY_BUFFER, N * sizeof(vec2), points, GL_STATIC_DRAW );

    // Initialize the vertex position attribute from the vertex shader    
    GLuint loc = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(0) );

    GLuint sbuffer;
    glGenBuffers( 1, &sbuffer );
    glBindBuffer( GL_ARRAY_BUFFER, sbuffer );
    glBufferData( GL_ARRAY_BUFFER, N * sizeof(short), nullptr, GL_STATIC_DRAW );

    // Initialize the vertex position attribute from the vertex shader    
    GLuint sloc = glGetAttribLocation( program, "vState" );
    glEnableVertexAttribArray( sloc );
    glVertexAttribIPointer( sloc, 1, GL_SHORT, 0, BUFFER_OFFSET(0) );

    mat4 projection = Ortho2D(0, w, 0, h);
    GLuint projection_loc = glGetUniformLocation(program, "projection");
    glUniformMatrix4fv(projection_loc, 1, GL_TRUE, projection);

    GLuint kE_loc = glGetUniformLocation(program, "kE");
    glUniform1i(kE_loc, kE);

    GLuint kI_loc = glGetUniformLocation(program, "kI");
    glUniform1i(kI_loc, kI);

    renderMap(m);

    glClearColor( 0.5, 0.5, 0.5, 1.0 ); /* gray background */

    //delete points;
}

//----------------------------------------------------------------------------

void
display( void )
{
    glClear( GL_COLOR_BUFFER_BIT );

    glDrawArrays( GL_POINTS, 0, N );

    glutSwapBuffers();
}

//----------------------------------------------------------------------------

void
keyboard( unsigned char key, int x, int y )
{
    switch ( key ) {
    case 033:
        exit( EXIT_SUCCESS );
        break;
    }
}

//----------------------------------------------------------------------------

void update( int frame )
{
	//auto t = 0u;
	if (countInfected(m, ebola) > 0 && frame++ < T) {
		glutTimerFunc( 17, update, frame + 1 );
		m = computeNext(m, ebola);
		if (t % M == 0) {
			renderMap(m);
		}
	} else {
		glutTimerFunc( 17, update, 0 );
		m = { m.size(), std::vector<Host>( m.size(), 0 ) };
	}
}

//----------------------------------------------------------------------------

int
main( int argc, char **argv )
{
	unsigned const N = std::atoi(argv[1]);
	T = std::atoi(argv[2]);
        double PT = std::atof(argv[3]);
        double PD = std::atof(argv[4]);
        short const KE = std::atoi(argv[5]);
        short const KI = std::atoi(argv[6]);
        int const S = std::atoi(argv[7]);
	M = std::atoi(argv[8]);

	std::srand(std::time(nullptr));

	m = { N, std::vector<Host>( N, 0 ) };

	ebola = { "Ebola-like", PT, PD, KE, KI };

	seedDisease(m, ebola, S);

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE );
    glutInitWindowSize( N, N );
    glutInitContextVersion( 3, 2 );
    glutInitContextProfile( GLUT_CORE_PROFILE );
    glutCreateWindow( "Ghostmap" );

    glewExperimental = GL_TRUE;
    glewInit();

    init(N, N, KE, KI);

    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutTimerFunc( 17, update, 0 );

    glutMainLoop();

    return 0;
}
