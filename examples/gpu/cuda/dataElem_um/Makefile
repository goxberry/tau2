CUDA_ARCH_FLAGS := -arch=sm_35
CC_FLAGS += $(CUDA_ARCH_FLAGS)

#EXE = dataElem_um dataElem_um_c++_1 dataElem_um_c++_2
EXE = dataElem_um

all: $(EXE)

% : %.cu
	nvcc $< $(CC_FLAGS) $(LIB_FLAGS) -o $@

clean: 
	rm -f $(EXE)