
CXXFLAGS=-Wall -g -O

frondsim: main.o Frond.o Gizmo.o FrondGroup.o
	$(CXX) -Wl,--export-dynamic -o $@ $^ -ldl -lglut -lGLU -lGL -L/usr/X11R6/lib -lXmu -lXi -lXext -lX11

main.o: Frond.h FrondGroup.h Gizmo.h
Frond.o: Frond.h
Gizmo.o: Gizmo.h Frond.h
FrondGroup.o: FrondGroup.h Frond.h Gizmo.h

%.gizmo: ../%.c
	$(CC) -shared -o $@ $< -DTESTRIG -I../libpwm

clean:
	rm -f *.o *.gizmo *~ frondsim
