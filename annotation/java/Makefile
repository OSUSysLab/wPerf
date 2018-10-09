jdir := $(shell echo ${JAVA_HOME})
all:
	javac edu/osu/cse/ops/UDS.java
	javah edu.osu.cse.ops.UDS
	gcc -c kerntool.c -lpthread -I"${jdir}/include" -I"${jdir}/include/linux" -o kerntool.o -fPIC
	gcc -shared -lpthread -o libkerntool.so kerntool.o -fPIC
	jar -cfve uds.jar edu.osu.cse.ops.UDS edu/osu/cse/ops/*.class
	cp *.so /tmp/
	cp *.jar /tmp/
clean:
	rm *.jar
	rm *.so
	rm *.o
