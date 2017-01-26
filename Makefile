all: sample

sample: sample.cpp glad.c
	g++ -o sample sample.cpp glad.c -lGL -lglfw -ldl -lmpg123 -lao

clean:
	rm sample

# all: sample2D

# sample2D: Sample_GL3_2D.cpp glad.c
# 	g++ -o sample2D Sample_GL3_2D.cpp glad.c -lGL -lglfw -ldl

# clean:
# 	rm sample2D