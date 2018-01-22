CXX=clang++
CXXFLAGS=-std=c++14 -pedantic -O3 -I$(IDIR)

IDIR=include
ODIR=obj
LDIR=lib

LIBS=-lGL -lGLU -lGLEW -lglut
EXES=gpathogen

_DEPS=
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ= gpathogen.o InitShader.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: $(EXES)

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

gpathogen: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXES) $(ODIR)/*.o *~ core $(IDIR)/*~

